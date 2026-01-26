/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

class QDateTime;

namespace atools {
namespace geo {
class LineString;
class Pos;
class PosD;
}
namespace fs {
namespace util {

/* Add rhumb line points for all lines having equal latY. Also moves points at the poles slightly away. */
atools::geo::LineString correctBoundary(const atools::geo::LineString& geometry);

/* Calculate the display points of an ILS feather. Minimum width is 2 degrees */
void calculateIlsGeometry(const atools::geo::Pos& pos, float headingTrue, float widthDeg, float featherLengthNm,
                          geo::Pos& p1, geo::Pos& p2, geo::Pos& pmid);

void calculateIlsGeometryD(const atools::geo::PosD& pos, double headingTrue, double widthDeg, double featherLengthNm,
                           atools::geo::PosD& p1, atools::geo::PosD& p2, atools::geo::PosD& pmid);

const float DEFAULT_FEATHER_LEN_NM = 9.f;

/* Split runway name into parts and return true if name matches a runway number */
bool runwayNameSplitNum(const QString& name, int *number = nullptr, QString *designator = nullptr, bool *trueHeading = nullptr);

/* Split runway name into parts and return true if name matches a runway number. Number is zero prefixed. */
bool runwayNameSplitStr(const QString& name, QString *number = nullptr, QString *designator = nullptr, bool *trueHeading = nullptr);

/* true if any of "RW01", "RW1", "1", "01", "RW02C", "RW1C", "1C", "01C" or "02T" */
bool runwayNameValid(const QString& name);

/* Compare numerically by number and then by designator in order of L, C, R */
int compareRunwayNumber(const QString& rw1, const QString& rw2);

/* From "L", "R" to "LEFT" and "RIGHT" */
QString runwayDesignatorLong(const QString& designatorName);

/* "01R" to "1" */
QString runwayNumber(const QString& runwayName);

/* "01R" to "R" */
QString runwayDesignator(const QString& runwayName);

/* Get the closes matching runway name from the list of airport runways or empty if none.
 * Returns the runway as formatted in runwayName */
QString runwayBestFit(const QString& runwayName, const QStringList& airportRunwayNames);

/* As above but returns runways formatted like the one found in the airport list */
QString runwayBestFitFromList(const QString& runwayName, const QStringList& airportRunwayNames);

/* Gives all variants of the runway (+1 and -1) plus the original one as the first in the list.
 *  Can deal with prefix "RW" and keeps it. */
const QStringList runwayNameVariants(QString name);

/* Returns all variants of zero prefixed runways 09 vs 9. E.g. for "09C" returns "9C".
 *  Can deal with prefix "RW" and keeps it. */
const QStringList runwayNameZeroPrefixVariants(const QString& name);

/* Prefixes any runway number with zeros if needed. "9C" to "09C", for example. Returns "name" if not a runway. */
QString runwayNamePrefixZero(const QString& name);

/* Gives all variants of the runway (+1 and -1) plus the original one as the first in the list for an
 * ARINC name like N32 or I19-Y */
QStringList arincNameNameVariants(const QString& name);

/* Compare runway numbers by ignoring leading zero and prefix "RW".
 * If fuzzy = true: Compare runway numbers fuzzy by ignoring a deviation of one */
bool runwayEqual(QString name1, QString name2, bool fuzzy);
bool runwayContains(const QStringList& runways, QString name, bool fuzzy);

/* True if e.g. "RW10B" for a SID or STAR which means that 10L, 10C and 10R can be used. */
bool hasSidStarParallelRunways(QString approachArincName);

/* True if "ALL" for a SID or STAR. Means SID/STAR can be used for all runways of an airport. */
bool hasSidStarAllRunways(const QString& approachArincName);

/* Get a list of runways for SID and STAR which are applicable for all or several parallel runways
 * @param runwayNames List of all runway names of an airport prefixed with "RW"
 * @param arincName Name like "RW07B" or "ALL"
 */
void sidStarMultiRunways(const QStringList& runwayNames, const QString& arincName, QStringList *sidStarRunways,
                         const QString& allDisplayName = QString(), QStringList *sidStarDispNames = nullptr);

/* Turn runway names to common form. "1" to "01", "01T" to "01", "RW01" to "01" */
QString normalizeRunway(QString runway);
QStringList normalizeRunways(QStringList names);

/* Converts decimals from transponder to integer.
 * Returns decimal 4095/ octal 07777 / hex 0xFFF for number 7777
 * -1 if number is not valid. */
qint16 decodeTransponderCode(int code);

/* Get the aircraft type name for the ICAO code */
const QString& aircraftTypeForCode(const QString& code);

/* Basic check by pattern if the ICAO aircraft type designator is valid */
bool isAircraftTypeDesignatorValid(const QString& type);

/* Integer frequency * 1000 rounded to the next 0.025 MHz value or
 * frequency * 100000 rounded to the next 8.33 kHz value */
float roundComFrequency(int frequency);

/* Maximum rating is 5 */
const int MAX_RATING = 5;

/* Calculate for FSX/P3D based on airport facilities */
int calculateAirportRating(bool isAddon, bool hasTower, bool msfs, int numTaxiPaths, int numParkings, int numAprons);

/* Calculate for X-Plane based on airport facilities */
int calculateAirportRatingXp(bool isAddon, bool is3D, bool hasTower, int numTaxiPaths, int numParkings, int numAprons);

/* Check the airport name if it contains military designators */
bool isNameMilitary(QString airportName);

/* Check if the name contains a closed identifier */
bool isNameClosed(const QString& airportName);

/* Capitalize any string ignoring aviation acronyms */
QString capNavString(const QString& str);

/* Capitalize only if ident not equal to name. Used for waypoint names. */
QString capWaypointNameString(const QString& ident, const QString& name, bool emptyIfEqual);

/* Capitalize airport name making special designators (AFB, ...) upper case */
QString capAirportName(const QString& str);

/* Limits ident to upper case characters and digits and trims length to five for userpoints.
 * Returns N with following number if empty. */
QString adjustIdent(QString ident, int length = 5, int id = -1);

/* Limits region to upper case characters and trims length to 2. Returns "ZZ" if empty. For userpoints. */
QString adjustRegion(QString region);

/* Max 10 characters, digits, space and underscore */
QString adjustFsxUserWpName(QString name, int length = 10);

/* More relaxed than FSX. Uses default name if the result is empty and increments number. */
QString adjustMsfsUserWpName(QString name, int length = 10, int *number = nullptr);

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
QString createSpeedAndAltitude(float speedKts, float altFeet, bool metricSpeed, bool metricAlt);

/* Converts ARINC 424.18 field type definition 5.42 to 32 bit representation
 * E.g. "V  " to "2105430" */
QString waypointFlagsToXplane(QString flags, const QString& defaultValue = QString());

/* Converts 32 bit representation to ARINC 424.18 field type definition 5.42
 * E.g. "2105430" to "V  " */
QString waypointFlagsFromXplane(const QString& flags, const QString& defaultValue = QString());

/* Extract date from filenames like GRIB-2022-11-25-00.00-ZULU-wind.grib or GRIB-2022-9-6-21.00-wind.grib */
QDateTime xpGribFilenameToDate(const QString& filename);

/* Extract date from filenames like Metar-2022-9-6-20.00.txt or metar-2022-11-24-21.00-ZULU.txt */
QDateTime xpMetarFilenameToDate(const QString& filename);

} // namespace util
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_FSUTIL_H
