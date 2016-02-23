/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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

#include "geo/calculations.h"

namespace atools {

namespace geo {

// / @brief The usual PI/180 constant
// static const double DEG_TO_RAD =;

// / @brief Earth's quadratic mean radius for WGS-84
// static const double EARTH_RADIUS_IN_METERS = 6372797.560856;

// / @brief Based on WGS-84

/** @brief Computes the arc, in radian, between two WGS-84 Poss.
 *
 * The result is equal to <code>Distance(from,to)/EARTH_RADIUS_IN_METERS</code>
 *    <code>= 2*asin(sqrt(h(d/EARTH_RADIUS_IN_METERS )))</code>
 *
 * where:<ul>
 *    <li>d is the distance in meters between 'from' and 'to' Poss.</li>
 *    <li>h is the haversine function: <code>h(x)=sin²(x/2)</code></li>
 * </ul>
 *
 * The haversine formula gives:
 *    <code>h(d/R) =
 * h(from.lat-to.lat)+h(from.lon-to.lon)+cos(from.lat)*cos(to.lat)</code>
 *
 * @sa http://en.wikipedia.org/wiki/Law_of_haversines
 */
// double arcInRadians(const Pos& from, const Pos& to)
// {
// double latitudeArc = (from.getLatY() - to.getLatY()) * DEG_TO_RAD;
// double longitudeArc = (from.getLonX() - to.getLonX()) * DEG_TO_RAD;
// double latitudeH = sin(latitudeArc * 0.5);
// latitudeH *= latitudeH;
// double lontitudeH = sin(longitudeArc * 0.5);
// lontitudeH *= lontitudeH;
// double tmp = cos(from.getLatY() * DEG_TO_RAD) * cos(to.getLatY() * DEG_TO_RAD);
// return 2.0 * asin(sqrt(latitudeH + tmp * lontitudeH));
// }

} /* namespace geo */
} // namespace atools
