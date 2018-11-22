/*****************************************************************************
* Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
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

/* Floating point frequency rounded to the next 0.025 MHz value */
float roundComFrequencyF(float frequency);

/* Integer frequency * 1000 rounded to the next 0.025 MHz value */
float roundComFrequency(int frequency)
{
  return roundComFrequencyF(frequency / 1000.f);
}

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

/* Limits ident to upper case characters and digits and trims length to five.
 * Returns N with following number if empty. */
QString adjustIdent(QString ident, int length = 5, int id = -1);

/* Limits region to upper case characters and trims length to 2. Returns "ZZ" if empty. */
QString adjustRegion(QString region);

/* Max 10 characters, digits, space and underscore */
QString adjustFsxUserWpName(QString name, int length = 10);

/* Upper case characters and digits and length between 2 and 5 */
bool isValidIdent(const QString& ident);

/* Upper case characters length equal 2 */
bool isValidRegion(const QString& region);

/*
 * Gets speed and altitude from strings in ICAO route format.
 *  N0490F360
 *  M084F330
 *  Speed
 *  K0800 (800 Kilometers)
 *  N0490 (490 Knots)
 *  M082 (Mach 0.82)
 *  Level/altitude
 *  F340 (Flight level 340)
 *  S1260 (12600 Meters)
 *  A100 (10000 Feet)
 *  M0890 (8900 Meters)
 */
bool extractSpeedAndAltitude(const QString& item, float& speedKnots, float& altFeet, bool *speedOk = nullptr,
                             bool *altitudeOk = nullptr);

/* Speed and altitude have to match the pattern. Values are not checked. */
bool speedAndAltitudeMatch(const QString& item);

/* Gives always NXXXAXXX for altitude < 18000 ft or NXXXFXXX for altitude > 18000 */
QString createSpeedAndAltitude(float speedKnots, float altFeet);

} // namespace util
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_FSUTIL_H
