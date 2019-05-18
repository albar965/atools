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
using atools::geo::windSpeedFromUV;
using atools::geo::windDirectionFromUV;
using atools::geo::interpolateWindDir;

namespace atools {
namespace grib {

/* One grid cell with all wind values at the corners for interpolation. top left corresponds to queried position. */
struct WindRect
{
  Wind topLeft, topRight, bottomRight, bottomLeft;
};

/* One grid cell with all grid positions values at the corners for interpolation. top left corresponds to queried position. */
struct GridRect
{
  QPoint topLeft, topRight, bottomRight, bottomLeft;
};

// Debug IO =======================================================================
QDebug operator<<(QDebug out, const WindPos& windPos)
{
  QDebugStateSaver saver(out);

  out.noquote().nospace() << "WindPos(" << windPos.wind << ", " << windPos.pos << ")";

  return out;
}

QDebug operator<<(QDebug out, const Wind& wind)
{
  QDebugStateSaver saver(out);

  out.noquote().nospace() << "Wind(speed " << wind.speed << ", dir " << wind.dir << ")";

  return out;
}

/* Interpolate wind speed and direction between two altitude layers */
Wind interpolateWind(Wind w0, Wind w1, float alt0, float alt1, float alt)
{
  Wind w;
  w.dir = interpolateWindDir(w0.dir, w1.dir, alt0, alt1, alt);
  w.speed = interpolate(w0.speed, w1.speed, alt0, alt1, alt);
  if(atools::almostEqual(w.speed, 0.f))
    w.dir = 0.f;
  return w;
}

/* Column number in grid */
inline int colNum(const atools::geo::Pos& pos)
{
  return static_cast<int>(pos.getLonX() < 0.f ? 360.f + pos.getLonX() : pos.getLonX());
}

/* Row number in grid */
inline int rowNum(const atools::geo::Pos& pos)
{
  return static_cast<int>(180.f - (pos.getLatY() + 90.f)) + (pos.getLonX() < 0.f ? 1 : 0);
}

/* Get colum and row number for position at one degree boundary. */
inline QPoint gridPos(const atools::geo::Pos& pos)
{
  return QPoint(colNum(pos), rowNum(pos));
}

/* A rectangle at one degree cell boundaries covering the given postion */
inline atools::geo::Rect globalRect(const atools::geo::Pos& pos)
{
  atools::geo::Rect rect;
  rect.setNorth(std::ceil(pos.getLatY()));
  rect.setWest(std::floor(pos.getLonX()));
  rect.setSouth(std::floor(pos.getLatY()));
  rect.setEast(std::ceil(pos.getLonX()));

  // Return at least a one degree width or height
  if(atools::almostEqual(rect.getWidthDegree(), 0.f))
    rect.setEast(rect.getEast() + 1.f);
  if(atools::almostEqual(rect.getHeightMeter(), 0.f))
    rect.setSouth(rect.getNorth() - 1.f);

  return rect;
}

/* Gives a rectangle for grid positions for the given world coordinate rect */
inline GridRect gridRect(const atools::geo::Rect& rect)
{
  GridRect retval;
  retval.topLeft = gridPos(rect.getTopLeft());
  retval.topRight = gridPos(rect.getTopRight());
  retval.bottomRight = gridPos(rect.getBottomRight());
  retval.bottomLeft = gridPos(rect.getBottomLeft());
  return retval;
}

// ===============================================================
WindQuery::WindQuery(QObject *parentObject, bool logVerbose)
  : QObject(parentObject), verbose(logVerbose)
{
  downloader = new GribDownloader(parentObject, logVerbose);

  // Download or read U and V components of wind - will be used to calculated speed and direction
  downloader->setParameters(PARAMETERS);

  // Layers 80 meters (260 ft) AGL and other values are mbar layers
  downloader->setSurfaces(SURFACES);

  connect(downloader, &GribDownloader::gribDownloadFinished, this, &WindQuery::gribDownloadFinished);
  connect(downloader, &GribDownloader::gribDownloadFailed, this, &WindQuery::gribDownloadFailed);

  // Set up file watcher for file based updates
  fileWatcher = new atools::util::FileSystemWatcher(parentObject, logVerbose);
  fileWatcher->setMinFileSize(180000);
  connect(fileWatcher, &atools::util::FileSystemWatcher::fileUpdated, this, &WindQuery::gribFileUpdated);
}

WindQuery::~WindQuery()
{
  delete downloader;
  delete fileWatcher;
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
}

void WindQuery::initFromData(const QByteArray& data)
{
  deinit();

  GribReader reader(verbose);
  reader.readData(data);
  convertDataset(reader.getDatasets());
}

void WindQuery::deinit()
{
  windLayers.clear();
  downloader->stopDownload();
  fileWatcher->stopWatching();
}

Wind WindQuery::getWindForPos(const Pos& pos, bool interpolateValue) const
{
  // Calculate grid position
  QPoint gPos = gridPos(pos);

  qDebug() << Q_FUNC_INFO << pos << gPos;

  // Get next layers below and above altitude
  WindAltLayer lower, upper;
  layersByAlt(lower, upper, pos.getAltitude());

  if(!interpolateValue || pos.nearGrid())
  {
    // No need to interpolate within grid - use position as is
    Wind lW = windForLayer(lower, gPos);

    if(upper != lower)
    {
      Wind uW = windForLayer(upper, gPos);
      // Interpolate between upper and lower layer
      return interpolateWind(lW, uW, lower.altitude, upper.altitude, pos.getAltitude());
    }
    return lW;
  }
  else
  {
    Rect global = globalRect(pos);
    GridRect grid = gridRect(global);

    // Interpolate wind within a grid rectangle for the lower layer
    WindRect windRectLower;
    windRectForLayer(windRectLower, lower, grid);
    Wind lWind = interpolateRect(windRectLower, global, pos);

    if(upper != lower)
    {
      // Interpolate wind within a grid rectangle for the upper layer if layers differ
      WindRect windRectUpper;
      windRectForLayer(windRectUpper, upper, grid);
      Wind uWind = interpolateRect(windRectUpper, global, pos);

      // Interpolate between upper and lower wind
      return interpolateWind(lWind, uWind, lower.altitude, upper.altitude, pos.getAltitude());
    }
    return lWind;
  }
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
    wp.wind = getWindForPos(rect.getTopLeft().alt(altFeet)); // Set altitude into pos
    wp.pos = rect.getTopLeft();
    result.append(wp);
  }
  else
  {
    // Split rectangle if it crosses the anti-metidian (date line)
    for(const atools::geo::Rect& r : rect.splitAtAntiMeridian())
    {
      // Get next layers below and above altitude
      WindAltLayer lower, upper;
      layersByAlt(lower, upper, altFeet);

      for(float lonx = std::floor(r.getWest()); lonx <= r.getEast(); lonx += 1.f)
      {
        for(float laty = std::ceil(r.getNorth()); laty >= r.getSouth(); laty -= 1.f)
        {
          Pos cell(lonx, laty);
          QPoint gPos = gridPos(cell);

          WindPos wp;
          // Calcualte grid cell upper left position (position for value)
          wp.pos = cell;
          wp.pos.setAltitude(altFeet);

          if(upper != lower)
          {
            Wind lWind = windForLayer(lower, gPos);
            Wind uWind = windForLayer(upper, gPos);

            // Interpolate wind within a grid rectangle for the upper layer if layers differ
            wp.wind = interpolateWind(lWind, uWind, lower.altitude, upper.altitude, altFeet);
          }
          else
          {
            Wind lWind = windForLayer(lower, gPos);

            float dir = lWind.dir;
            wp.wind.dir = dir < INVALID_DIR_VALUE / 2 ? normalizeCourse(dir) : 0.f;
            wp.wind.speed = lWind.speed;
          }
          result.append(wp);
        }
      }
    }
  }
}

Wind WindQuery::getWindAverageForLineString(const geo::LineString& linestring) const
{
  if(linestring.size() == 1)
    // Only one position
    return getWindForPos(linestring.getPos1());
  else if(linestring.size() == 2)
    // Only one line
    return getWindAverageForLine(linestring.at(0), linestring.at(1));
  else
  {
    // Sum up values
    double speed = 0., dir = 0.;
    for(int i = 0; i < linestring.size() - 1; i++)
    {
      Wind w = getWindAverageForLine(linestring.at(i), linestring.at(i + 1));
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

Wind WindQuery::getWindAverageForLine(const Line& line) const
{
  return getWindAverageForLine(line.getPos1(), line.getPos2());
}

Wind WindQuery::getWindAverageForLine(const Pos& pos1, const Pos& pos2) const
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
    pos1.interpolatePointsAlt(pos2, distanceMeter, numPoints, positions);
    positions.append(pos2);
  }
  else
    // Only start and end needed
    positions << pos1 << pos2;

  Wind wind = {0.f, 0.f};
  WindRect windRectLower, windRectUpper;

  for(const atools::geo::Pos& pos : positions)
  {
    Rect global = globalRect(pos);
    GridRect grid = gridRect(global);

    // Get next layers below and above altitude
    WindAltLayer lower, upper;
    layersByAlt(lower, upper, pos.getAltitude());

    // Get interpolated wind in grid cell at lower layer
    windRectForLayer(windRectLower, lower, grid);
    Wind lW = interpolateRect(windRectLower, global, pos);

    Wind w;
    if(upper != lower)
    {
      // Get interpolated wind in grid cell at upper layer
      windRectForLayer(windRectUpper, upper, grid);
      Wind uW = interpolateRect(windRectUpper, global, pos);

      // Interpolate at altitude between layers
      w = interpolateWind(lW, uW, lower.altitude, upper.altitude, pos.getAltitude());
    }
    else
    {
      w.speed = lW.speed;
      w.dir = lW.dir < INVALID_DIR_VALUE / 2 ? lW.dir : 0.f;
    }
    // qDebug() << pos << "s" << s << "d" << d;
    wind.speed += w.speed;
    wind.dir += w.dir;
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

Wind WindQuery::interpolateRect(const WindRect& windRect, const atools::geo::Rect& rect, const geo::Pos& pos) const
{
  float left = rect.getWest(), right = rect.getEast(), top = rect.getNorth(), bottom = rect.getSouth();

  // Interpolate at top and bottom between left and right
  Wind wTop, wBottom;
  wTop = interpolateWind(windRect.topLeft, windRect.topRight, left, right, pos.getLonX());
  wBottom = interpolateWind(windRect.bottomLeft, windRect.bottomRight, left, right, pos.getLonX());

  // Interpolate between top and bottom
  return interpolateWind(wTop, wBottom, top, bottom, pos.getLatY());
}

void WindQuery::windRectForLayer(WindRect& windRect, const WindAltLayer& layer, const GridRect& rect) const
{
  windRect.topLeft = windForLayer(layer, rect.topLeft);
  windRect.topRight = windForLayer(layer, rect.topRight);
  windRect.bottomRight = windForLayer(layer, rect.bottomRight);
  windRect.bottomLeft = windForLayer(layer, rect.bottomLeft);
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
  windLayers.clear();

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
          float u = dataU.at(i + j * 360);
          float v = dataV.at(i + j * 360);
          Wind wind;
          wind.speed = atools::geo::meterPerSecToKnots(atools::geo::windSpeedFromUV(u, v));
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
