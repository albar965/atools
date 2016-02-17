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

#include "logging/loggingdefs.h"
#include "fs/bgl/ap/approachtypes.h"

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
      return "LOCALIZER";

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
      return "LOCALIZER_BACKCOURSE";
  }
  qWarning().nospace().noquote() << "Unknown approach type " << type;
  return QString();
}

QString approachFixTypeToStr(ap::ApproachFixType type)
{
  switch(type)
  {
    case atools::fs::bgl::ap::FIX_UNKNWON_VALUE_8:
      return "FIX_UNKNWON_VALUE_8";

    case ap::FIX_NONE:
      return "NONE";

    case ap::FIX_VOR:
      return "VOR";

    case ap::FIX_NDB:
      return "NDB";

    case ap::FIX_TERMINAL_NDB:
      return "TERMINAL_NDB";

    case ap::FIX_WAYPOINT:
      return "WAYPOINT";

    case ap::FIX_TERMINAL_WAYPOINT:
      return "TERMINAL_WAYPOINT";

    case ap::FIX_RUNWAY:
      return "RUNWAY";
  }
  qWarning().nospace().noquote() << "Unknown approach fix type " << type;
  return QString();
}

} // namespace ap
} // namespace bgl
} // namespace fs
} // namespace atools
