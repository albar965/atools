/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_METARINDEX_H
#define ATOOLS_METARINDEX_H

#include "fs/weather/weathertypes.h"

#include <functional>

#include <QVector>

class QTextStream;

class QDateTime;

namespace atools {
namespace geo {
class Pos;
template<typename TYPE>
class SpatialIndex;
}

namespace fs {
namespace weather {

class PosIndex;
class Metar;

/*
 * Reads, caches and indexes (by position) METAR reports in NOAA style as also used by X-Plane.
 * Can also read flat, plain text METAR files like they are provided by IVAO or VATSIM.
 *
 * Example for XPLANE or NOAA:
 *
 * 2017/07/30 18:45
 * KHYI 301845Z 13007KT 070V130 10SM SCT075 38/17 A2996
 *
 * 2017/07/30 18:55
 * KPRO 301855Z AUTO 11003KT 10SM CLR 26/14 A3022 RMK AO2 T02570135
 *
 * 2017/07/30 18:47
 * KADS 301847Z 06005G14KT 13SM SKC 32/19 A3007
 *
 * Example for format FLAT:
 *
 * KC99 100906Z AUTO 30022G42KT 10SM CLR M01/M04 A3035 RMK AO2
 * LCEN 100920Z 16004KT 090V230 CAVOK 31/10 Q1010 NOSIG
 */
class MetarIndex
{
public:
  MetarIndex(atools::fs::weather::MetarFormat formatParam, bool verboseLogging = false);
  ~MetarIndex();

  MetarIndex(const MetarIndex& other) = delete;
  MetarIndex& operator=(const MetarIndex& other) = delete;

  /* Read METARs from stream and add them to the index. Merges into current list or clears list before.
   * Older of duplicates are ignored/removed.
   * Returns number of METARs read. */
  int read(QTextStream& stream, const QString& fileName, bool merge);
  int read(const QString& filename, bool merge);

  /* Clears all lists */
  void clear();
  void clearCache();

  /* true if nothing was read */
  bool isEmpty() const;

  /* Number of unique airport idents in index */
  int numStationMetars() const;

  /* Get METAR information for station, nearest or interpolated.
   * - Station will be saved as request ident if given. Only interpolated and/or nearest are returned if station is not given.
   * - Nearest is returned if no station can be found.
   * - Interpolated is returned if no station found and nearest is not close to pos.
   * Position and ident of original request are kept.*/
  const Metar& getMetar(const QString& station, atools::geo::Pos pos);

  /* Set to a function that returns the coordinates for an airport ident. Needed to find the nearest if no position is given. */
  void setFetchAirportCoords(const std::function<atools::geo::Pos(const QString&)>& value)
  {
    fetchAirportCoords = value;
  }

  /* Maximum number of nearest airports fetched for interpolation */
  void setNumInterpolation(int value)
  {
    numInterpolation = value;
  }

  /* Maximum distance for nearest airports fetched for interpolation */
  void setMaxDistanceInterpolationNm(float value)
  {
    maxDistanceInterpolationNm = value;
  }

  /* Maximum number of interpolated METARs in cache */
  void setMaxInterpolatedCacheSize(int value)
  {
    maxInterpolatedCacheSize = value;
  }

  /* Maximum distance to interpolated cache to use it */
  void setMaxDistanceToleranceMeter(float value)
  {
    maxDistanceToleranceMeter = value;
  }

private:
  /* Get a METAR string. Empty if not available */
  const atools::fs::weather::Metar& fetchMetar(const QString& ident) const;

  /* Read NOAA or XPLANE format */
  int readNoaaXplane(QTextStream& stream, const QString& fileOrUrl, bool merge);

  /* Read flat file format like VATSIM */
  int readFlat(QTextStream& stream, const QString& fileOrUrl, bool merge);

  /* Read JSON file format from IVAO */
  int readJson(QTextStream& stream, const QString& fileOrUrl, bool merge);

  /* Copy airports from the complete list to the index with coordinates.
   * Copies only airports that exist in the current simulator database, i.e. where fetchAirportCoords returns
   * a valid coordinate. */
  void updateIndex();

  /* Update or insert a METAR entry */
  void updateOrInsert(const QString& metarString, const QString& ident, const QDateTime& lastTimestamp);

  /* Callback to get airport coodinates by ICAO ident */
  std::function<atools::geo::Pos(const QString&)> fetchAirportCoords;

  /* Map containing all loaded METARs airport idents mapped to the position in metarVector */
  QHash<QString, int> identIndexMap;

  /* Index containing all stations which could be resolved to a coordinate. */
  atools::geo::SpatialIndex<PosIndex> *spatialIndex = nullptr, *spatialIndexInterpolated = nullptr;
  QVector<atools::fs::weather::Metar> metarVector, metarInterpolatedVector;

  int maxInterpolatedCacheSize = 40000;
  float maxDistanceToleranceMeter = 100.f;

  int numInterpolation = 8;
  float maxDistanceInterpolationNm = 200.f;

  bool verbose = false;
  atools::fs::weather::MetarFormat format = atools::fs::weather::UNKNOWN;

  // Block fetching METARs while reading files. Can happen if input events are processes while reading.
  bool reading = false;
};

} // namespace weather
} // namespace fs
} // namespace atools

#endif // ATOOLS_METARINDEX_H
