/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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
  // Additional types from X-Plane
  // FLIGHT_MANAGEMENT_SYSTEM_FMS_APPROACH: "FMS";
  // INSTRUMENT_GUIDANCE_SYSTEM_IGS_APPROACH: "IGS";
  // GNSS_LANDING_SYSTEM_GLSAPPROACH: "GNSS";
  // TACAN_APPROACH: "TCN";
  // MICROWAVE_LANDING_SYSTEM_MLS_TYPE_A_APPROACH:
  // MICROWAVE_LANDING_SYSTEM_MLS_TYPE_B_AND_C_APPROACH: "MLS";
  // PROCEDURE_WITH_CIRCLE_TOLAND_MINIMUMS: "CTL";
  // MICROWAVE_LANDING_SYSTEM_MLS_APPROACH: "MLS";

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

    case ap::TACAN:
      return "TCN";
  }
  qWarning().nospace().noquote() << "Invalid approach type " << type;
  return "UNKN";
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

    /* From P3D v5 upwards - these are wrong types for this field taken from the XSD.
     * They will be converted to WAYPOINT. */
    case ap::fix::MANUAL_TERMINATION:
    case ap::fix::COURSE_TO_ALT:
    case ap::fix::COURSE_TO_DIST:
    case ap::fix::HEADING_TO_ALT:
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
