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

#ifndef ATOOLS_FS_WEATHERTYPES_H
#define ATOOLS_FS_WEATHERTYPES_H

#include <QHash>

namespace atools {
namespace fs {
namespace weather {

/* type of METAR in class Metar */
enum MetarType
{
  NONE, /* Not set */
  BEST, /* Station, interpolated or nearest depending on availability */
  STATION,
  NEAREST,
  INTERPOLATED
};

/* X-Plane types to read weather files */
enum XpWeatherType
{
  WEATHER_XP_UNKNOWN,
  WEATHER_XP11,
  WEATHER_XP12
};

enum MetarFormat
{
  UNKNOWN,
  NOAA, /* Date time and METAR line separated format */
  XPLANE, /* as NOAA but with X-Plane special keywords */
  FLAT, /* Simple text of "ICAO METAR" strings */
  JSON /* IVAO JSON */
};

/*
 * Test the weather server URL or file synchronously.
 * @param url URL containing a %1 placeholder for the metar
 * @param url an airport ICAO
 * @param result metar if successfull - otherwise error message
 * @return true if successfull
 */
bool testUrl(QStringList& result, const QString& urlStr, const QString& airportIcao,
             const QHash<QString, QString>& headerParameters = QHash<QString, QString>(), int readLines = 6);

} // namespace weather
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_WEATHERTYPES_H
