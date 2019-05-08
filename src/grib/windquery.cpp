/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#include "grib/windquery.h"

#include "grib/gribdownloader.h"
#include "grib/gribreader.h"
#include "geo/calculations.h"
#include "geo/rect.h"
#include "geo/linestring.h"
#include "atools.h"
#include "util/filesystemwatcher.h"
#include "exception.h"
#include "geo/line.h"

using atools::grib::GribDownloader;
using atools::geo::Rect;
using atools::geo::Pos;
using atools::geo::Line;
using atools::geo::LineString;
using atools::geo::normalizeCourse;
using atools::interpolate;
using atools::geo::normalizeLonXDeg;
using atools::geo::normalizeLatYDeg;

namespace atools {
namespace grib {

// Debug IO =======================================================================
QDebug operator<<(QDebug out, const WindPos& windPos)
{
  QDebugStateSaver saver(out);

  out.noquote().nospace() << "Wind(speed " << windPos.wind.speed << ", "
                          << "dir " << windPos.wind.dir << ", " << windPos.pos << ")";

  return out;
}

QDebug operator<<(QDebug out, const Wind& wind)
{
  QDebugStateSaver saver(out);

  out.noquote().nospace() << "Wind[speed " << wind.speed << ", dir " << wind.dir << "]";

  return out;
}

inline float interpolateDir(float f0, float f1, float x0, float x1, float x)
{
  if(f0 < INVALID_DIR_VALUE / 2 && f1 < INVALID_DIR_VALUE / 2)
  {
    if(atools::almostEqual(f0, f1))
      return normalizeCourse(f0);
    else
      return normalizeCourse(f0 + ((f1 - f0) / (x1 - x0)) * (x - x0));
  }
  else
  {
    if(f0 < INVALID_DIR_VALUE / 2)
      return normalizeCourse(f0);

    if(f1 < INVALID_DIR_VALUE / 2)
      return normalizeCourse(f1);
  }
  return 0.f;
}

// ===============================================================
WindQuery::WindQuery(QObject *parentObject, bool logVerbose)
  : QObject(parentObject), verbose(logVerbose)
{
  downloader = new GribDownloader(parentObject, logVerbose);

  // Download or read U and V components of wind - will be used to calculated speed and direction
  downloader->setParameters({"UGRD", "VGRD"});

  // Layers 80 ft AGL and other values are mbar layers
  downloader->setSurfaces({-80, 200, 300, 450, 700});

  connect(downloader, &GribDownloader::gribDownloadFinished, this, &WindQuery::gribDownloadFinished);
  connect(downloader, &GribDownloader::gribDownloadFailed, this, &WindQuery::gribDownloadFailed);

  fileWatcher = new atools::util::FileSystemWatcher(parentObject, logVerbose);
  fileWatcher->setMinFileSize(180000);
  connect(fileWatcher, &atools::util::FileSystemWatcher::fileUpdated, this, &WindQuery::gribFileUpdated);
}

WindQuery::~WindQuery()
{
  delete downloader;
}

void WindQuery::initFromUrl(const QString& baseUrl)
{
  deinit();

  // Start download and conversion when done
  downloader->startDownload(QDateTime(), baseUrl);
}

void WindQuery::initFromFile(const QString& filename)
{
  deinit();

  // Inital load
  gribFileUpdated(filename);

  // Start checking for changes
  fileWatcher->setFilenameAndStart(filename);

  emit windDataUpdated();
}

void WindQuery::initFromData(const QByteArray& data)
{
  deinit();

  GribReader reader(verbose);
  reader.readData(data);
  convertDataset(reader.getDatasets());

  emit windDataUpdated();
}

void WindQuery::deinit()
{
  windLayers.clear();
  downloader->stopDownload();
  fileWatcher->stopWatching();
}

Wind WindQuery::getWindForPos(const Pos& pos) const
{
  return getWindForPos(pos, pos.getAltitude());
}

Wind WindQuery::getWindForPos(const Pos& pos, float altFeet) const
{
  // Calculate grid position
  int col = static_cast<int>(pos.getLonX() + 180.f);
  int row = static_cast<int>(180.f - (pos.getLatY() + 90.f));

  // Get next layers below and above altitude
  WindAltLayer lower, upper;
  layersByAlt(lower, upper, altFeet);

  // Interpolate wind withing a grid rectangle for the lower layer
  WindRect windRectLower;
  windRectForLayer(windRectLower, lower, col, row);
  Wind lW = interpolateRect(windRectLower, pos, col, row);

  if(upper != lower)
  {
    // Interpolate wind within a grid rectangle for the upper layer if layers differ
    WindRect windRectUpper;
    windRectForLayer(windRectUpper, upper, col, row);
    Wind uW = interpolateRect(windRectUpper, pos, col, row);

    // Interpolate between upper and lower layer
    Wind wind;
    wind.speed = interpolate(lW.speed, uW.speed, lower.altitude, upper.altitude, altFeet);
    wind.dir = interpolateDir(lW.dir, uW.dir, lower.altitude, upper.altitude, altFeet);
    return wind;
  }
  return lW;
}

atools::grib::WindPosVector WindQuery::getWindForRect(const Rect& rect, float altFeet) const
{
  WindPosVector result;
  getWindForRect(result, rect, altFeet);
  return result;
}

void WindQuery::getWindForRect(WindPosVector& result, const Rect& rect, float altFeet) const
{
  if(windLayers.isEmpty())
    return;

  if(rect.isPoint())
  {
    // No need to interpolate for single point rect
    WindPos wp;
    wp.wind = getWindForPos(rect.getTopLeft(), altFeet);
    wp.pos = rect.getTopLeft();
    result.append(wp);
  }
  else
  {
    // Split rectangle if it crosses the anti-metidian (date line)
    for(const atools::geo::Rect& r : rect.splitAtAntiMeridian())
    {
      // Calculate grid cell boundaries
      int leftIdx = static_cast<int>(std::floor(r.getWest() + 180.f));
      int rightIdx = static_cast<int>(std::ceil(r.getEast() + 180.f));
      int topIdx = static_cast<int>(std::floor(180.f - (r.getNorth() + 90.f)));
      int bottomIdx = static_cast<int>(std::ceil(180.f - (r.getSouth() + 90.f)));

      // Roll over - there is no value for 360
      if(leftIdx >= 360)
        leftIdx = 359;
      if(rightIdx >= 360)
        rightIdx = 359;

      // Get next layers below and above altitude
      WindAltLayer lower, upper;
      layersByAlt(lower, upper, altFeet);

      // Read one column
      for(int row = topIdx; row <= bottomIdx; row++)
      {
        int startIndex = leftIdx + row * 360;
        int endIndex = rightIdx + row * 360;

        // Read one row of values
        for(int idx = startIndex, col = leftIdx; idx <= endIndex; idx++, col++)
        {
          WindPos wp;
          // Calcualte grid cell upper left position (position for value)
          wp.pos = Pos(static_cast<float>(col) - 180.f, 180.f - (static_cast<float>(row) + 90.f));

          if(upper != lower)
          {
            // Interpolate wind within a grid rectangle for the upper layer if layers differ
            wp.wind.dir = interpolateDir(upper.winds.at(idx).dir, lower.winds.at(idx).dir,
                                         upper.altitude, lower.altitude, altFeet);
            wp.wind.speed = interpolate(upper.winds.at(idx).speed, lower.winds.at(idx).speed,
                                        upper.altitude, lower.altitude, altFeet);
          }
          else
          {
            float dir = lower.winds.at(idx).dir;
            wp.wind.dir = dir < INVALID_DIR_VALUE / 2 ? normalizeCourse(dir) : 0.f;
            wp.wind.speed = lower.winds.at(idx).speed;
          }

          result.append(wp);
        }
      }
    }

    // Sort to get order by y and x values (row by col)
    std::sort(result.begin(), result.end(),
              [](const WindPos& w1, const WindPos& w2) -> bool
        {
          if(atools::almostEqual(w1.pos.getLatY(), w2.pos.getLatY()))
            return w1.pos.getLonX() < w2.pos.getLonX();
          else
            return w1.pos.getLatY() > w2.pos.getLatY();
        });

  }
}

Wind WindQuery::getWindAverageForLineString(const geo::LineString& linestring, float altFeet) const
{
  if(linestring.size() == 1)
    // Only one position
    return getWindForPos(linestring.getPos1(), altFeet);
  else if(linestring.size() == 2)
    // Only one line
    return getWindAverageForLine(linestring.at(0), linestring.at(1), altFeet);
  else
  {
    // Sum up values
    double speed = 0., dir = 0.;
    for(int i = 0; i < linestring.size() - 1; i++)
    {
      Wind w = getWindAverageForLine(linestring.at(i), linestring.at(i + 1), altFeet);
      speed += w.speed;
      dir += w.dir < INVALID_DIR_VALUE / 2 ? w.dir : 0.f;
    }

    // Calculate average
    Wind wind;
    wind.speed = static_cast<float>(speed / (linestring.size() - 1));
    wind.dir = normalizeCourse(static_cast<float>(dir / (linestring.size() - 1)));

    return wind;
  }
}

Wind WindQuery::getWindAverageForLine(const Line& line, float altFeet) const
{
  return getWindAverageForLine(line.getPos1(), line.getPos2(), altFeet);
}

Wind WindQuery::getWindAverageForLine(const Pos& pos1, const Pos& pos2, float altFeet) const
{
  // Calculate the number of samples for a line. Roughly four samples per degree.
  float distanceMeter = pos1.distanceMeterTo(pos2);
  float meterPerDeg = atools::geo::nmToMeter(60.f);
  float meterPerSample = meterPerDeg / SAMPLES_PER_DEGREE;
  int numPoints = atools::roundToInt(distanceMeter / meterPerSample);

  QList<atools::geo::Pos> positions;

  if(numPoints > 1)
  {
    // Calculate number of points - will include origin but not pos2
    pos1.interpolatePoints(pos2, distanceMeter, numPoints, positions);
    positions.append(pos2);
  }
  else
    // Only start and end needed
    positions << pos1 << pos2;

  // Get next layers below and above altitude
  WindAltLayer lower, upper;
  layersByAlt(lower, upper, altFeet);

  Wind wind = {0.f, 0.f};
  WindRect windRectLower, windRectUpper;

  for(const atools::geo::Pos& pos : positions)
  {
    // Calculate grid position
    int col = static_cast<int>(pos.getLonX() + 180.f);
    int row = static_cast<int>(180.f - (pos.getLatY() + 90.f));

    // Get interpolated wind in grid cell at lower layer
    windRectForLayer(windRectLower, lower, col, row);
    Wind lW = interpolateRect(windRectLower, pos, col, row);

    float s, d;
    if(upper != lower)
    {
      // Get interpolated wind in grid cell at upper layer
      windRectForLayer(windRectUpper, upper, col, row);
      Wind uW = interpolateRect(windRectUpper, pos, col, row);

      // Interpolate at altitude between layers
      s = interpolate(lW.speed, uW.speed, lower.altitude, upper.altitude, altFeet);
      d = interpolateDir(lW.dir, uW.dir, lower.altitude, upper.altitude, altFeet);
    }
    else
    {
      s = lW.speed;
      d = lW.dir < INVALID_DIR_VALUE / 2 ? lW.dir : 0.f;
    }
    // qDebug() << pos << "s" << s << "d" << d;
    wind.speed += s;
    wind.dir += d;
  }

  wind.speed /= positions.size();
  wind.dir /= positions.size();
  wind.dir = normalizeCourse(wind.dir);
  return wind;
}

void WindQuery::layersByAlt(WindAltLayer& lower, WindAltLayer& upper, float altitude) const
{
  if(windLayers.size() == 1)
    // Only one wind layer
    lower = upper = windLayers.first();
  else if(windLayers.size() > 1)
  {
    // Returns an iterator pointing to the first item with key key in the map.
    // If the map contains no item with key key, the function returns an iterator to the nearest item with a greater key.
    LayerMap::const_iterator it = windLayers.lowerBound(altitude);
    if(it != windLayers.end())
    {
      if(atools::almostEqual(it->altitude, altitude, ALTITUDE_EPSILON))
        // Layer is at requested altitude - no need to interpolate
        lower = upper = *it;
      else if(it == windLayers.begin())
      {
        // First layer - add a zero wind layer for interpolation between layer and ground
        upper = *it;

        lower.altitude = 0.f;
        lower.winds.fill({INVALID_DIR_VALUE, 0.f}, 360 * 181);
      }
      else
      {
        lower = *(it - 1);
        upper = *it;
      }
    }
    else
      lower = upper = windLayers.last();
  }
}

Wind WindQuery::interpolateRect(const WindRect& windRect, const geo::Pos& pos, int leftCol, int topRow) const
{
  // Calculate right and bottom cell index
  int rightCol = leftCol < 359 ? leftCol + 1 : 0;
  int bottomRow = topRow < 180 ? topRow + 1 : 0;

  // Calculate cell border coordinates in degree
  float left = normalizeLonXDeg(leftCol - 180.f);
  float right = normalizeLonXDeg(rightCol - 180.f);
  float top = 180.f - (topRow + 90.f);
  float bottom = 180.f - (bottomRow + 90.f);

  // Interpolate at top and bottom between left and right
  Wind wTop, wBottom, wind;
  wTop.speed = interpolate(windRect.topLeft.speed, windRect.topRight.speed, left, right, pos.getLonX());
  wTop.dir = interpolateDir(windRect.topLeft.dir, windRect.topRight.dir, left, right, pos.getLonX());
  wBottom.speed = interpolate(windRect.bottomLeft.speed, windRect.bottomRight.speed, left, right, pos.getLonX());
  wBottom.dir = interpolateDir(windRect.bottomLeft.dir, windRect.bottomRight.dir, left, right, pos.getLonX());

  // Interpolate between top and bottom
  wind.speed = interpolate(wTop.speed, wBottom.speed, top, bottom, pos.getLatY());
  wind.dir = interpolateDir(wTop.dir, wBottom.dir, top, bottom, pos.getLatY());
  return wind;
}

void WindQuery::windRectForLayer(WindRect& windRect, const WindAltLayer& layer, int leftCol, int topRow) const
{
  int rightCol = leftCol < 359 ? leftCol + 1 : 0;
  int bottomRow = topRow < 180 ? topRow + 1 : 0;

  windRect.topLeft = windForLayer(layer, leftCol, topRow);
  windRect.topRight = windForLayer(layer, rightCol, topRow);
  windRect.bottomRight = windForLayer(layer, rightCol, bottomRow);
  windRect.bottomLeft = windForLayer(layer, leftCol, bottomRow);
}

void WindQuery::gribDownloadFinished(const GribDatasetVector& datasets, QString)
{
  // Download finished - update coordinates
  convertDataset(datasets);
  emit windDataUpdated();
}

void WindQuery::gribDownloadFailed(const QString& error, int errorCode, QString)
{
  emit windDownloadFailed(error, errorCode);
}

void WindQuery::gribFileUpdated(const QString& filename)
{
  GribReader reader(verbose);
  reader.readFile(filename);
  convertDataset(reader.getDatasets());

  emit windDataUpdated();
}

// Required GRIB parameters:
// shapeOfTheEarth = 6;
// Ni = 360;
// Nj = 181;
// iScansNegatively = 0;
// jScansPositively = 0;
// jPointsAreConsecutive = 0;
// alternativeRowScanning = 0;
// latitudeOfFirstGridPointInDegrees = 90;
// longitudeOfFirstGridPointInDegrees = 0;
// latitudeOfLastGridPointInDegrees = -90;
// longitudeOfLastGridPointInDegrees = 359;
// iDirectionIncrementInDegrees = 1;
// jDirectionIncrementInDegrees = 1;
// Ni — number of points along a parallel - 360
// Nj — number of points along a meridian - 181
// multiplying Ni (octets 31-34) by Nj (octets 35-38) yields the total number of points
// i direction - west to east along a parallel or left to right along an x-axis.
// j direction - south to north along a meridian, or bottom to top along a y-axis.
// V component of wind; northward_wind;
// U component of wind; eastward_wind;
void WindQuery::convertDataset(const GribDatasetVector& datasets)
{
  for(int dsidx = 0; dsidx < datasets.size(); dsidx += 2)
  {
    const GribDataset& datasetUWind = datasets.at(dsidx);
    const GribDataset& datasetVWind = datasets.at(dsidx + 1);

    // Need parametes ordered by U and V
    if(datasetUWind.getParameterType() == atools::grib::U_WIND &&
       datasetVWind.getParameterType() == atools::grib::V_WIND)
    {
      const QVector<float>& dataU = datasetUWind.getData();
      const QVector<float>& dataV = datasetVWind.getData();
      WindAltLayer layer;
      layer.altitude = datasetUWind.getAltFeetRounded();

      for(int j = 0; j < 181; j++) // y
      {
        for(int i = 0; i < 360; i++) // x
        {
          int i2 = i + 1;
          if(i2 >= 360) // 359 is last
            i2 = 0;

          float u = dataU.at(i + j * 360);
          float v = dataV.at(i + j * 360);
          Wind wind;
          wind.speed = atools::geo::windSpeedFromUV(u, v);
          wind.dir = normalizeCourse(atools::geo::windDirectionFromUV(u, v));
          layer.winds.append(wind);
        }
      }
      windLayers.insert(layer.altitude, layer);
    }
    else
      throw atools::Exception("Invalid dataset order for  U and V wind component");
  }
}

} // namespace grib
} // namespace atools
