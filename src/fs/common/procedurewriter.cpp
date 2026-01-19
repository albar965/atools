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

#include "fs/common/procedurewriter.h"

#include "atools.h"
#include "fs/common/airportindex.h"
#include "sql/sqldatabase.h"
#include "sql/sqlquery.h"
#include "sql/sqlrecord.h"

#include "sql/sqlutil.h"

#pragma GCC diagnostic ignored "-Wswitch-enum"

using atools::sql::SqlQuery;
using atools::sql::SqlUtil;
using atools::sql::SqlRecord;
using atools::sql::SqlRecordList;

namespace atools {
namespace fs {
namespace common {

enum RunwayFieldIndex
{
  RW_ROW_CODE,
  IDENT,
  RWY_GRAD,
  LTP_ELLIPSOID_HEIGHT,
  LND_TRESH_ELEV,
  TCH,
  LOC_MLS_GLS_IDENT,
  CAT,
  CLASS,
  RESERVED
};

namespace rt {

// SID Route Type Description Field Content
enum SidRouteType
{
  ENGINE_OUT_SID = '0',
  SID_RUNWAY_TRANSITION = '1', // -> 3
  SID_OR_SID_COMMON_ROUTE = '2',
  SID_ENROUTE_TRANSITION = '3',
  RNAV_SID_RUNWAY_TRANSITION = '4', // -> 6
  RNAV_SID_OR_SID_COMMON_ROUTE = '5',
  RNAV_SID_ENROUTE_TRANSITION = '6',

  FMS_SID_RUNWAY_TRANSITION = 'F',
  FMS_SID_OR_SID_COMMON_ROUTE = 'M',
  FMS_SID_ENROUTE_TRANSITION = 'S',
  VECTOR_SID_RUNWAY_TRANSITION = 'T',
  VECTOR_SID_ENROUTE_TRANSITION = 'V'
};

// STAR Route Type Description Field Content
enum StarRouteType
{
  STAR_ENROUTE_TRANSITION = '1', // -> 2,3
  STAR_OR_STAR_COMMON_ROUTE = '2',
  STAR_RUNWAY_TRANSITION = '3',
  RNAV_STAR_ENROUTE_TRANSITION = '4', // -> 5,6
  RNAV_STAR_OR_STAR_COMMON_ROUTE = '5',
  RNAV_STAR_RUNWAY_TRANSITION = '6',
  PROFILE_DESCENT_ENROUTE_TRANSITION = '7',
  PROFILE_DESCENT_COMMON_ROUTE = '8',
  PROFILE_DESCENT_RUNWAY_TRANSITION = '9',

  FMS_STAR_ENROUTE_TRANSITION = 'F',
  FMS_STAR_OR_STAR_COMMON_ROUTE = 'M',
  FMS_STAR_RUNWAY_TRANSITION = 'S'
};

// Approach Route Type Description Route Type Field Content
enum ApproachRouteType
{
  LOCALIZER_BACKCOURSE_APPROACH = 'B',
  INSTRUMENT_LANDING_SYSTEM_ILS_APPROACH = 'I',
  LOCALIZER_ONLY_LOC_APPROACH = 'L',
  AREA_NAVIGATION_RNAV_APPROACH_NOTE_1 = 'R',
  NON_DIRECTIONAL_BEACON_NDB_APPROACH = 'N',
  VOR_APPROACH = 'V',
  MICROWAVE_LANDING_SYSTEM_MLS_APPROACH = 'M',
  APPROACH_TRANSITION = 'A',
  LOCALIZER_DIRECTIONAL_AID_LDA_APPROACH = 'X',
  SIMPLIFIED_DIRECTIONAL_FACILITY_SDF_APPROACH = 'U',
  VORDME_APPROACH = 'D',
  GLOBAL_POSITIONING_SYSTEM_GPS_APPROACH = 'P',
  FLIGHT_MANAGEMENT_SYSTEM_FMS_APPROACH = 'F',
  INSTRUMENT_GUIDANCE_SYSTEM_IGS_APPROACH = 'G',
  GNSS_LANDING_SYSTEM_GLSAPPROACH = 'J',
  NON_DIRECTIONAL_BEACON_AND_DME_NDB_AND_DME_APPROACH = 'Q',
  VOR_APPROACH_USING_VORDME_VORTAC = 'S',
  TACAN_APPROACH = 'T',
  MICROWAVE_LANDING_SYSTEM_MLS_TYPE_A_APPROACH = 'W',
  MICROWAVE_LANDING_SYSTEM_MLS_TYPE_B_AND_C_APPROACH = 'Y',
  MISSED_APPROACH = 'Z',
  DME_REQUIRED_FOR_PROCEDURE = 'D',
  GPS_REQUIRED_DME_DME_TO_RNP_XX_X_NOT_AUTHORIZED = 'J',
  GBAS_PROCEDURE = 'L',
  DME_NOT_REQUIRED_FOR_PROCEDURE = 'N',
  GPS_REQUIRED = 'P',
  GPS_OR_DME_DME_TO_RNP_XX_X_REQUIRED = 'R',
  DME_DME_REQUIRED_FOR_PROCEDURE = 'T',
  RNAV_SENSOR_NOT_SPECIFIED = 'U',
  VOR_DME_RNAV = 'V',
  SBAS_PROCEDURE_NOTE_2 = 'W',
  PRIMARY_MISSED_APPROACH = 'A',
  SECONDARY_MISSED_APPROACH = 'B',
  ENGINE_OUT_MISSED_APPROACH = 'E',
  PROCEDURE_WITH_CIRCLE_TOLAND_MINIMUMS = 'C',
  PROCEDURE_WITH_STRAIGHT_IN_MINIMUMS = 'S'
};

// All route types that are approach transitions
static const std::initializer_list<rt::ApproachRouteType> APPR_TRANS =
{
  rt::APPROACH_TRANSITION
};

// All route types that are SID transitions
static const std::initializer_list<rt::SidRouteType> SID_TRANS =
{
  rt::SID_ENROUTE_TRANSITION, rt::RNAV_SID_ENROUTE_TRANSITION, rt::FMS_SID_ENROUTE_TRANSITION,
  rt::VECTOR_SID_ENROUTE_TRANSITION
};

// All route types that are common SID routes
static const std::initializer_list<rt::SidRouteType> SID_COMMON =
{
  rt::SID_OR_SID_COMMON_ROUTE, rt::RNAV_SID_OR_SID_COMMON_ROUTE, rt::FMS_SID_OR_SID_COMMON_ROUTE
};

// All route types that are STAR transitions
static const std::initializer_list<rt::StarRouteType> STAR_TRANS =
{
  rt::STAR_ENROUTE_TRANSITION, rt::RNAV_STAR_ENROUTE_TRANSITION, rt::PROFILE_DESCENT_ENROUTE_TRANSITION
};

// All route types that are common STAR routes
static const std::initializer_list<rt::StarRouteType> STAR_COMMON =
{
  rt::STAR_OR_STAR_COMMON_ROUTE, rt::RNAV_STAR_OR_STAR_COMMON_ROUTE, rt::PROFILE_DESCENT_COMMON_ROUTE,
  rt::FMS_STAR_OR_STAR_COMMON_ROUTE
};

}

namespace sc {

enum SectionCode
{
  AIRPORT = 'P',
  ENROUTE = 'E',
  NAVAID = 'D'
};

// Section code D - navaids
enum NavaidSubSectionCode
{
  VHF = ' ',
  NDB = 'B'
};

// Section code E - enroute
enum EnrouteSubSectionCode
{
  WAYPOINTS = 'A',
  AIRWAY_MARKERS = 'M',
  HOLDING_PATTERNS = 'P',
  AIRWAYS_AND_ROUTES = 'R',
  PREFERRED_ROUTES = 'T',
  AIRWAY_RESTRICTIONS = 'U',
  ER_COMMUNICATIONS = 'V',

