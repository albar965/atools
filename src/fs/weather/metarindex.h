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

#ifndef ATOOLS_METARINDEX_H
#define ATOOLS_METARINDEX_H

#include "geo/simplespatialindex.h"

#include <QDateTime>

namespace atools {
namespace fs {
namespace weather {

struct MetarData
{
  QString ident, metar;
  QDateTime timestamp;

  bool isValid() const
  {
    return !ident.isEmpty();
  }

};
class MetarResult;

/*
 * Downloads caches and indexes (by position) METAR reports in NOAA style as also used by X-Plane.
 * Example:
 *
 * 2017/07/30 18:45
 * KHYI 301845Z 13007KT 070V130 10SM SCT075 38/17 A2996
 *
 * 2017/07/30 18:55
 * KPRO 301855Z AUTO 11003KT 10SM CLR 26/14 A3022 RMK AO2 T02570135
 *
 * 2017/07/30 18:47
 * KADS 301847Z 06005G14KT 13SM SKC 32/19 A3007
 */
class MetarIndex
{
public:
  MetarIndex(int size = 100000, bool verboseLogging = false);
  ~MetarIndex();

  /* Read METARs from stream and add them to the index. Merges into current list or clears list before.
   * Older of duplicates are ignored/removed. */
  bool read(QTextStream& stream, const QString& fileName, bool merge);

  /* Clears all lists */
  void clear();

  /* true if nothing was read */
  bool isEmpty() const;

  /* Get all ICAO codes that have a weather station */
  QSet<QString> getMetarAirportIdents() const;

  /* Get a METAR string. Empty if not available */
  QString getMetar(const QString& ident);

  /* Get METAR information for station, nearest and interpolated.
   * Also keeps position and ident of original request.*/
  atools::fs::weather::MetarResult getMetar(const QString& station, const atools::geo::Pos& pos);

  /* Set to a function that returns the coordinates for an airport ident. Needed to find the nearest. */
  void setFetchAirportCoords(const std::function<atools::geo::Pos(const QString&)>& value)
  {
    fetchAirportCoords = value;
  }

  /* Copy airports from the complete list to the index with coordinates.
   * Copies only airports that exist in the current simulator database, i.e. where fetchAirportCoords returns
   * a valid coordinate. */
  void updateIndex();

private:
  /* Callback to get airport coodinates by ICAO ident */
  std::function<atools::geo::Pos(const QString&)> fetchAirportCoords;

  /* Map containing all found METARs */
  QHash<QString, MetarData> metarMap;

  /* Index containing only stations with a valid position */
  atools::geo::SimpleSpatialIndex<QString, MetarData> *index = nullptr;

  bool verbose = false;
};

} // namespace weather
} // namespace fs
} // namespace atools

#endif // ATOOLS_METARINDEX_H
