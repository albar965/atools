/*****************************************************************************
* Copyright 2015-2023 Alexander Barthel alex@littlenavmap.org
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

#include "atools.h"
#include "exception.h"
#include "geo/calculations.h"
#include "geo/line.h"
#include "geo/linestring.h"
#include "geo/rect.h"
#include "grib/gribdownloader.h"
#include "grib/gribreader.h"
#include "util/filesystemwatcher.h"
#include "fs/util/fsutil.h"

#include <QDir>

using atools::grib::GribDownloader;
using atools::geo::Rect;
using atools::geo::Pos;
using atools::geo::Line;
using atools::geo::LineString;
using atools::geo::normalizeCourse;
using atools::interpolate;
using atools::geo::windSpeedFromUV;
using atools::geo::windUComponent;
using atools::geo::windVComponent;
using atools::geo::windDirectionFromUV;

namespace atools {
namespace grib {

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
using Qt::endl;
#endif

/* Allowed altitude inaccuracy when comparing layer altitudes. */
Q_CONSTEXPR static int ALTITUDE_EPSILON = 50.f;

struct WindData
{
  /* Use U and V components for calculation. */
  float u, v;

  /* Convert to wind with speed and direction */
  Wind toWind() const
  {
    return {atools::geo::windDirectionFromUV(u, v), atools::geo::windSpeedFromUV(u, v)};
  }

  void init()
  {
    u = v = 0.;
  }

  bool operator==(const WindData& other) const
  {
    return atools::almostEqual(u, other.u) && atools::almostEqual(v, other.v);
  }

  bool operator!=(const WindData& other) const
  {
    return !operator==(other);
  }

};

const static atools::grib::WindData EMPTY_WIND_DATA = {0.f, 0.f};

/* Internal data structure for wind direction and speed computed from U/V speeds */
struct WindAltLayer
{
  int altitude = 0;
  QVector<WindData> winds;
  float surface;

  bool operator<(const WindAltLayer& l) const
  {
    return altitude < l.altitude;
  }

  bool operator==(const WindAltLayer& l) const
  {
    return atools::almostEqual(altitude, l.altitude, ALTITUDE_EPSILON);
  }

  bool operator!=(const WindAltLayer& l) const
  {
    return !(*this == l);
  }

  bool isValid() const
  {
    return !winds.isEmpty();
  }

};

/* One grid cell with all wind values at the corners for interpolation. top left corresponds to queried position. */
struct WindRect
{
  WindData topLeft, topRight, bottomRight, bottomLeft;
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

/* Column number in grid */
inline int colNum(const atools::geo::Pos& pos)
{
  return static_cast<int>(pos.getLonX() < 0.f ? 360.f + pos.getLonX() : pos.getLonX());
}

/* Row number in grid */
inline int rowNum(const atools::geo::Pos& pos)
{
  return static_cast<int>(180.f - (pos.getLatY() + 90.f));
}

/* Get colum and row number for position at one degree boundary. */
inline QPoint gridPos(const atools::geo::Pos& pos)
{
  return QPoint(colNum(pos), rowNum(pos));
}

/* A rectangle at one degree cell boundaries covering the given position */
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
  connect(downloader, &GribDownloader::gribDownloadSslErrors, this, &WindQuery::windDownloadSslErrors);
  connect(downloader, &GribDownloader::gribDownloadProgress, this, &WindQuery::windDownloadProgress);

  // Set up file watcher for file based updates
  fileWatcher = new atools::util::FileSystemWatcher(parentObject, logVerbose);
  fileWatcher->setMinFileSize(180000); // Do not accept smaller files
  fileWatcher->setDelayMs(10000); // Delay notification for 10 seconds to avoid incomplete files
  connect(fileWatcher, &atools::util::FileSystemWatcher::dirUpdated, this, &WindQuery::gribDirUpdated);
  connect(fileWatcher, &atools::util::FileSystemWatcher::filesUpdated, this, &WindQuery::gribFileUpdated);
}

WindQuery::~WindQuery()
{
  delete downloader;
  delete fileWatcher;
}

