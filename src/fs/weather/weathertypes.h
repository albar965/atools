/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_FS_WEATHERTYPES_H
#define ATOOLS_FS_WEATHERTYPES_H

#include "geo/pos.h"

#include <QDateTime>

namespace atools {
namespace fs {
namespace weather {

enum MetarFormat
{
  UNKNOWN,
  NOAA, /* Date time and METAR line separated format */
  XPLANE, /* as NOAA but with X-Plane special keywords */
  FLAT /* Simple text of "ICAO METAR" strings */
};

/*
 * Collects METAR information for station, nearest and interpolated values.
 * Also keeps position and ident of original request.
 */
struct MetarResult
{
  QString requestIdent, metarForStation, metarForNearest, metarForInterpolated;
  atools::geo::Pos requestPos;
  QDateTime timestamp;

  /* True if the origin is a simulator request. Currently only FSX/P3D. */
  bool simulator = false;

  void init(const QString& station, const atools::geo::Pos& pos);

  bool isValid() const
  {
    return !requestIdent.isEmpty();
  }

  bool isEmpty() const
  {
    return !isValid() ||
           (metarForStation.isEmpty() && metarForNearest.isEmpty() && metarForInterpolated.isEmpty());
  }

  bool operator==(const atools::fs::weather::MetarResult& other)
  {
    return requestIdent == other.requestIdent &&
           metarForStation == other.metarForStation &&
           metarForNearest == other.metarForNearest &&
           metarForInterpolated == other.metarForInterpolated &&
           requestPos == other.requestPos;
  }

  bool operator!=(const atools::fs::weather::MetarResult& other)
  {
    return !operator==(other);
  }

};

QDebug operator<<(QDebug out, const atools::fs::weather::MetarResult& record);

/*
 * Test the weather server URL or file synchronously.
 * @param url URL containing a %1 placeholder for the metar
 * @param url an airport ICAO
 * @param result metar if successfull - otherwise error message
 * @return true if successfull
 */
bool testUrl(const QString& urlStr, const QString& airportIcao, QStringList& result, int readLines = 6);

} // namespace weather
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_WEATHERTYPES_H
