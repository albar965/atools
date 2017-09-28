/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_FS_FSUTIL_H
#define ATOOLS_FS_FSUTIL_H

#include <QString>

namespace atools {
namespace fs {
namespace util {

/* Get the aircraft type name for the ICAO code */
QString aircraftTypeForCode(const QString& code);

/* Maximum rating is 5 */
const int MAX_RATING = 5;

/* Calculate for FSX/P3D based on airport facilities */
int calculateAirportRating(bool isAddon, bool hasTower, int numTaxiPaths, int numParkings, int numAprons);

/* Calculate for X-Plane based on airport facilities */
int calculateAirportRatingXp(bool isAddon, bool is3D, bool hasTower, int numTaxiPaths, int numParkings, int numAprons);

/* Check the airport name if it contains military designators */
bool isNameMilitary(const QString& airportName);

/* Check if the name contains a closed identifier */
bool isNameClosed(const QString& airportName);

/* Capitalize any string ignoring aviation acronyms */
QString capNavString(const QString& str);

/* Capitalize airport name making special designators (AFB, ...) upper case */
QString capAirportName(const QString& str);

} // namespace util
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_FSUTIL_H
