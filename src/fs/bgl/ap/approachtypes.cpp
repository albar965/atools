/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

#include "fs/bgl/ap/approachtypes.h"

#include <QDebug>

namespace atools {
namespace fs {
namespace bgl {

namespace ap {

QString approachTypeToStr(ap::ApproachType type)
{
  switch(type)
  {
    case ap::GPS:
      return "GPS";

    case ap::VOR:
      return "VOR";

    case ap::NDB:
      return "NDB";

    case ap::ILS:
      return "ILS";

    case ap::LOCALIZER:
      return "LOC";

    case ap::SDF:
      return "SDF";

    case ap::LDA:
      return "LDA";

    case ap::VORDME:
      return "VORDME";

    case ap::NDBDME:
      return "NDBDME";

    case ap::RNAV:
      return "RNAV";

    case ap::LOCALIZER_BACKCOURSE:
      return "LOCB";
  }
  qWarning().nospace().noquote() << "Invalid approach type " << type;
  return "INVALID";
}

QString approachFixTypeToStr(ap::fix::ApproachFixType type)
{
  switch(type)
  {
    case ap::fix::LOCALIZER:
      return "L";

    case ap::fix::NONE:
      return "NONE";

    case ap::fix::VOR:
      return "V";

    case ap::fix::NDB:
      return "N";

    case ap::fix::TERMINAL_NDB:
      return "TN";

    case ap::fix::WAYPOINT:
      return "W";

    case ap::fix::TERMINAL_WAYPOINT:
      return "TW";

    case ap::fix::RUNWAY:
      return "R";
  }
  qWarning().nospace().noquote() << "Invalid approach fix type " << type;
  return "INVALID";
}

} // namespace ap
} // namespace bgl
} // namespace fs
} // namespace atools