void WindQuery::initFromUrl(const QString& baseUrl)
{
  deinit();

  if(!baseUrl.isEmpty())
    // Start download and conversion when done
    downloader->startDownload(QDateTime(), baseUrl);
  else
    qWarning() << Q_FUNC_INFO << "URL is empty. Wind download suspended.";
}

void WindQuery::initFromPath(const QString& path, atools::fs::weather::XpWeatherType type)
{
  deinit();

  weatherPath = path;
  weatherType = type;
  currentGribFile = collectGribFiles();

  if(verbose)
    qDebug() << Q_FUNC_INFO << weatherPath << currentGribFile;

  // Inital load
  gribFileUpdated({currentGribFile});

  if(currentGribFile.isEmpty() && weatherType == atools::fs::weather::WEATHER_XP12)
    // Watch folder if file not given
    fileWatcher->setFilenameAndStart(weatherPath);
  else
    // Start checking file and parent folder for changes
    fileWatcher->setFilenameAndStart(currentGribFile);
}

QString WindQuery::collectGribFiles()
{
  QFileInfoList gribFiles;
  if(weatherType == atools::fs::weather::WEATHER_XP11)
    // global_winds.grib
    gribFiles.append(QFileInfo(weatherPath));
  else if(weatherType == atools::fs::weather::WEATHER_XP12)
  {
    // GRIB-2022-9-6-21.00-ZULU-wind.grib
    // GRIB-2023-02-22-18.00-ZULU-wind-v2.grib
    QDir weatherDir(weatherPath);
    weatherDir.setSorting(QDir::Unsorted);
    weatherDir.setFilter(QDir::Files | QDir::NoDotAndDotDot);
    weatherDir.setNameFilters({"GRIB-*-wind-v*.grib", "GRIB-*-wind.grib"});

    for(QFileInfo entry : weatherDir.entryInfoList())
      gribFiles.append(entry);

    // Sort by timestamp - put latest at begin of list
    if(gribFiles.size() > 1)
      std::sort(gribFiles.begin(), gribFiles.end(), [](const QFileInfo& file1, const QFileInfo& file2)->bool {
            return atools::fs::util::xpGribFilenameToDate(file1.fileName()) > atools::fs::util::xpGribFilenameToDate(file2.fileName());
          });
  }

  if(verbose)
    qDebug() << Q_FUNC_INFO << gribFiles;

  // Return first and latest file since GRIB cannot be merged like METAR
  if(!gribFiles.isEmpty())
    return gribFiles.constFirst().absoluteFilePath();
  else
    return QString();
}

void WindQuery::initFromFixedModel(float dir, float speed, float altitude)
{
  // Zero wind at zero altitude as lower layer
  initFromFixedModel(0.f, 0.f, 0.f, dir, speed, altitude);
}

void WindQuery::initFromFixedModel(float dirLower, float speedLower, float altitudeLower,
                                   float dirUpper, float speedUpper, float altitudeUpper)
{
  deinit();

  // Add lower layer ==========================
  WindAltLayer groundLayer;
  groundLayer.altitude = roundToInt(altitudeLower);
  groundLayer.winds.fill(WindData{windUComponent(speedLower, dirLower),
                                  windVComponent(speedLower, dirLower)}, 360 * 181);
  windLayers.insert(atools::roundToInt(groundLayer.altitude), groundLayer);

  // Add upper layer ==========================
  WindAltLayer altLayer;
  altLayer.altitude = roundToInt(altitudeUpper);
  altLayer.winds.fill(WindData{windUComponent(speedUpper, dirUpper),
                               windVComponent(speedUpper, dirUpper)}, 360 * 181);
  windLayers.insert(atools::roundToInt(altLayer.altitude), altLayer);
}

void WindQuery::deinit()
{
  analyisTime = QDateTime();
  windLayers.clear();
  downloader->stopDownload();
  fileWatcher->stopWatching();
  weatherPath.clear();
  currentGribFile.clear();
}

