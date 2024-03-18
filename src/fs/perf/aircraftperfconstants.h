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

#ifndef AIRCRAFTPERFCONSTANTS_H
#define AIRCRAFTPERFCONSTANTS_H

#include <limits>

#include <QString>

namespace atools {
namespace fs {
namespace perf {

/* Flight segment as detected by the AircraftPerfHandler for simulator events.
 *  Numeric order is important for comparing. */
enum FlightSegment
{
  NONE, /* No state, before flight */
  DEPARTURE_PARKING, /* At gate not moving yet */
  DEPARTURE_TAXI, /* Moving at departure airport */
  CLIMB,
  CRUISE,
  DESCENT,
  DESTINATION_TAXI,
  DESTINATION_PARKING,
  LOADED, /* Inactive and loaded from last session */
  INVALID
};

enum RunwayType
{
  SOFT, /* Soft or hard runways */
  HARD, /* Hard runways only */
  WATER, /* Water runways only */
  WATER_LAND /* Water and soft/hard runways (amphibian) */
};

QString runwayTypeToStr(atools::fs::perf::RunwayType type);
atools::fs::perf::RunwayType runwayTypeFromStr(QString str);

/* Use to indicate invalid values */
const float INVALID_PERF_VALUE = std::numeric_limits<float>::max();

} // namespace perf
} // namespace fs
} // namespace atools

#endif // AIRCRAFTPERFCONSTANTS_H
