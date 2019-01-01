/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_FS_LB_TYPES_H
#define ATOOLS_FS_LB_TYPES_H

namespace atools {
namespace fs {
namespace lb {
namespace types {

/* Mask for bitfield flags */
extern const int AIRCRAFT_FLAG_MULTIMOTOR;

enum AircraftType
{
  AIRCRAFT_UNKNOWN = 0,
  AIRCRAFT_GLIDER = 1,
  AIRCRAFT_FIXED_WING = 2,
  AIRCRAFT_AMPHIBIOUS = 3,
  AIRCRAFT_ROTARY = 4
};

enum RecordType
{
  RECORD_LOGBOOK_ENTRY = 5
};

enum RecordSubType
{
  SUBRECORD_PLANE_DESCRIPTION = 2,
  SUBRECORD_AIRPORT_LIST = 3,
  SUBRECORD_DESCRIPTION = 4
};

} // namespace types
} // namespace lb
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_LB_TYPES_H
