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

#ifndef LITTLENAVMAP_COORDINATES_H
#define LITTLENAVMAP_COORDINATES_H

#include <QString>

namespace atools {
namespace geo {
class Pos;
}
namespace fs {
namespace util {

/* Recognizes the following formats or alternatively one from fromAnyWaypointFormat:
 *
 *  N49° 26' 41.57" E9° 12' 5.49"
 *  N54* 16.82' W008* 35.95'
 *  N 52 33.58 E 13 17.26
 *  49° 26' 41,57" N 9° 12' 5,49" E
 *  49° 26,69' N 9° 12,09' E
 *  49,4449° N 9,2015° E
 *  N 49,4449° E 9,2015°
 */
atools::geo::Pos fromAnyFormat(const QString& coords);

QString toGfpFormat(const atools::geo::Pos& pos);
QString toDegMinFormat(const atools::geo::Pos& pos);
QString toDegMinSecFormat(const atools::geo::Pos& pos);

atools::geo::Pos fromAnyWaypointFormat(const QString& str);

/* N44124W122451 or N14544W017479 or S31240E136502 */
atools::geo::Pos fromGfpFormat(const QString& str);

/* Degrees only 46N078W */
atools::geo::Pos fromDegFormat(const QString& str);

/* Degrees and minutes 4620N07805W */
atools::geo::Pos fromDegMinFormat(const QString& str);

/* Degrees, minutes and seconds 481200N0112842E (Skyvector) */
atools::geo::Pos fromDegMinSecFormat(const QString& str);

/* Degrees and minutes in pair "N6500 W08000" or "N6500/W08000" */
atools::geo::Pos fromDegMinPairFormat(const QString& str);

/* NAT type 5020N */
atools::geo::Pos fromNatFormat(const QString& str);

/* OpenAir airspace format
*  50:40:42 N 003:13:30 E
*  39:06.2 N 121:35.5 E */
atools::geo::Pos fromOpenAirFormat(const QString& coordStr);

} // namespace util
} // namespace fs
} // namespace atools

#endif // LITTLENAVMAP_COORDINATES_H
