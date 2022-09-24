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

#ifndef ATOOLS_FS_FSUTIL_H
#define ATOOLS_FS_FSUTIL_H

#include <QString>

namespace atools {
namespace fs {
namespace util {

/* Split runway name into parts and return true if name matches a runway number */
bool runwayNameSplit(const QString& name, int *number = nullptr, QString *designator = nullptr, bool *trueHeading = nullptr);

/* Split runway name into parts and return true if name matches a runway number. Number is zero prefixed. */
bool runwayNameSplit(const QString& name, QString *number = nullptr, QString *designator = nullptr, bool *trueHeading = nullptr);

QString runwayDesignatorLong(const QString& name);

/* Get the closes matching runway name from the list of airport runways or empty if none.
 * Returns the runway as formatted in runwayName */
QString runwayBestFit(const QString& runwayName, const QStringList& airportRunwayNames);

/* As above but returns runways formatted like the one found in the airport list */
QString runwayBestFitFromList(const QString& runwayName, const QStringList& airportRunwayNames);

/* Gives all variants of the runway (+1 and -1) plus the original one as the first in the list.
 *  Can deal with prefix "RW" and keeps it. */
QStringList runwayNameVariants(QString name);

/* Returns all variants of zero prefixed runways 09 vs 9. E.g. for "09C" returns "9C".
 *  Can deal with prefix "RW" and keeps it. */
QStringList runwayNameZeroPrefixVariants(const QString& name);

/* Prefixes any runway number with zeros if needed. "9C" to "09C", for example. Returns "name" if not a runway. */
QString runwayNamePrefixZero(const QString& name);

/* Gives all variants of the runway (+1 and -1) plus the original one as the first in the list for an
 * ARINC name like N32 or I19-Y */
QStringList arincNameNameVariants(const QString& name);

/* Compare runway numbers fuzzy by ignoring a deviation of one */
bool runwayAlmostEqual(const QString& name1, const QString& name2);

/* Compare runway numbers by ignoring leading zero and prefix "RW" */
bool runwayEqual(QString name1, QString name2);
bool runwayContains(const QStringList& runways, QString name);

/* True if e.g. "RW10B" for a SID or STAR which means that 10L, 10C and 10R can be used. */
bool hasSidStarParallelRunways(QString approachArincName);

/* True if "ALL" for a SID or STAR. Means SID/STAR can be used for all runways of an airport. */
bool hasSidStarAllRunways(const QString& approachArincName);

/* Get a list of runways for SID and STAR which are applicable for all or several parallel runways
 * @param runwayNames List of all runway names of an airport prefixed with "RW"
 * @param arincName Name like "RW07B" or "ALL"
 */
void sidStarMultiRunways(const QStringList& runwayNames, QString arincName, const QString& allName, QStringList *sidStarRunways,
                         QStringList *sidStarDispNames = nullptr);

/* Converts decimals from transponder to integer.
 * Returns decimal 4095/ octal 07777 / hex 0xFFF for number 7777
 * -1 if number is not valid. */
qint16 decodeTransponderCode(int code);

/* Get the aircraft type name for the ICAO code */
QString aircraftTypeForCode(const QString& code);

/* Integer frequency * 1000 rounded to the next 0.025 MHz value or
 * frequency * 100000 rounded to the next 8.33 kHz value */
float roundComFrequency(int frequency);

/* Maximum rating is 5 */
const int MAX_RATING = 5;

/* Calculate for FSX/P3D based on airport facilities */
int calculateAirportRating(bool isAddon, bool hasTower, int numTaxiPaths, int numParkings, int numAprons);

/* Calculate for X-Plane based on airport facilities */
int calculateAirportRatingXp(bool isAddon, bool is3D, bool hasTower, int numTaxiPaths, int numParkings, int numAprons);

/* Check the airport name if it contains military designators */
bool isNameMilitary(QString airportName);

/* Check if the name contains a closed identifier */
bool isNameClosed(const QString& airportName);

/* Capitalize any string ignoring aviation acronyms */
QString capNavString(const QString& str);

/* Capitalize airport name making special designators (AFB, ...) upper case */
QString capAirportName(const QString& str);

/* Limits ident to upper case characters and digits and trims length to five for userpoints.
 * Returns N with following number if empty. */
QString adjustIdent(QString ident, int length = 5, int id = -1);

/* Limits region to upper case characters and trims length to 2. Returns "ZZ" if empty. For userpoints. */
QString adjustRegion(QString region);

/* Max 10 characters, digits, space and underscore */
QString adjustFsxUserWpName(QString name, int length = 10);

/* More relaxed than FSX */
QString adjustMsfsUserWpName(QString name, int length = 10);

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

/* Converts ARINC 424.18 field type definition 5.42 to 32 bit representation
 * E.g. "V  " to "2105430" */
QString waypointFlagsToXplane(QString flags, const QString& defaultValue = QString());

/* Converts 32 bit representation to ARINC 424.18 field type definition 5.42
 * E.g. "2105430" to "V  " */
QString waypointFlagsFromXplane(const QString& flags, const QString& defaultValue = QString());

} // namespace util
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_FSUTIL_H
