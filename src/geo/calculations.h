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

#ifndef ATOOLS_GEO_CALCULATIONS_H
#define ATOOLS_GEO_CALCULATIONS_H

namespace atools {
namespace geo {

class Pos;

/* Calculate distance in meters (great circle route) between positions */
double distanceInMeters(const Pos& from, const Pos& to);

/* Calculate distance in nautical miles (great circle route)
 * between positions */
double distanceInNm(const Pos& from, const Pos& to);

/* Distance from meters to nautical miles */
double metersToNm(double nm);

/* Distance from nautical miles to meters */
double nmToMeters(double nm);

} /* namespace geo */
} // namespace atools

#endif /* ATOOLS_GEO_CALCULATIONS_H */
