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

#ifndef ATOOLS_APPROACHTYPES_H
#define ATOOLS_APPROACHTYPES_H

#include <QString>

namespace atools {
namespace fs {
namespace bgl {

namespace ap {

enum ApproachType
{
  GPS = 0x01,
  VOR = 0x02,
  NDB = 0x03,
  ILS = 0x04,
  LOCALIZER = 0x05,
  SDF = 0x06,
  LDA = 0x07,
  VORDME = 0x08,
  NDBDME = 0x09,
  RNAV = 0x0a,
  LOCALIZER_BACKCOURSE = 0x0b
};

QString approachTypeToStr(atools::fs::bgl::ap::ApproachType type);

enum ApproachFixType
{
  FIX_NONE = 0,
  FIX_VOR = 2,
  FIX_NDB = 3,
  FIX_TERMINAL_NDB = 4,
  FIX_WAYPOINT = 5,
  FIX_TERMINAL_WAYPOINT = 6,
  FIX_UNKNWON_VALUE_8 = 8,
  FIX_RUNWAY = 9
};

QString approachFixTypeToStr(atools::fs::bgl::ap::ApproachFixType type);

} // namespace ap
} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_APPROACHTYPES_H
