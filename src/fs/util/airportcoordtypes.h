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

#ifndef ATOOLS_AIRPORTCOORDTYPES_H
#define ATOOLS_AIRPORTCOORDTYPES_H

class QByteArray;

namespace atools {
namespace geo {
class Pos;
}
namespace fs {
namespace util {

/* Function pointer type for callback to fetch airport coordinates.
 * Use a raw pointer instead of std::function due to performance reasons.
 * The optional "object" can be used to pass in a "this" pointer.
 * Usage: noaaWeather->setFetchAirportCoords(&WeatherReporter::fetchAirportCoordinates, this);
 */
typedef atools::geo::Pos (*AirportCoordFuncType)(const QByteArray&, void *object);

} // namespace util
} // namespace fs
} // namespace atools

#endif // ATOOLS_AIRPORTCOORDTYPES_H
