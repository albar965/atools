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

#ifndef ATOOLS_GRIBWINDQUERY_H
#define ATOOLS_GRIBWINDQUERY_H

#include "grib/gribcommon.h"

#include <QObject>

#include "geo/pos.h"
#include "atools.h"

class QObject;

namespace atools {
namespace geo {
class Rect;
class Line;
class LineString;
}

namespace grib {

class GribDownloader;

/* Combines wind speed and direction */
struct Wind
{
  /* Degrees true and knots */
  float dir, speed;

  bool operator==(const Wind& other) const
  {
    return atools::almostEqual(speed, other.speed) && atools::almostEqual(dir, other.dir);
  }

  bool operator!=(const Wind& other) const
  {
    return !operator==(other);
  }

};

/* Combines wind speed and direction at a position */
struct WindPos
{
  atools::geo::Pos pos;
  Wind wind;

  bool operator==(const WindPos& other) const
  {
    return pos == other.pos && wind == other.wind;
  }

  bool operator!=(const WindPos& other) const
  {
    return !operator==(other);
  }

};

typedef QVector<WindPos> WindPosVector;

/*
 * Takes care for downloading/reading and decoding of GRIB2 wind files. Provides a query API to calculate and interpolate
 * winds for points, rectangles and lines.
 *
 * Wind speed and direction are interpolated for positions in the grid and given altitudes between layers.
 * Fetching data above the highest altitude will use the highest data.
 *
 * Data is updated automatically every 30 minutes.
 *
 * Downloaded/possible sets are:
 * Altitide | Act. pressure | Pressure param | Downloaded | Used by X-Plane
 * 10000 ft | 69.7 mb       | 700            | *          | XP
 * 15000 ft | 57.2 mb       | 550            |            |
 * 20000 ft | 46.6 mb       | 450            | *          |
 * 25000 ft | 37.6 mb       | 350            |            |
 * 30000 ft | 30.1 mb       | 300            | *          |
 * 35000 ft | 23.8 mb       | 250            |            | XP
 * 40000 ft | 18.8 mb       | 200            | *          |
 */
class GribWindQuery
  : public QObject
{
  Q_OBJECT

public:
  GribWindQuery(QObject *parentObject, bool logVerbose);
  ~GribWindQuery();

  /* Initialize download from https://nomads.ncep.noaa.gov/cgi-bin/filter_gfs_1p00.pl
   *  Will download and update every 30 minutes */
  void init();

  /* Read data from file or byte array */
  void initFromFile(const QString& filename);
  void initFromData(const QByteArray& data);

  /* Clear data and stop periodic downloads */
  void deinit();

  /* Get interpolated wind data for single position. Altitude in feet is used from position. */
  Wind getWindForPos(const atools::geo::Pos& pos) const;

  /* Get interpolated wind data for single position and altitude. */
  Wind getWindForPos(const atools::geo::Pos& pos, float altFeet) const;

  /* Get an array of wind data for the given rectangle at the given altitude from the data grid.
   * Data is only interpolated between layers. Result is sorted by y and x coordinates.*/
  void getWindForRect(atools::grib::WindPosVector& result, const atools::geo::Rect& rect, float altFeet) const;
  atools::grib::WindPosVector getWindForRect(const atools::geo::Rect& rect, float altFeet) const;

  /* Get average wind for the great circle line between the two given positions at the given altitude.
   *  Wind data is fetched for a certain number of spots along the line and thus not perfectly accurate. */
  Wind getWindAverageForLine(const atools::geo::Pos& pos1, const atools::geo::Pos& pos2, float altFeet) const;
  Wind getWindAverageForLine(const atools::geo::Line& line, float altFeet) const;
  Wind getWindAverageForLineString(const atools::geo::LineString& linestring, float altFeet) const;

signals:
  /* Download successfully finished. Only for void init(). */
  void windDownloadFinished();

  /* Download failed.  Only for void init(). */
  void windDownloadFailed(const QString& error, int errorCode);

private:
  /* Allowed inaccuracy when comparing layer altitudes. */
  Q_CONSTEXPR static float ALTITUDE_EPSILON = 10.f;

  /* Do roughly four samples per degree when interpolating winds for lines */
  Q_CONSTEXPR static int SAMPLES_PER_DEGREE = 4;
  atools::grib::GribDownloader *downloader = nullptr;
  bool verbose = false;

  /* Internal data structure for wind direction and speed computed from U/V speeds */
  struct WindAltLayer
  {
    float altitude = 0.f;
    QVector<Wind> winds;

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

  /* One grid cell with all wind values at the corners for interpolation */
  struct WindRect
  {
    Wind topLeft, topRight, bottomRight, bottomLeft;
  };

  /* Wind for grid position */
  Wind windForLayer(const WindAltLayer& layer, int col, int row) const
  {
    return layer.winds.at(col + row * 360);
  }

  /* Get layer above and below (or at) altitude */
  void layersByAlt(WindAltLayer& lower, WindAltLayer& upper, float altitude) const;

  /* Fill cell rectangle with wind values at corners */
  void windRectForLayer(WindRect& windRect, const WindAltLayer& layer, int leftCol, int topRow) const;

  /* Quadratic interpolation- Returns wind for position in the grid cell */
  Wind interpolateRect(const WindRect& windRect, const geo::Pos& pos, int leftCol, int topRow) const;

  /* Convert data from U/V components to speed/heading */
  void convertDataset(const atools::grib::GribDatasetVector& datasets);

  void gribDownloadFinished(const atools::grib::GribDatasetVector& datasets, QString);
  void gribDownloadFailed(const QString &error, int errorCode, QString);

  /* Maps rounded altitude to wind layer data. Sorted by altitude. */
  typedef QMap<float, WindAltLayer> LayerMap;

  LayerMap windLayers;

};

QDebug operator<<(QDebug out, const atools::grib::Wind& wind);
QDebug operator<<(QDebug out, const atools::grib::WindPos& windPos);

} // namespace grib
} // namespace atools

Q_DECLARE_METATYPE(atools::grib::Wind);
Q_DECLARE_METATYPE(atools::grib::WindPos);

Q_DECLARE_TYPEINFO(atools::grib::Wind, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(atools::grib::WindPos, Q_PRIMITIVE_TYPE);

#endif // ATOOLS_GRIBWINDQUERY_H