  VORDME = 'D'
};

// Section code P - Airport
enum AirportSubSectionCode
{
  REFERENCE_POINTS = 'A',
  GATES = 'B',
  TERMINAL_WAYPOINTS = 'C',
  SIDS = 'D',
  STARS = 'E',
  APPROACH_PROCEDURES = 'F',
  RUNWAYS = 'G',
  LOCALIZER_GLIDE_SLOPE = 'I',
  TAA = 'K',
  MLS = 'L',
  LOCALIZER_MARKER = 'M',
  TERMINAL_NDB = 'N',
  PATH_POINT = 'P',
  FLT_PLANNING_ARR_DEP = 'R',
  MSA = 'S',
  GLS_STATION = 'T',
  AP_COMMUNICATIONS = 'V'
};

}

namespace dc {

enum WaypointDescriptionCode1
{
  AIRPORT_AS_WAYPOINT = 'A',
  ESSENTIAL_WAYPOINT = 'E',
  OFF_AIRWAY_WAYPOINT = 'F',
  RUNWAY_AS_WAYPOINT = 'G',
  HELIPORT_AS_WAYPOINT = 'H',
  NDB_NAVAID_AS_WAYPOINT = 'N',
  PHANTOM_WAYPOINT = 'P',
  NON_ESSENTIAL_WAYPOINT = 'R',
  TRANSITION_ESSENTIAL_WAYPOINT = 'T',
  VHF_NAVAID_AS_WAYPOINT = 'V'
};

enum WaypointDescriptionCode2
{
  END_OF_SID_STAR_IAP_ROUTE_TYPE = 'B',
  END_OF_ENROUTE_AIRWAY_OR_TERMINAL_PROCEDURE = 'E',
  UNCHARTED_AIRWAY_INTERSECTION = 'U',
  FLYOVER_WAYPOINT = 'Y'
};

enum WaypointDescriptionCode3
{
  UNNAMED_STEPDOWN_WAYPOINT_AFTER_FINAL_APPROACH_WAYPOINT = 'A',
  UNNAMED_STEPDOWN_WAYPOINT_BEFORE_FINAL_APPROACH_WAYPOINT = 'B',
  ATC_COMPULSORY_WAYPOINT = 'C',
  OCEANIC_GATEWAY_WAYPOINT = 'G',
  FIRST_LEG_OF_MISSED_APPROACH_PROCEDURE = 'M',
  PATH_POINT_WAYPOINT = 'P',
  NAMED_STEPDOWN_WAYPOINT = 'S'
};

enum WaypointDescriptionCode4
{
  INITIAL_APPROACH_WAYPOINT = 'A',
  INTERMEDIATE_APPROACH_WAYPOINT = 'B',
  INITIAL_APPROACH_WAYPOINT_WITH_HOLDING = 'C',
  INITIAL_APPROACH_WITH_FINAL_APPROACH_COURSE_WAYPOINT = 'D',
  FINAL_END_POINT_WAYPOINT = 'E',
  PUBLISHED_FINAL_APPROACH_WAYPOINT_OR_DATABASE_FINAL_APPROACH_WAYPOINT = 'F',
  HOLDING_WAYPOINT = 'H',
  FINAL_APPROACH_COURSE_WAYPOINT = 'I',
  PUBLISHED_MISSED_APPROACH_POINT_WAYPOINT = 'M'
};

}

ProcedureWriter::ProcedureWriter(atools::sql::SqlDatabase& sqlDb, atools::fs::common::AirportIndex *airportIndexParam)
  : db(sqlDb), airportIndex(airportIndexParam),
  // Create SqlRecords for tables that can be cloned before filling with data
  APPROACH_RECORD(sqlDb.record("approach", ":")), APPROACH_LEG_RECORD(sqlDb.record("approach_leg", ":")),
  TRANSITION_RECORD(sqlDb.record("transition", ":")), TRANSITION_LEG_RECORD(sqlDb.record("transition_leg", ":"))
{
  initQueries();
}

ProcedureWriter::~ProcedureWriter()
{
  deInitQueries();
}

void ProcedureWriter::write(const ProcedureInput& line)
{
  rc::RowCode rowCode = toRowCode(line);

  if(rowCode == rc::APPROACH || rowCode == rc::SID || rowCode == rc::STAR)
  {
    int seqNo = line.seqNr;
    char routeType = line.routeType;
    QString routeIdent = line.sidStarAppIdent;
    QString transIdent = line.transIdent;

    // Finish and write a procedure if code has changed - includes transitions and probably multiple approaches
    if(rowCode != curRowCode)
      finishProcedure(line);
    else if(routeIdent != curRouteIdent)
      finishProcedure(line);

    // Start a new procedure row in approach or transition table if these changes
    bool writeNewProcedure = routeType != curRouteType || routeIdent != curRouteIdent || transIdent != curTransIdent;

    curRowCode = rowCode;
    curSeqNo = seqNo;
    curRouteType = routeType;
    curRouteIdent = routeIdent;
    curTransIdent = transIdent;

    if(writeNewProcedure)
      writeProcedure(line);
    else
      writeProcedureLeg(line);
  }
  // ignore RWY and PRDAT
}

void ProcedureWriter::writeProcedure(const ProcedureInput& line)
{
  if(curRowCode == rc::APPROACH)
  {
    if(atools::contains(static_cast<rt::ApproachRouteType>(curRouteType), rt::APPR_TRANS))
      writeTransition(line);
    else
      writeApproach(line);
  }
  else if(curRowCode == rc::SID)
  {
    // first approach then transition
    // duplicate all transitions for each approach before writing transitions
    if(atools::contains(static_cast<rt::SidRouteType>(curRouteType), rt::SID_TRANS))
      writeTransition(line);
    else
      writeApproach(line);
  }
  else if(curRowCode == rc::STAR)
  {
    // first transition then approach
    // duplicate all transitions for each approach before writing transitions
    if(atools::contains(static_cast<rt::StarRouteType>(curRouteType), rt::STAR_TRANS))
      writeTransition(line);
    else
      writeApproach(line);
  }
}

void ProcedureWriter::writeProcedureLeg(const ProcedureInput& line)
{
  if(curRowCode == rc::APPROACH)
  {
    if(atools::contains(static_cast<rt::ApproachRouteType>(curRouteType), rt::APPR_TRANS))
      writeTransitionLeg(line);
    else
      writeApproachLeg(line);
  }
  else if(curRowCode == rc::SID)
  {
    if(atools::contains(static_cast<rt::SidRouteType>(curRouteType), rt::SID_TRANS))
      writeTransitionLeg(line);
    else
      writeApproachLeg(line);
  }
  else if(curRowCode == rc::STAR)
  {
    if(atools::contains(static_cast<rt::StarRouteType>(curRouteType), rt::STAR_TRANS))
      writeTransitionLeg(line);
    else
      writeApproachLeg(line);
  }
}

void ProcedureWriter::assignApproachIds(ProcedureWriter::Procedure& proc)
{
  proc.record.setValue(QStringLiteral(":approach_id"), ++curApproachId);
}

void ProcedureWriter::assignApproachLegIds(sql::SqlRecordList& records)
{
  for(SqlRecord& rec : records)
  {
    rec.setValue(QStringLiteral(":approach_leg_id"), ++curApproachLegId);
    rec.setValue(QStringLiteral(":approach_id"), curApproachId);
  }
}

void ProcedureWriter::assignTransitionIds(ProcedureWriter::Procedure& proc)
{
  proc.record.setValue(QStringLiteral(":transition_id"), ++curTransitionId);
  proc.record.setValue(QStringLiteral(":approach_id"), curApproachId);

  for(SqlRecord& rec : proc.legRecords)
  {
    rec.setValue(QStringLiteral(":transition_leg_id"), ++curTransitionLegId);
    rec.setValue(QStringLiteral(":transition_id"), curTransitionId);
  }
}

void ProcedureWriter::finishProcedure(const ProcedureInput& line)
{
  if(curRowCode == rc::APPROACH)
  {
    numProcedures += approaches.size() + transitions.size();
    if(approaches.size() > 1)
      qWarning() << line.context << "Found more than one approach" << approaches.size();

    if(approaches.isEmpty())
      qWarning() << line.context << "No approaches found. Invalid state.";
    else
    {
      // Write approach
      Procedure& appr = approaches.first();

      // Collect flags from legs
      for(const SqlRecord& legRec : std::as_const(appr.legRecords))
      {
        if(legRec.valueFloat(QStringLiteral(":vertical_angle"), 0.f) < 0.1f)
          appr.record.setValue(":has_vertical_angle", 1);
        if(legRec.valueFloat(":rnp", 0.f) > 0.f)
          appr.record.setValue(":has_rnp", 1);
      }

      assignApproachIds(appr);
      assignApproachLegIds(appr.legRecords);
      insertApproachQuery->bindAndExecRecord(appr.record);
      insertApproachLegQuery->bindAndExecRecords(appr.legRecords);

      // Write transitions for one approach
      for(Procedure& trans : transitions)
      {
        assignTransitionIds(trans);
        insertTransitionQuery->bindAndExecRecord(trans.record);
        insertTransitionLegQuery->bindAndExecRecords(trans.legRecords);
      }
    }
  }
  else if(curRowCode == rc::STAR || curRowCode == rc::SID)
  {
    // KNRM SID
    // 5 RNAV_SID_OR_SID_COMMON_ROUTE
    // 6 RNAV_SID_ENROUTE_TRANSITION
    // more 6
    // 4 RNAV_SID_RUNWAY_TRANSITION
    // 6 RNAV_SID_ENROUTE_TRANSITION
    // more 6

    // STAR: First in file are transitions then multiple approaches
    // duplicate all transitions for each approach before writing

    // SID: First in file are approaches then multiple transitions
    // duplicate all approachs for each  transition before writing

    numProcedures += approaches.size() + transitions.size();

    if(approaches.isEmpty())
      qWarning() << line.context << "No SID/STAR found. Invalid state.";
    else
    {
      int numCommon = 0;
      for(const Procedure& appr : std::as_const(approaches))
      {
        if(appr.isCommonRoute)
          numCommon++;
      }

      if(numCommon > 1)
        qWarning() << line.context << "Found more than one common route for SID/STAR.";

      Procedure sidCommon, starCommon;
      if(curRowCode == rc::SID &&
         approaches.constLast().isCommonRoute && !approaches.constFirst().isCommonRoute)
      {
        // Example: EDDT SID
        // 4 RNAV_SID_RUNWAY_TRANSITION
        // more 4
        // 5 RNAV_SID_OR_SID_COMMON_ROUTE

        sidCommon = approaches.takeLast();

        // Remove the IF of the common route
        if(sidCommon.legRecords.constFirst().value(QStringLiteral(":type")) == QStringLiteral("IF"))
          sidCommon.legRecords.removeFirst();
      }

      if(curRowCode == rc::STAR && !approaches.constLast().isCommonRoute)
      {
        // Example: KBOI STAR
        // 4 RNAV_STAR_ENROUTE_TRANSITION
        // more 4
        // 5 RNAV_STAR_OR_STAR_COMMON_ROUTE
        // 6 RNAV_STAR_RUNWAY_TRANSITION
        // more 6

        for(int i = 0; i < approaches.size(); i++)
        {
          if(approaches.at(i).isCommonRoute)
          {
            starCommon = approaches.takeAt(i);
            break;
          }
        }
      }

      // Write all procedures - get a copy of the object since it is modified
      for(Procedure appr : std::as_const(approaches))
      {
        assignApproachIds(appr);
        // qDebug() << appr.legRecords;
        insertApproachQuery->bindAndExecRecord(appr.record);

        if(starCommon.isValid())
        {
          // Prefix the common route legs to the STAR
          assignApproachLegIds(starCommon.legRecords);
          insertApproachLegQuery->bindAndExecRecords(starCommon.legRecords);

          // Remove the IF of the STAR which will be replaced by the TF of the common route
          if(appr.legRecords.constFirst().value(QStringLiteral(":type")) == QStringLiteral("IF"))
            appr.legRecords.removeFirst();
        }

        // Write SID or STAR legs
        assignApproachLegIds(appr.legRecords);
        insertApproachLegQuery->bindAndExecRecords(appr.legRecords);

        if(sidCommon.isValid())
        {
          // Append the common route legs to the SID
          assignApproachLegIds(sidCommon.legRecords);
          insertApproachLegQuery->bindAndExecRecords(sidCommon.legRecords);
        }

        // Assign a new set of ids and write a duplicate of all transitions for the current approach
        for(Procedure& trans : transitions)
        {
          assignTransitionIds(trans);
          // qDebug() << trans.legRecords;
          insertTransitionQuery->bindAndExecRecord(trans.record);
          insertTransitionLegQuery->bindAndExecRecords(trans.legRecords);
        }
      }
    }
  }

  reset();
}

void ProcedureWriter::writeApproach(const ProcedureInput& line)
{
  // Ids are assigned later
  SqlRecord rec(APPROACH_RECORD);

  // rec.setValue(":approach_id", curApproachId);

  // Orphaned procedures where airport is not present in simulator - mostly X-Plane
  if(line.airportId == -1)
    rec.setNull(QStringLiteral(":airport_id"));
  else
    rec.setValue(QStringLiteral(":airport_id"), line.airportId);

  if(line.airportIdent.isEmpty())
    rec.setNull(QStringLiteral(":airport_ident"));
  else
    rec.setValue(QStringLiteral(":airport_ident"), line.airportIdent);

  QString suffix, rwy;
  QString apprIdent = line.sidStarAppIdent.trimmed();

  if(curRowCode == rc::APPROACH)
    rec.setValue(QStringLiteral(":arinc_name"), apprIdent);

  bool commonRoute = false;

  // Extract runway name "B" suffixes, "ALL" and CTL are ignored
  if(curRowCode == rc::SID || curRowCode == rc::STAR)
  {
    rwy = sidStarRunwayNameAndSuffix(line);
    rec.setValue(QStringLiteral(":arinc_name"), line.transIdent);

    if(curRowCode == rc::SID)
      commonRoute = atools::contains(static_cast<rt::SidRouteType>(curRouteType), rt::SID_COMMON);

    if(curRowCode == rc::STAR)
      commonRoute = atools::contains(static_cast<rt::StarRouteType>(curRouteType), rt::STAR_COMMON);
  }
  else
    apprRunwayNameAndSuffix(line, rwy, suffix);

  // Create the SID/STAR workaround as it is used by FSX/P3D
  QString type = procedureType(line);
  if(curRowCode == rc::STAR)
  {
    rec.setValue(QStringLiteral(":suffix"), QStringLiteral("A"));
    rec.setValue(QStringLiteral(":has_gps_overlay"), 1);
    rec.setValue(QStringLiteral(":type"), QStringLiteral("GPS"));
  }
  else if(curRowCode == rc::SID)
  {
    rec.setValue(QStringLiteral(":suffix"), QStringLiteral("D"));
    rec.setValue(QStringLiteral(":has_gps_overlay"), 1);
    rec.setValue(QStringLiteral(":type"), QStringLiteral("GPS"));
  }
  else
  {
    // Indicator Definition Field Content
    // Procedure Not Authorized for GPS or FMS Overlay, 0
    // Procedure Authorized for GPS Overlay, primary Navaids operating and monitored, 1
    // Procedure Authorized for GPS Overlay, primary Navaids installed, not monitored, 2
    // Procedure Authorized for GPS Overlay, Procedure Title includes "GPS", 3
    // Procedure Authorized for FMS Overlay, 4
    // Procedure Authorized for FMS and GPS Overlay, 5
    // RNAV (GPS), Procedure, can be flown with SBAS (WAAS), A
    // RNAV (GPS) Procedure, cannot be B flown with SBAS (WAAS), B
    // RNAV (GPS) Procedure, use of SBAS (WAAS) not specified, C
    // Stand Alone GPS Procedure P
    // Procedure Overlay Authorization unspecified, U

    // GPS overlay flag
    QString gpsIndicator = line.gnssFmsIndicator.trimmed();
    rec.setValue(QStringLiteral(":has_gps_overlay"),
                 type != QStringLiteral("GPS") && !gpsIndicator.isEmpty() && gpsIndicator != QStringLiteral("0") &&
                 gpsIndicator != QStringLiteral("U"));
    rec.setValue(QStringLiteral(":suffix"), suffix);
    rec.setValue(QStringLiteral(":type"), type);
  }

  rwy = rwy.trimmed();
  if(rwy.isEmpty())
  {
    rec.setValue(QStringLiteral(":runway_name"), QVariant(QMetaType::fromType<QString>()));
    rec.setValue(QStringLiteral(":runway_end_id"), QVariant(QMetaType::fromType<int>()));
  }
  else
  {
    rec.setValue(QStringLiteral(":runway_name"), rwy);
    rec.setValue(QStringLiteral(":runway_end_id"), airportIndex->getRunwayEndIdVar(line.airportId, rwy));
  }

  NavIdInfo navInfo = navaidTypeFix(line);
  // Might be reset later when writing the FAP leg
  rec.setValue(QStringLiteral(":fix_type"), navInfo.type);

  if(curRouteType == rc::APPROACH)
    rec.setValue(QStringLiteral(":fix_ident"), line.fixIdent);
  else // SID and STAR
    rec.setValue(QStringLiteral(":fix_ident"), line.sidStarAppIdent.trimmed());

  rec.setValue(QStringLiteral(":fix_region"), navInfo.region);

  rec.setValue(QStringLiteral(":aircraft_category"), line.aircraftCategory);

  approaches.append(Procedure(curRowCode, rec, commonRoute, line.sidStarAppIdent.trimmed()));

  writeApproachLeg(line);

  // not used: fix_airport_ident
  // not used: altitude
  // not used: heading
  // not used: missed_altitude
}

void ProcedureWriter::writeApproachLeg(const ProcedureInput& line)
{
  // Ids are assigned later
  SqlRecord rec(APPROACH_LEG_RECORD);

  QString waypointDescr = line.descCode;

  if(waypointDescr.size() > 3)
  {
    if(waypointDescr.at(3) == QChar('F') && curRowCode == rc::APPROACH)
    {
      NavIdInfo fafInfo = navaidTypeFix(line);
      // FAF - use this one to set the approach name
      approaches.last().record.setValue(QStringLiteral(":fix_type"), fafInfo.type);
      approaches.last().record.setValue(QStringLiteral(":fix_ident"), line.fixIdent);
      approaches.last().record.setValue(QStringLiteral(":fix_region"), fafInfo.region);
    }

    if(waypointDescr.at(3) != QChar(' ') && curRowCode == rc::APPROACH)
      rec.setValue(QStringLiteral(":approach_fix_type"), waypointDescr.at(3));
  }

  if(!writingMissedApproach)
    // First missed approach leg - remember state
    writingMissedApproach = waypointDescr.size() > 2 && waypointDescr.at(2) == 'M';
  rec.setValue(QStringLiteral(":is_missed"), writingMissedApproach);

  bindLeg(line, rec);

  approaches.last().legRecords.append(rec);
}

void ProcedureWriter::writeTransition(const ProcedureInput& line)
{
  // Ids are assigned later
  SqlRecord rec(TRANSITION_RECORD);

  NavIdInfo navInfo = navaidTypeFix(line);
  rec.setValue(QStringLiteral(":type"), QStringLiteral("F")); // set later to D if DME arc leg terminator
  rec.setValue(QStringLiteral(":fix_type"), navInfo.type);
  rec.setValue(QStringLiteral(":fix_ident"), line.transIdent.trimmed());
  rec.setValue(QStringLiteral(":fix_region"), navInfo.region);
  rec.setValue(QStringLiteral(":aircraft_category"), line.aircraftCategory);

  transitions.append(Procedure(curRowCode, rec, false /* common route */, line.sidStarAppIdent.trimmed()));

  writeTransitionLeg(line);

  // not used: fix_airport_ident
  // not useed  altitude
}

void ProcedureWriter::writeTransitionLeg(const ProcedureInput& line)
{
  // Ids are assigned later
  SqlRecord rec(TRANSITION_LEG_RECORD);

  if(line.pathTerm == QStringLiteral("AF"))
  {
    // Set transition type to DME arc if an AF leg is found
    SqlRecord& lastRec = transitions.last().record;

    // Arc to fix
    lastRec.setValue(QStringLiteral(":type"), QStringLiteral("D"));

    // not used: dme_airport_ident
    if(line.theta < atools::fs::common::INVALID_FLOAT)
      lastRec.setValue(QStringLiteral(":dme_radial"), line.theta);
    else
      lastRec.setNull(QStringLiteral(":dme_radial"));

    if(line.rho < atools::fs::common::INVALID_FLOAT)
      lastRec.setValue(QStringLiteral(":dme_distance"), line.rho);
    else
      lastRec.setNull(QStringLiteral(":dme_distance"));

    if(!line.recdNavaid.trimmed().isEmpty())
    {
      lastRec.setValue(QStringLiteral(":dme_ident"), line.recdNavaid.trimmed());
      lastRec.setValue(QStringLiteral(":dme_region"), line.recdRegion.trimmed());
    }
    else
      qWarning() << line.context << "No recommended navaid for AF leg";
  }

  bindLeg(line, rec);

  transitions.last().legRecords.append(rec);
}

void ProcedureWriter::bindLeg(const ProcedureInput& line, atools::sql::SqlRecord& rec)
{
  QString waypointDescr = line.descCode;

  // Overfly but not for runways
  bool overfly = waypointDescr.size() > 1 && (waypointDescr.at(1) == 'Y' || waypointDescr.at(1) == 'B') && waypointDescr.at(0) != 'G';

  rec.setValue(QStringLiteral(":type"), line.pathTerm);

  // Altitude
  QString altDescr = line.altDescr;
  bool swapAlt = false;
  bool altDescrValid = true;

  if(altDescr == QStringLiteral("+") || altDescr == QStringLiteral("-") || altDescr == QStringLiteral("B"))
    // Use same values - no mapping needed
    rec.setValue(QStringLiteral(":alt_descriptor"), altDescr);
  else if(altDescr == QStringLiteral("C"))
  {
    // At or above in second field - swap values and turn into at or above
    swapAlt = true;
    rec.setValue(QStringLiteral(":alt_descriptor"), QStringLiteral("+"));
  }
  else if(altDescr.isEmpty() || altDescr == QStringLiteral(" ") || altDescr == QStringLiteral("@"))
    // At altitude
    rec.setValue(QStringLiteral(":alt_descriptor"), QStringLiteral("A"));
  else if(altDescr == QStringLiteral("G") || altDescr == QStringLiteral("I"))
    // G Glide Slope altitude (MSL) specified in the second "Altitude" field and
    // "at" altitude specified in the first "Altitude" field on the FAF Waypoint in Precision Approach Coding
    // with electronic Glide Slope.
    // I Glide Slope Intercept Altitude specified in second "Altitude" field and
    // "at" altitude specified in first "Altitude" field on the FACF Waypoint in Precision Approach Coding
    // with electronic Glide Slope
    // Ignore Glide Slope altitude and turn into simple at restriction
    rec.setValue(QStringLiteral(":alt_descriptor"), QStringLiteral("A"));
  else if(altDescr == QStringLiteral("H") || altDescr == QStringLiteral("J"))
    // H Glide Slope Altitude (MSL) specified in second "Altitude" field and
    // "at or above" altitude specified in first "Altitude" field on the FAF Waypoint in Precision Approach Coding
    // with electronic Glide Slope
    // J Glide Slope Intercept Altitude specified in second "Altitude" field and
    // "at or above" altitude J specified in first "Altitude" field on the FACF Waypoint in Precision Approach Coding
    // with electronic Glide Slope "At" altitude on the coded vertical angle in the
    // Ignore Glide Slope altitude and turn into simple at or above restriction
    rec.setValue(QStringLiteral(":alt_descriptor"), QStringLiteral("+"));
  else if(altDescr == QStringLiteral("V"))
    // Ignore second altitude in step down fix waypoints and convert to at or above
    rec.setValue(QStringLiteral(":alt_descriptor"), QStringLiteral("+"));
  else if(altDescr == QStringLiteral("X"))
    // Ignore second altitude in step down fix waypoints and convert to at
    rec.setValue(QStringLiteral(":alt_descriptor"), QStringLiteral("A"));
  else if(altDescr == QStringLiteral("Y"))
    // Ignore second altitude in step down fix waypoints and convert to at or below
    rec.setValue(QStringLiteral(":alt_descriptor"), QStringLiteral("-"));
  else
  {
    altDescrValid = false;
    qWarning() << line.context << "Unexpected alt descriptor" << altDescr;
  }

  QString turnDir = line.turnDir.trimmed();
  if(turnDir == QStringLiteral("E"))
    rec.setValue(QStringLiteral(":turn_direction"), QStringLiteral("B"));
  else if(turnDir.size() == 1)
    rec.setValue(QStringLiteral(":turn_direction"), turnDir);
  // else null

  NavIdInfo navInfo = navaidTypeFix(line);

  rec.setValue(QStringLiteral(":fix_type"), navInfo.type);
  rec.setValue(QStringLiteral(":fix_ident"), line.fixIdent);
  rec.setValue(QStringLiteral(":fix_region"), navInfo.region);

  if(line.waypointPos.isValid() && rec.contains(QStringLiteral(":fix_lonx")) && rec.contains(QStringLiteral(":fix_laty")))
  {
    rec.setValue(QStringLiteral(":fix_lonx"), line.waypointPos.getLonX());
    rec.setValue(QStringLiteral(":fix_laty"), line.waypointPos.getLatY());
  }
  else
  {
    rec.setValue(QStringLiteral(":fix_lonx"), QVariant(QMetaType::fromType<double>()));
    rec.setValue(QStringLiteral(":fix_laty"), QVariant(QMetaType::fromType<double>()));
  }

  // not used: fix_airport_ident

  if(line.pathTerm == QStringLiteral("RF"))
  {
    if(!line.centerFixOrTaaPt.isEmpty())
    {
      NavIdInfo centerNavInfo = navaidType(line.context + ". RF recommended",
                                           QString(), line.centerSecCode, line.centerSubCode,
                                           line.centerFixOrTaaPt, line.centerIcaoCode, line.centerPos, line.airportPos);

      // Constant radius arc
      rec.setValue(QStringLiteral(":recommended_fix_type"), centerNavInfo.type);
      rec.setValue(QStringLiteral(":recommended_fix_ident"), line.centerFixOrTaaPt.trimmed());
      rec.setValue(QStringLiteral(":recommended_fix_region"), centerNavInfo.region);

      if(line.recdWaypointPos.isValid() &&
         rec.contains(QStringLiteral(":recommended_fix_lonx")) && rec.contains(QStringLiteral(":recommended_fix_laty")))
      {
        rec.setValue(QStringLiteral(":recommended_fix_lonx"), line.recdWaypointPos.getLonX());
        rec.setValue(QStringLiteral(":recommended_fix_laty"), line.recdWaypointPos.getLatY());
      }
      else
      {
        rec.setValue(QStringLiteral(":recommended_fix_lonx"), QVariant(QMetaType::fromType<double>()));
        rec.setValue(QStringLiteral(":recommended_fix_laty"), QVariant(QMetaType::fromType<double>()));
      }

    }
    else
      qWarning() << line.context << "No center fix for RF leg";
  }
  else if(!line.recdNavaid.isEmpty())
  {
    NavIdInfo recdNavInfo = navaidType(line.context + ". recommended",
                                       QString(), line.recdSecCode, line.recdSubCode, line.recdNavaid, line.recdRegion,
                                       line.recdWaypointPos, line.airportPos);

    rec.setValue(QStringLiteral(":recommended_fix_type"), recdNavInfo.type);
    rec.setValue(QStringLiteral(":recommended_fix_ident"), line.recdNavaid.trimmed());
    rec.setValue(QStringLiteral(":recommended_fix_region"), recdNavInfo.region);

    if(line.recdWaypointPos.isValid() && rec.contains(QStringLiteral(":recommended_fix_lonx")) &&
       rec.contains(QStringLiteral(":recommended_fix_laty")))
    {
      rec.setValue(QStringLiteral(":recommended_fix_lonx"), line.recdWaypointPos.getLonX());
      rec.setValue(QStringLiteral(":recommended_fix_laty"), line.recdWaypointPos.getLatY());
    }
    else
    {
      rec.setValue(QStringLiteral(":recommended_fix_lonx"), QVariant(QMetaType::fromType<double>()));
      rec.setValue(QStringLiteral(":recommended_fix_laty"), QVariant(QMetaType::fromType<double>()));
    }
  }
  // else null

  if(line.pathTerm == QStringLiteral("AF") && line.recdNavaid.trimmed().isEmpty())
    qWarning() << line.context << "No recommended fix for AF leg";

  rec.setValue(QStringLiteral(":is_flyover"), overfly);
  rec.setValue(QStringLiteral(":is_true_course"), 0); // Not used
  rec.setValue(QStringLiteral(":course"), line.magCourse);

  if(line.rnp < atools::fs::common::INVALID_FLOAT)
    rec.setValue(QStringLiteral(":rnp"), line.rnp);
  else
    rec.setNull(QStringLiteral(":rnp"));

  // time minutes
  rec.setValue(QStringLiteral(":time"), line.rteHoldTime);

  // distance nm
  rec.setValue(QStringLiteral(":distance"), line.rteHoldDist);

  if(line.theta < atools::fs::common::INVALID_FLOAT)
    rec.setValue(QStringLiteral(":theta"), line.theta);
  else
    rec.setNull(QStringLiteral(":theta"));

  if(line.rho < atools::fs::common::INVALID_FLOAT)
    rec.setValue(QStringLiteral(":rho"), line.rho);
  else
    rec.setNull(QStringLiteral(":rho"));

  if(altDescrValid)
  {
    rec.setValue(QStringLiteral(":altitude1"), altitudeFromStr(swapAlt ? line.altitude2 : line.altitude));
    rec.setValue(QStringLiteral(":altitude2"), altitudeFromStr(swapAlt ? line.altitude : line.altitude2));
  }

  // Speed limit ==========================
  if(line.speedLimit > 0)
  {
    rec.setValue(QStringLiteral(":speed_limit"), line.speedLimit);

    QString spdDescr = line.speedLimitDescr;
    if(spdDescr == QStringLiteral("+") || spdDescr == QStringLiteral("-"))
      rec.setValue(QStringLiteral(":speed_limit_type"), spdDescr);
    // else null means speed at

    if(!atools::contains(spdDescr, {QString(), QStringLiteral(" "), QStringLiteral("+"), QStringLiteral("-")}))
      qWarning() << line.context << "Invalid speed limit" << spdDescr;
  }
  // else null

  // Vertical angle ==========================
  if(!line.verticalAngle.isNull())
    rec.setValue(QStringLiteral(":vertical_angle"), line.verticalAngle);
  // else null

  // arinc_descr_code varchar(25), -- ARINC description code 5.17
  rec.setValue(QStringLiteral(":arinc_descr_code"), line.descCode);
}

float ProcedureWriter::altitudeFromStr(const QString& altStr)
{
  if(altStr.startsWith(QStringLiteral("FL")))
    // Simplify - turn flight levelt to feet
    return altStr.mid(2).toFloat() * 100.f;
  else
    return altStr.toFloat();
}

void ProcedureWriter::finish(const ProcedureInput& line)
{
  finishProcedure(line);

  updateAirportQuery->bindValue(QStringLiteral(":num"), numProcedures);
  updateAirportQuery->bindValue(QStringLiteral(":id"), line.airportId);
  updateAirportQuery->exec();
  numProcedures = 0;
}

void ProcedureWriter::reset()
{
  approaches.clear();
  transitions.clear();
  curRowCode = rc::NONE;
  writingMissedApproach = false;

  curSeqNo = 0;
  curRouteType = ' ';
  curRouteIdent.clear();
  curTransIdent.clear();
}

atools::fs::common::rc::RowCode ProcedureWriter::toRowCode(const ProcedureInput& line)
{
  QString code = line.rowCode;
  if(code == QStringLiteral("APPCH"))
    return rc::APPROACH;
  else if(code == QStringLiteral("SID"))
    return rc::SID;
  else if(code == QStringLiteral("STAR"))
    return rc::STAR;
  else if(code == QStringLiteral("RWY"))
    return rc::RWY;
  else if(code == QStringLiteral("PRDAT"))
    return rc::PRDAT;
  else
  {
    qWarning() << line.context << "Unexpexted row code" << code;
    return rc::NONE;
  }
}

QString ProcedureWriter::procedureType(const ProcedureInput& line)
{
  QString type;
  if(curRowCode == rc::APPROACH)
  {
    rt::ApproachRouteType apprRouteType = static_cast<rt::ApproachRouteType>(curRouteType);

    switch(apprRouteType)
    {
      // New types from X-Plane
      case rt::FLIGHT_MANAGEMENT_SYSTEM_FMS_APPROACH:
        type = QStringLiteral("FMS");
        break;
      case rt::INSTRUMENT_GUIDANCE_SYSTEM_IGS_APPROACH:
        type = QStringLiteral("IGS");
        break;
      case rt::GNSS_LANDING_SYSTEM_GLSAPPROACH:
        type = QStringLiteral("GNSS");
        break;
      case rt::TACAN_APPROACH:
        type = QStringLiteral("TCN");
        break;
      case rt::MICROWAVE_LANDING_SYSTEM_MLS_TYPE_A_APPROACH:
      case rt::MICROWAVE_LANDING_SYSTEM_MLS_TYPE_B_AND_C_APPROACH:
      case rt::MICROWAVE_LANDING_SYSTEM_MLS_APPROACH:
        type = QStringLiteral("MLS");
        break;
      case rt::PROCEDURE_WITH_CIRCLE_TOLAND_MINIMUMS:
        type = QStringLiteral("CTL");
        break;

      // FSX/P3D types
      case rt::GLOBAL_POSITIONING_SYSTEM_GPS_APPROACH:
        type = QStringLiteral("GPS");
        break;
      case rt::LOCALIZER_BACKCOURSE_APPROACH:
        type = QStringLiteral("LOCB");
        break;
      case rt::INSTRUMENT_LANDING_SYSTEM_ILS_APPROACH:
        type = QStringLiteral("ILS");
        break;
      case rt::LOCALIZER_ONLY_LOC_APPROACH:
        type = QStringLiteral("LOC");
        break;
      case rt::AREA_NAVIGATION_RNAV_APPROACH_NOTE_1:
        type = QStringLiteral("RNAV");
        break;
      case rt::NON_DIRECTIONAL_BEACON_NDB_APPROACH:
        type = QStringLiteral("NDB");
        break;
      case rt::VOR_APPROACH:
        type = QStringLiteral("VOR");
        break;
      case rt::VOR_APPROACH_USING_VORDME_VORTAC:
      case rt::VORDME_APPROACH:
        type = QStringLiteral("VORDME");
        break;
      case rt::NON_DIRECTIONAL_BEACON_AND_DME_NDB_AND_DME_APPROACH:
        type = QStringLiteral("NDBDME");
        break;
      case rt::LOCALIZER_DIRECTIONAL_AID_LDA_APPROACH:
        type = QStringLiteral("LDA");
        break;
      case rt::SIMPLIFIED_DIRECTIONAL_FACILITY_SDF_APPROACH:
        type = QStringLiteral("SDF");
        break;

      // case rt::MISSED_APPROACH:
      // case rt::PRIMARY_MISSED_APPROACH:
      // case rt::ENGINE_OUT_MISSED_APPROACH:
      default:
        qWarning() << line.context << "Unexpected approach route type" << curRouteType;

        if(curRouteType == ' ' || curRouteType == '\0')
          // No way to get a type
          type = QStringLiteral("UNKNOWN");
        else
          // Fall back to single character
          type = atools::charToStr(curRouteType);
        break;
    }
  }
  else if(curRowCode == rc::SID || curRowCode == rc::STAR)
    type = QStringLiteral("GPS");

  return type;
}

ProcedureWriter::NavIdInfo ProcedureWriter::navaidTypeFix(const ProcedureInput& line)
{
  return navaidType(line.context + ". fix_type", line.descCode, line.secCode, line.subCode, line.fixIdent,
                    line.region, line.waypointPos, line.airportPos);

}

ProcedureWriter::NavIdInfo ProcedureWriter::navaidType(const QString& context, const QString& descCode,
                                                       const QString& sectionCode,
                                                       const QString& subSectionCode, const QString& ident,
                                                       const QString& region,
                                                       const atools::geo::PosD& pos,
                                                       const atools::geo::PosD& airportPos)
{
  if(ident.isEmpty())
    return NavIdInfo();

  if(sectionCode.trimmed().isEmpty())
  {
    // No section codes - try desctiption
    QString descr = descCode;
    if(!descr.isEmpty())
    {
      dc::WaypointDescriptionCode1 code1 = static_cast<dc::WaypointDescriptionCode1>(descr.at(0).toLatin1());

      switch(code1)
      {
        case atools::fs::common::dc::HELIPORT_AS_WAYPOINT:
        case atools::fs::common::dc::AIRPORT_AS_WAYPOINT:
          return NavIdInfo(QStringLiteral("A"), region);

        case atools::fs::common::dc::RUNWAY_AS_WAYPOINT:
          return NavIdInfo(QStringLiteral("R"), region);

        case atools::fs::common::dc::NDB_NAVAID_AS_WAYPOINT:
          return NavIdInfo(QStringLiteral("N"), region);

        case atools::fs::common::dc::VHF_NAVAID_AS_WAYPOINT:
          return NavIdInfo(QStringLiteral("V"), region);

        // Values below can refer to waypoint, VOR or NDB resolve them below
        case atools::fs::common::dc::NON_ESSENTIAL_WAYPOINT:
        case atools::fs::common::dc::TRANSITION_ESSENTIAL_WAYPOINT:
        case atools::fs::common::dc::ESSENTIAL_WAYPOINT:
        case atools::fs::common::dc::OFF_AIRWAY_WAYPOINT:
        case atools::fs::common::dc::PHANTOM_WAYPOINT:
          break;
      }
    }

    // Fall back to airport position
    atools::geo::PosD searchPos = pos.isValid() && !pos.isNull() ? pos : airportPos;

    if(searchPos.isValid() && !searchPos.isNull())
    {
      NavIdInfo inf;

      // Try an exact and faster coordinate search first
      // For that we need double coordinate values
      findFix(findWaypointExactQuery, ident, region, searchPos);
      if(findWaypointExactQuery->next())
      {
        inf.type = findWaypointExactQuery->valueStr("type");
        inf.region = findWaypointExactQuery->valueStr("region");
      }
      else
      {
        // Nothing found at position - look in vicinity for waypoints
        findFix(findWaypointQuery, ident, region, searchPos);
        if(findWaypointQuery->next())
        {
          inf.type = findWaypointQuery->valueStr("type");
          inf.region = findWaypointQuery->valueStr("region");
        }
        else
        {
          // Nothing found at position - look at position for ILS
          findFix(findIlsExactQuery, ident, region, searchPos);
          if(findIlsExactQuery->next())
            // Do not set type for ILS
            inf.region = findIlsExactQuery->valueStr("region");
          else
          {
            // Nothing found at position - look in vicinity for ILS
            findFix(findIlsQuery, ident, region, searchPos);
            if(findIlsQuery->next())
              // Do not set type for ILS
              inf.region = findIlsQuery->valueStr("region");
          }
        }
      }

      if(!inf.type.isEmpty())
      {
        // N = NDB, OA = off airway, V = VOR, WN = named waypoint, WU = unnamed waypoint, G = GBAS approach station
        if(inf.type == QStringLiteral("WN") || inf.type == QStringLiteral("WU"))
        {
          inf.type = QStringLiteral("W");
          return inf;
        }
        else if(inf.type == QStringLiteral("N") || inf.type == QStringLiteral("V"))
          return inf;
      }
    }
    else if(!pos.isNull())
      qWarning() << context << "Cannot find navaid type for" << ident << "/" << region << pos;

    return NavIdInfo();
  }

  sc::SectionCode sc = static_cast<sc::SectionCode>(sectionCode.at(0).toLatin1());

  // LOCALIZER:  "L"
  // NONE:  "NONE"
  // VOR:  "V"
  // NDB:  "N"
  // TERMINAL_NDB:  "TN"
  // WAYPOINT:  "W"
  // TERMINAL_WAYPOINT:  "TW"
  // RUNWAY:  "R"

  if(sc == sc::AIRPORT)
  {
    sc::AirportSubSectionCode subSec =
      static_cast<sc::AirportSubSectionCode>(subSectionCode.at(0).toLatin1());

    switch(subSec)
    {
      case atools::fs::common::sc::TERMINAL_WAYPOINTS:
        return NavIdInfo(QStringLiteral("TW"), region);

      case atools::fs::common::sc::RUNWAYS:
        return NavIdInfo(QStringLiteral("R"), region);

      case atools::fs::common::sc::LOCALIZER_MARKER:
      case atools::fs::common::sc::LOCALIZER_GLIDE_SLOPE:
        return NavIdInfo(QStringLiteral("L"), region);

      case atools::fs::common::sc::TERMINAL_NDB:
        return NavIdInfo(QStringLiteral("TN"), region);

      case atools::fs::common::sc::REFERENCE_POINTS:
        return NavIdInfo(QStringLiteral("A"), region); // Airport reference point - new with X-Plane

      case atools::fs::common::sc::GLS_STATION:
        // ignore these
        return NavIdInfo();

      case atools::fs::common::sc::PATH_POINT:
      case atools::fs::common::sc::GATES:
      case atools::fs::common::sc::SIDS:
      case atools::fs::common::sc::STARS:
      case atools::fs::common::sc::APPROACH_PROCEDURES:
      case atools::fs::common::sc::TAA:
      case atools::fs::common::sc::MLS:
      case atools::fs::common::sc::FLT_PLANNING_ARR_DEP:
      case atools::fs::common::sc::MSA:
      case atools::fs::common::sc::AP_COMMUNICATIONS:
        break;

    }
    qWarning() << context << "Unexpected airport section" << sectionCode << "sub" << subSectionCode;
  }
  else if(sc == sc::ENROUTE)
  {
    sc::EnrouteSubSectionCode subSec =
      static_cast<sc::EnrouteSubSectionCode>(subSectionCode.at(0).toLatin1());
    switch(subSec)
    {
      case atools::fs::common::sc::WAYPOINTS:
        return NavIdInfo(QStringLiteral("W"), region);

      case atools::fs::common::sc::VORDME:
        return NavIdInfo(QStringLiteral("V"), region);

      case atools::fs::common::sc::AIRWAY_MARKERS:
      case atools::fs::common::sc::HOLDING_PATTERNS:
      case atools::fs::common::sc::AIRWAYS_AND_ROUTES:
      case atools::fs::common::sc::PREFERRED_ROUTES:
      case atools::fs::common::sc::AIRWAY_RESTRICTIONS:
      case atools::fs::common::sc::ER_COMMUNICATIONS:
        break;
    }
    qWarning() << context << "Unexpected enroute section" << sectionCode << "sub" << subSectionCode;
  }
  else if(sc == sc::NAVAID)
  {
    sc::NavaidSubSectionCode subSec = static_cast<sc::NavaidSubSectionCode>(subSectionCode.at(0).toLatin1());

    switch(subSec)
    {
      case atools::fs::common::sc::VHF:
        return NavIdInfo(QStringLiteral("V"), region);

      case atools::fs::common::sc::NDB:
        return NavIdInfo(QStringLiteral("N"), region);
    }
    qWarning() << context << "Unexpected navaid section" << sectionCode << "sub" << subSectionCode;
  }
  else
    qWarning() << context << "Unexpected section" << sectionCode;

  return NavIdInfo();
}

void ProcedureWriter::findFix(atools::sql::SqlQuery *query, const QString& ident, const QString& region,
                              const atools::geo::PosD& pos) const
{
  query->bindValue(QStringLiteral(":ident"), ident);
  query->bindValue(QStringLiteral(":region"), region.isEmpty() ? QStringLiteral("%") : region);
  query->bindValue(QStringLiteral(":lonx"), pos.getLonX());
  query->bindValue(QStringLiteral(":laty"), pos.getLatY());
  query->exec();
}

QString ProcedureWriter::sidStarRunwayNameAndSuffix(const ProcedureInput& line)
{
  QString ident = line.transIdent;
  if(ident.startsWith(QStringLiteral("RW")))
  {
    // Get designator if there is one
    QString desig = ident.size() > 4 ? ident.at(4) : QString();

    if(ident.at(2).isDigit() && ident.at(3).isDigit() && !atools::contains(desig,
                                                                           {QString(), QStringLiteral("L"), QStringLiteral("R"),
                                                                            QStringLiteral("C"), QStringLiteral("-"), QStringLiteral("B"),
                                                                            QStringLiteral("T")}))
      qWarning() << line.context << "Invalid designator" << desig;

    if(desig != QStringLiteral("B")) // B = multiple runways with same number but different designator
    {
      if(!desig.isEmpty() && desig != QStringLiteral("L") && desig != QStringLiteral("R") && desig != QStringLiteral("C") &&
         desig != QStringLiteral("T"))
        desig.clear();

      // Get runway number only if valid to ignore all CVOR, NDEA, etc. approaches
      if(ident.at(2).isDigit() && ident.at(3).isDigit())
        return ident.mid(2, 2) + desig;
    }
  }
  return QString();
}

void ProcedureWriter::apprRunwayNameAndSuffix(const ProcedureInput& line, QString& runway, QString& suffix)
{
  const QString ident = line.sidStarAppIdent;
  suffix.clear();

  // Get runway
  QString rw = ident.mid(1, 2);
  QString desig = ident.mid(3, 1);

  // Check for digits to get runway - circle to land if no runway given
  bool hasRunway = rw.size() == 2 && rw.at(0).isDigit() && rw.at(1).isDigit() &&
                   atools::contains(desig,
                                    {QString(), QStringLiteral("L"), QStringLiteral("R"), QStringLiteral("C"), QStringLiteral("-"),
                                     QStringLiteral("B"), QStringLiteral("T")});

  if(hasRunway && ident.size() >= 4 && ident.at(3).isDigit())
    // Full number like P168 - this is not a runway
    hasRunway = false;

  if(hasRunway)
  {
    // TODO consider true designator
    // D26 D26-1 D26-2 D26-Y D26-Z D26LZ
    // I26L, B08R, R29, V01L, N35 L16RA, L16RB, V08-A, V08-B I18L1, I18L2, N08T R35-Y, R35-Z
    if(desig == QStringLiteral("L") || desig == QStringLiteral("R") || desig == QStringLiteral("C") || desig == QStringLiteral("T"))
      // Add only real designators and not QStringLiteral("B")
      runway = rw + desig;
    else
      runway = rw;
    suffix = ident.mid(4, 1);
  }
  else
  {
    // CNDB CNDM CVDM CVOR CVORY
    // VDME VDMF VDMH VOR1 VOR2 VORA VORB
    // VORA, VORB VOR-A, VOR-B, NDBB, CVOR, VDMA, LOCD,BI P168, NDAT (NDB, DME, Alpha, True), NDB-1, NDB-2
    if(ident.contains(QStringLiteral("-")))
      // Suffix after dash
      suffix = ident.section('-', 1, 1);
    else if(ident.startsWith(QStringLiteral("C")))
      // Special case for CNDB or CVOR
      suffix = ident.mid(4, 1);
    else
      suffix = ident.mid(3, 1);
  }
}

void ProcedureWriter::initQueries()
{
  deInitQueries();

  SqlUtil util(&db);

  insertApproachQuery = new SqlQuery(db);
  insertApproachQuery->prepare(util.buildInsertStatement(QStringLiteral("approach")));

  insertApproachLegQuery = new SqlQuery(db);
  insertApproachLegQuery->prepare(util.buildInsertStatement(QStringLiteral("approach_leg")));

  insertTransitionQuery = new SqlQuery(db);
  insertTransitionQuery->prepare(util.buildInsertStatement(QStringLiteral("transition")));

  insertTransitionLegQuery = new SqlQuery(db);
  insertTransitionLegQuery->prepare(util.buildInsertStatement(QStringLiteral("transition_leg")));

  updateAirportQuery = new SqlQuery(db);
  updateAirportQuery->prepare(QStringLiteral("update airport set num_approach = :num where airport_id = :id"));

  findWaypointExactQuery = new SqlQuery(db);
  findWaypointExactQuery->prepare("select type, region from waypoint where ident = :ident and region like :region and "
                                  "lonx = :lonx and laty = :laty");

  findWaypointQuery = new SqlQuery(db);
  findWaypointQuery->prepare("select type, region from waypoint where ident = :ident and region like :region and "
                             "(abs(lonx - :lonx) + abs(laty - :laty)) < 0.1 "
                             "order by abs(lonx - :lonx) + abs(laty - :laty)");

  findIlsExactQuery = new SqlQuery(db);
  findIlsExactQuery->prepare("select type, region from ils where ident = :ident and region like :region and "
                             "lonx = :lonx and laty = :laty");

  findIlsQuery = new SqlQuery(db);
  findIlsQuery->prepare("select type, region from ils where ident = :ident and region like :region and "
                        "(abs(lonx - :lonx) + abs(laty - :laty)) < 0.1 "
                        "order by abs(lonx - :lonx) + abs(laty - :laty)");
}

void ProcedureWriter::deInitQueries()
{
  delete insertApproachQuery;
  insertApproachQuery = nullptr;

  delete insertApproachLegQuery;
  insertApproachLegQuery = nullptr;

  delete insertTransitionQuery;
  insertTransitionQuery = nullptr;

  delete insertTransitionLegQuery;
  insertTransitionLegQuery = nullptr;

  delete updateAirportQuery;
  updateAirportQuery = nullptr;

  delete findWaypointQuery;
  findWaypointQuery = nullptr;

  delete findWaypointExactQuery;
  findWaypointExactQuery = nullptr;

  delete findIlsQuery;
  findIlsQuery = nullptr;

  delete findIlsExactQuery;
  findIlsExactQuery = nullptr;
}

} // namespace common
} // namespace fs
} // namespace atools