Wind WindQuery::getWindForPos(atools::geo::Pos pos, bool interpolateValue) const
{
  pos.normalize();

  if(!pos.isValid())
  {
    qWarning() << Q_FUNC_INFO << "invalid pos";
    return EMPTY_WIND;
  }
  // Calculate grid position
  QPoint gPos = gridPos(pos);

  if(verbose)
    qDebug() << Q_FUNC_INFO << pos << gPos;

  // Get next layers below and above altitude
  WindAltLayer lower, upper;
  layersByAlt(lower, upper, pos.getAltitude());

  if(!interpolateValue || pos.nearGrid(1.f, atools::geo::Pos::POS_EPSILON_500M))
  {
    // No need to interpolate within grid - use position as is
    WindData lW = windForLayer(lower, gPos);

    if(upper != lower)
    {
      WindData uW = windForLayer(upper, gPos);
      // Interpolate between upper and lower layer
      return interpolateWind(lW, uW, lower.altitude, upper.altitude, pos.getAltitude()).toWind();
    }
    else
      return lW.toWind();
  }
  else
  {
    Rect global = globalRect(pos);
    GridRect grid = gridRect(global);

    // Interpolate wind within a grid rectangle for the lower layer
    WindRect windRectLower;
    windRectForLayer(windRectLower, lower, grid);
    WindData lWind = interpolateRect(windRectLower, global, pos);

    if(upper != lower)
    {
      // Interpolate wind within a grid rectangle for the upper layer if layers differ
      WindRect windRectUpper;
      windRectForLayer(windRectUpper, upper, grid);
      WindData uWind = interpolateRect(windRectUpper, global, pos);

      // Interpolate between upper and lower wind
      return interpolateWind(lWind, uWind, lower.altitude, upper.altitude, pos.getAltitude()).toWind();
    }
    else
      return lWind.toWind();
  }
}

WindPosList WindQuery::getWindForRect(const atools::geo::Rect& rect, float altFeet) const
{
  WindPosList result;
  getWindForRect(result, rect, altFeet, 1);
  return result;
}

