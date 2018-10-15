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

#ifndef AIRCRAFTPERFCONSTANTS_H
#define AIRCRAFTPERFCONSTANTS_H

#include <QString>

namespace atools {
namespace fs {
namespace perf {

/* Use to indicate invalid values */
const float INVALID_PERF_VALUE = std::numeric_limits<float>::max();

/* Default which is used if no performance is available */
const float DEFAULT_SPEED = 100.f;
const float DEFAULT_CLIMB_DESCENT = 333.f;

/* Fuel unit used in AircraftPerf */
enum FuelUnit
{
  // Number used as index in combo boxes
  WEIGHT, /* lbs or kg */
  VOLUME, /* gallons or liters */
  UNKNOWN = -1
};

QString fuelUnitToString(atools::fs::perf::FuelUnit type);
atools::fs::perf::FuelUnit fuelUnitFromString(const QString& value);

} // namespace perf
} // namespace fs
} // namespace atools

#endif // AIRCRAFTPERFCONSTANTS_H
