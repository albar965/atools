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
    case ap::fix::AIRPORT: // New in MSFS
      return "A";

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

QString arincNameAppr(ApproachType type, const QString& runwayName, char suffix, bool gpsOverlay)
{
  // Not considered in BGL or not applicable
  // Approach Transition  A
  // Flight Management System (FMS) Approach  F
  // Instrument Guidance System (IGS) Approach  G
  // GNSS Landing System (GLS)Approach  J
  // Microwave Landing System (MLS) Approach  M
  // Microwave Landing System (MLS), Type A Approach  W
  // Microwave Landing System (MLS), Type B and C Approach  Y
  // Missed Approach  Z

  QString arincAppWithRunway, arincAppCircleToLand;
  switch(type)
  {
    case atools::fs::bgl::ap::GPS:
      // Global Positioning System (GPS) Approach P or Arrival / STAR or Departure / SID
      if((suffix != 'A' && suffix != 'D') || !gpsOverlay)
        arincAppWithRunway = "P";
      break;

    case atools::fs::bgl::ap::VOR:
      // VOR Approach V
      arincAppWithRunway = "V";
      arincAppCircleToLand = "VOR";
      break;

    case atools::fs::bgl::ap::NDB:
      // Non-Directional Beacon (NDB) Approach N
      arincAppWithRunway = "N";
      arincAppCircleToLand = "NDB";
      break;

    case atools::fs::bgl::ap::ILS:
      // Instrument Landing System (ILS) Approach I
      arincAppWithRunway = "I";
      arincAppCircleToLand = "ILS"; // Should never happen - always has a runway
      break;

    case atools::fs::bgl::ap::LOCALIZER:
      // Localizer Only (LOC) Approach L
      arincAppWithRunway = "L";
      arincAppCircleToLand = "LDM";
      break;

    case atools::fs::bgl::ap::SDF:
      // Simplified Directional Facility (SDF) Approach U
      arincAppWithRunway = "U";
      arincAppCircleToLand = "SDF"; // Should never happen - always has a runway
      break;

    case atools::fs::bgl::ap::LDA:
      // Localizer Directional Aid (LDA) Approach X
      arincAppWithRunway = "X";
      arincAppCircleToLand = "LDA";
      break;

    case atools::fs::bgl::ap::VORDME:
      // VOR Approach using VORDME/VORTAC S - not in BGL
      // VORDME Approach D
      arincAppWithRunway = "D";
      arincAppCircleToLand = "VDM";
      break;

    case atools::fs::bgl::ap::NDBDME:
      // Non-Directional Beacon + DME (NDB+DME) Approach Q
      arincAppWithRunway = "Q";
      arincAppCircleToLand = "NDM";
      break;

    case atools::fs::bgl::ap::RNAV:
      // Area Navigation (RNAV) Approach (Note 1) R
      arincAppWithRunway = "R";
      arincAppCircleToLand = "RNV";
      break;

    case atools::fs::bgl::ap::LOCALIZER_BACKCOURSE:
      // Localizer/Backcourse Approach B
      arincAppWithRunway = "B";
      arincAppCircleToLand = "LBC";
      break;

    case atools::fs::bgl::ap::TACAN:
      // TACAN Approach T
      arincAppWithRunway = "T";
      arincAppCircleToLand = "TCN"; // Should never happen - always has a runway
      break;
  }

  QString rw = runwayName != "00" && !runwayName.isEmpty() ? runwayName : QString();
  QString sfx = suffix != '0' && suffix != 0 ? QString(QChar(suffix)) : QString();
  QString arinc;

  if((sfx == 'A' || sfx == 'D') && gpsOverlay)
  {
    // SID or STAR ======================
    if(!rw.isEmpty())
      arinc = "RW" + rw;
    else
      // No runway
      arinc = "ALL";
  }
  else
  {
    // Approach procedure ======================
    if(!rw.isEmpty())
      // Add runway and suffix - "D31L"
      arinc = arincAppWithRunway + rw;
    else
      // Circle-to-land - "VOR", "RNV", etc.
      arinc = arincAppCircleToLand;

    if(!sfx.isEmpty())
    {
      if(arinc.size() >= 4 || rw.isEmpty())
        // Has runway designator or circle-to-land. Suffix after runway designator - "D31LZ" or "VORA"
        arinc += sfx;
      else
        // No runway designator. Suffix after runway number separated by dash - "D32-A"
        arinc += "-" + sfx;
    }
    else if(rw.isEmpty())
      // Circle-to-land and no suffix - prepend "C" - "CVOR" or "CVDM"
      arinc = QChar('C') + arinc;
  }
  return arinc;
}

} // namespace ap
} // namespace bgl
} // namespace fs
} // namespace atools