void WindQuery::getWindForRect(WindPosList& result, atools::geo::Rect rect, float altFeet, int gridSpacing) const
{
  if(windLayers.isEmpty())
    return;

  rect.normalize();

  if(rect.isPoint(atools::geo::Pos::POS_EPSILON_100M))
  {
    // No need to interpolate for single point and very small rectangles
    WindPos wp;
    wp.wind = getWindForPos(rect.getCenter().alt(altFeet)); // Set altitude into pos
    wp.pos = rect.getTopLeft();
    result.append(wp);
  }
  else
  {
    // Get next layers below and above altitude
    WindAltLayer lower, upper;
    layersByAlt(lower, upper, altFeet);

    // Split rectangle if it crosses the anti-meridian (date line)
    for(const atools::geo::Rect& r : rect.splitAtAntiMeridian())
    {
      for(float lonx = std::floor(r.getWest()); lonx <= r.getEast(); lonx += 1.f)
      {
        for(float laty = std::ceil(r.getNorth()); laty >= r.getSouth(); laty -= 1.f)
        {
          Pos cell(lonx, laty);
          if(gridSpacing > 1 && !cell.nearGrid(gridSpacing))
            continue;

          QPoint gPos = gridPos(cell);

          WindPos wp;
          // Calculate grid cell upper left position (position for value)
          wp.pos = cell;
          wp.pos.setAltitude(altFeet);

          if(upper != lower)
            // Interpolate wind within a grid rectangle for the upper layer if layers differ
            wp.wind = interpolateWind(windForLayer(lower, gPos), windForLayer(upper, gPos),
                                      lower.altitude, upper.altitude, altFeet).toWind();
          else
            wp.wind = windForLayer(lower, gPos).toWind();

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
    return getWindAverageForLine(linestring.toLine());
  else
  {
    WindData windData = EMPTY_WIND_DATA;
    // Sum up values
    for(int i = 0; i < linestring.size() - 1; i++)
    {
      WindData wd = windAverageForLine(linestring.at(i), linestring.at(i + 1));
      windData.u += wd.u;
      windData.v += wd.v;
    }

    // Calculate average
    windData.u = windData.u / (linestring.size() - 1);
    windData.v = windData.v / (linestring.size() - 1);

    return windData.toWind();
  }
}

QString WindQuery::getDebug(const geo::Pos& pos) const
{
  QString retval;
  QTextStream out(&retval);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  out.setCodec("UTF-8");
#endif
  out.setRealNumberPrecision(2);
  out.setRealNumberNotation(QTextStream::FixedNotation);
  out << "=================" << endl;
  for(auto it = windLayers.begin(); it != windLayers.end(); ++it)
  {
    WindAltLayer layer = it.value();
    QPoint grid = gridPos(pos);
    WindData wind = windForLayer(layer, grid);

    out << "altitude " << it.key() << " surface " << layer.surface
        << " grid x " << grid.x() << " y " << grid.y() << endl;
    out << "wind u " << wind.u << " v " << wind.v << " kts "
        << " dir " << windDirectionFromUV(wind.u, wind.v) << " deg T"
        << " speed " << windSpeedFromUV(wind.u, wind.v) << " kts" << endl;
    out << "-----------" << endl;
  }
  return retval;
}

void WindQuery::setIgnoreSslErrors(bool value)
{
  downloader->setIgnoreSslErrors(value);
}

void WindQuery::getValidity(QDateTime& from, QDateTime& to) const
{
  from = analyisTime;
  to = analyisTime.addSecs(3600 * 6);
}

void WindQuery::debugDumpContainerSizes() const
{
  if(downloader != nullptr)
    downloader->debugDumpContainerSizes();
}

WindData WindQuery::windForLayer(const WindAltLayer& layer, const QPoint& point) const
{
  return layer.isValid() ? layer.winds.at(point.x() + point.y() * 360) : EMPTY_WIND_DATA;
}

Wind WindQuery::getWindAverageForLine(const Line& line) const
{
  return getWindAverageForLine(line.getPos1(), line.getPos2());
}

Wind WindQuery::getWindAverageForLine(const Pos& pos1, const Pos& pos2) const
{
  return windAverageForLine(pos1, pos2).toWind();
}

WindData WindQuery::windAverageForLine(geo::Pos pos1, geo::Pos pos2) const
{
  WindData windData = EMPTY_WIND_DATA;

  if(!pos1.isValid())
  {
    qWarning() << Q_FUNC_INFO << "invalid pos1";
    return windData;
  }

  if(!pos2.isValid())
  {
    qWarning() << Q_FUNC_INFO << "invalid pos2";
    return windData;
  }

  pos1.normalize();
  pos2.normalize();

  // Calculate the number of samples for a line. Roughly four samples per degree.
  float distanceMeter = pos1.distanceMeterTo(pos2);
  float meterPerDeg = atools::geo::nmToMeter(60.f);
  float meterPerSample = meterPerDeg / samplesPerDegree;
  int numPoints = atools::roundToInt(distanceMeter / meterPerSample);

  LineString positions;

  if(numPoints > 1)
  {
    // Calculate number of points - will include origin but not pos2
    pos1.interpolatePointsAlt(pos2, distanceMeter, numPoints, positions);
    positions.append(pos2);
  }
  else
    // Only start and end needed
    positions << pos1 << pos2;

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
    WindData lW = interpolateRect(windRectLower, global, pos);

    WindData w;
    if(upper != lower)
    {
      // Get interpolated wind in grid cell at upper layer
      windRectForLayer(windRectUpper, upper, grid);

      // Interpolate at altitude between layers
      w = interpolateWind(lW, interpolateRect(windRectUpper, global, pos),
                          lower.altitude, upper.altitude, pos.getAltitude());
    }
    else
      w = lW;

    windData.u += w.u;
    windData.v += w.v;
  }

  windData.u /= positions.size();
  windData.v /= positions.size();
  return windData;
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
    QMap<int, WindAltLayer>::const_iterator it = windLayers.lowerBound(atools::roundToInt(altitude));
    if(it != windLayers.end())
    {
      if(atools::almostEqual(it->altitude, atools::roundToInt(altitude), ALTITUDE_EPSILON))
        // Layer is at requested altitude - no need to interpolate
        lower = upper = *it;
      else if(it == windLayers.begin())
      {
        // First layer - add a zero wind layer for interpolation between layer and ground
        upper = *it;

        lower.altitude = 0.f;
        lower.winds.fill(EMPTY_WIND_DATA, 360 * 181);
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

WindData WindQuery::interpolateRect(const WindRect& windRect, const atools::geo::Rect& rect, const geo::Pos& pos) const
{
  float left = rect.getWest(), right = rect.getEast(), top = rect.getNorth(), bottom = rect.getSouth();

  // Interpolate at top and bottom between left and right
  WindData wTop = interpolateWind(windRect.topLeft, windRect.topRight, left, right, pos.getLonX());
  WindData wBottom = interpolateWind(windRect.bottomLeft, windRect.bottomRight, left, right, pos.getLonX());

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

void WindQuery::gribDownloadFinished(const GribDatasetVector& datasets, QString downloadUrl)
{
  qDebug() << Q_FUNC_INFO << downloadUrl;

  // Download finished - update coordinates
  convertDataset(datasets);
  emit windDataUpdated();
}

void WindQuery::gribDownloadFailed(const QString& error, int errorCode, QString downloadUrl)
{
  qWarning() << Q_FUNC_INFO << error << downloadUrl;
  emit windDownloadFailed(error, errorCode);
}

void WindQuery::gribDirUpdated(const QString& dir)
{
  if(verbose)
    qDebug() << Q_FUNC_INFO << dir;

  QString gribFile = collectGribFiles();
  if(gribFile != currentGribFile)
  {
    // Read file(s) again if updated and the list has changed
    currentGribFile = gribFile;
    gribFileUpdated({currentGribFile});
  }
}

void WindQuery::gribFileUpdated(const QStringList& filenames)
{
  QString filename = filenames.value(0);

  if(verbose)
    qDebug() << Q_FUNC_INFO << filename;

  if(!filename.isEmpty())
  {
    try
    {
      GribReader reader(verbose);
      reader.readFile(filename);
      convertDataset(reader.getDatasets());
    }
    catch(atools::Exception& e)
    {
      emit windDownloadFailed(e.getMessage(), 0);
      return;
    }
    catch(...)
    {
      emit windDownloadFailed(tr("Unknown error."), 0);
      return;
    }

    emit windDataUpdated();
  }
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
// Ni - number of points along a parallel - 360
// Nj - number of points along a meridian - 181
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
      if(datasetUWind.getDatetime().isValid())
        analyisTime = datasetUWind.getDatetime();

      const QVector<float>& dataU = datasetUWind.getData();
      const QVector<float>& dataV = datasetVWind.getData();
      WindAltLayer layer;
      layer.altitude = roundToInt(datasetUWind.getAltFeetRounded());
      layer.surface = datasetUWind.getSurface();

      for(int j = 0; j < 181; j++) // y
      {
        for(int i = 0; i < 360; i++) // x
        {
          WindData wind;
          wind.u = atools::geo::meterPerSecToKnots(dataU.at(i + j * 360));
          wind.v = atools::geo::meterPerSecToKnots(dataV.at(i + j * 360));
          layer.winds.append(wind);
        }
      }
      windLayers.insert(atools::roundToInt(layer.altitude), layer);
    }
    else
      throw atools::Exception("Invalid dataset order for  U and V wind component");
  }
}

/* Interpolate wind speed and direction between two altitude layers */
WindData WindQuery::interpolateWind(const WindData& w0, const WindData& w1, float alt0, float alt1, float alt) const
{
  if(w0 == w1)
    return w0;
  else if(atools::almostEqual(alt0, alt))
    return w0;
  else if(atools::almostEqual(alt1, alt))
    return w1;
  else
  {
    WindData w;
    w.u = interpolate(w0.u, w1.u, alt0, alt1, alt);
    w.v = interpolate(w0.v, w1.v, alt0, alt1, alt);
    return w;
  }
}

} // namespace grib
} // namespace atools
