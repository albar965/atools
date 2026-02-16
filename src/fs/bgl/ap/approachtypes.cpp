/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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
      return QStringLiteral("GPS");

    case ap::VOR:
      return QStringLiteral("VOR");

    case ap::NDB:
      return QStringLiteral("NDB");

    case ap::ILS:
      return QStringLiteral("ILS");

    case ap::LOCALIZER:
      return QStringLiteral("LOC");

    case ap::SDF:
      return QStringLiteral("SDF");

    case ap::LDA:
      return QStringLiteral("LDA");

    case ap::VORDME:
      return QStringLiteral("VORDME");

    case ap::NDBDME:
      return QStringLiteral("NDBDME");

    case ap::RNAV:
      return QStringLiteral("RNAV");

    case ap::LOCALIZER_BACKCOURSE:
      return QStringLiteral("LOCB");

    case ap::TACAN:
      return QStringLiteral("TCN");
  }
  qWarning().nospace().noquote() << "Invalid approach type " << type;
  return QStringLiteral("UNKN");
}

QString approachFixTypeToStr(ap::fix::ApproachFixType type)
{
  switch(type)
  {
    case ap::fix::AIRPORT: // New in MSFS
      return QStringLiteral("A");

    case ap::fix::LOCALIZER:
      return QStringLiteral("L");

    case ap::fix::NONE:
      return QStringLiteral("NONE");

    case ap::fix::VOR:
      return QStringLiteral("V");

    case ap::fix::NDB:
      return QStringLiteral("N");

    case ap::fix::TERMINAL_NDB:
      return QStringLiteral("TN");

    /* From P3D v5 upwards - these are wrong types for this field taken from the XSD.
     * They will be converted to WAYPOINT. */
    case ap::fix::MANUAL_TERMINATION:
    case ap::fix::COURSE_TO_ALT:
    case ap::fix::COURSE_TO_DIST:
    case ap::fix::HEADING_TO_ALT:
    case ap::fix::WAYPOINT:
      return QStringLiteral("W");

    case ap::fix::TERMINAL_WAYPOINT:
      return QStringLiteral("TW");

    case ap::fix::RUNWAY:
      return QStringLiteral("R");
  }
  qWarning().nospace().noquote() << "Invalid approach fix type " << type;
  return QStringLiteral("INVALID");
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
        arincAppWithRunway = QStringLiteral("P");
      break;

    case atools::fs::bgl::ap::VOR:
      // VOR Approach V
      arincAppWithRunway = QStringLiteral("V");
      arincAppCircleToLand = QStringLiteral("VOR");
      break;

    case atools::fs::bgl::ap::NDB:
      // Non-Directional Beacon (NDB) Approach N
      arincAppWithRunway = QStringLiteral("N");
      arincAppCircleToLand = QStringLiteral("NDB");
      break;

    case atools::fs::bgl::ap::ILS:
      // Instrument Landing System (ILS) Approach I
      arincAppWithRunway = QStringLiteral("I");
      arincAppCircleToLand = QStringLiteral("ILS"); // Should never happen - always has a runway
      break;

    case atools::fs::bgl::ap::LOCALIZER:
      // Localizer Only (LOC) Approach L
      arincAppWithRunway = QStringLiteral("L");
      arincAppCircleToLand = QStringLiteral("LDM");
      break;

    case atools::fs::bgl::ap::SDF:
      // Simplified Directional Facility (SDF) Approach U
      arincAppWithRunway = QStringLiteral("U");
      arincAppCircleToLand = QStringLiteral("SDF"); // Should never happen - always has a runway
      break;

    case atools::fs::bgl::ap::LDA:
      // Localizer Directional Aid (LDA) Approach X
      arincAppWithRunway = QStringLiteral("X");
      arincAppCircleToLand = QStringLiteral("LDA");
      break;

    case atools::fs::bgl::ap::VORDME:
      // VOR Approach using VORDME/VORTAC S - not in BGL
      // VORDME Approach D
      arincAppWithRunway = QStringLiteral("D");
      arincAppCircleToLand = QStringLiteral("VDM");
      break;

    case atools::fs::bgl::ap::NDBDME:
      // Non-Directional Beacon + DME (NDB+DME) Approach Q
      arincAppWithRunway = QStringLiteral("Q");
      arincAppCircleToLand = QStringLiteral("NDM");
      break;

    case atools::fs::bgl::ap::RNAV:
      // Area Navigation (RNAV) Approach (Note 1) R
      arincAppWithRunway = QStringLiteral("R");
      arincAppCircleToLand = QStringLiteral("RNV");
      break;

    case atools::fs::bgl::ap::LOCALIZER_BACKCOURSE:
      // Localizer/Backcourse Approach B
      arincAppWithRunway = QStringLiteral("B");
      arincAppCircleToLand = QStringLiteral("LBC");
      break;

    case atools::fs::bgl::ap::TACAN:
      // TACAN Approach T
      arincAppWithRunway = QStringLiteral("T");
      arincAppCircleToLand = QStringLiteral("TCN"); // Should never happen - always has a runway
      break;
  }

  QString rw = runwayName != QStringLiteral("00") && !runwayName.isEmpty() ? runwayName : QString();
  QString sfx = suffix != '0' && suffix != 0 ? QString(QChar(suffix)) : QString();
  QString arinc;

  if((sfx == 'A' || sfx == 'D') && gpsOverlay)
  {
    // SID or STAR ======================
    if(!rw.isEmpty())
      arinc = QStringLiteral("RW") + rw;
    else
      // No runway
      arinc = QStringLiteral("ALL");
  }
  else
  {
    // Approach procedure ======================
    if(!rw.isEmpty())
      // Add runway and suffix - QStringLiteral("D31L")
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
        arinc += QStringLiteral("-") + sfx;
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
