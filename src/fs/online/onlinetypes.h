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

#ifndef ATOOLS_ONLINETYPES_H
#define ATOOLS_ONLINETYPES_H

class QString;

namespace atools {
namespace fs {
namespace online {

enum Format
{
  UNKNOWN,
  VATSIM,
  IVAO
};

// enum ClientType
// Value  Description
// ATC  ATC or Observer connections
// PILOT  Pilot connections
// FOLME  Follow Me Car connections
QString clientType(const QString& type);

namespace fac {
enum FacilityType
{
  OBSERVER = 0,
  FLIGHT_INFORMATION = 1,
  DELIVERY = 2,
  GROUND = 3,
  TOWER = 4,
  APPROACH = 5,
  ACC = 6,
  DEPARTURE = 7
};

}

namespace adm {
enum AdministrativeRating
{
  SUSPENDED = 0,
  OBSERVER = 1,
  USER = 2,
  SUPERVISOR = 11,
  ADMINISTRATOR = 12
};

}

namespace atc {
enum AtcRating
{
  OBSERVER = 1,
  BASIC_FLIGHT_STUDENT_FS1 = 2,
  FLIGHT_STUDENT_FS2 = 3,
  ADVANCED_FLIGHT_STUDENT_FS3 = 4,
  PRIVATE_PILOT_PP = 5,
  SENIOR_PRIVATE_PILOT_SPP = 6,
  COMMERCIAL_PILOT_CP = 7,
  AIRLINE_TRANSPORT_PILOT_ATP = 8,
  SENIOR_FLIGHT_INSTRUCTOR_SFI = 9,
  CHIEF_FLIGHT_INSTRUCTOR_CFI = 10
};

}

namespace sim {
enum Simulator
{
  UNKNOWN = 0,
  MICROSOFT_FLIGHT_SIMULATOR_95 = 1,
  MICROSOFT_FLIGHT_SIMULATOR_98 = 2,
  MICROSOFT_COMBAT_FLIGHT_SIMULATOR = 3,
  MICROSOFT_FLIGHT_SIMULATOR_2000 = 4,
  MICROSOFT_COMBAT_FLIGHT_SIMULATOR_2 = 5,
  MICROSOFT_FLIGHT_SIMULATOR_2002 = 6,
  MICROSOFT_COMBAT_FLIGHT_SIMULATOR_3 = 7,
  MICROSOFT_FLIGHT_SIMULATOR_2004 = 8,
  MICROSOFT_FLIGHT_SIMULATOR_X = 9,
  XPLANE = 11,
  XPLANE_8X = 12,
  XPLANE_9X = 13,
  XPLANE_10X = 14,
  PS1 = 15,
  XPLANE_11X = 16,
  XPLANE_12X = 17,
  FLY = 20,
  FLY_2 = 21,
  FLIGHTGEAR = 25,
  PREPAR3D_1X = 30
};

}

} // namespace online
} // namespace fs
} // namespace atools

#endif // ATOOLS_ONLINETYPES_H
