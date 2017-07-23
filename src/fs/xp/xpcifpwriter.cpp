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

#include "fs/xp/xpcifpwriter.h"

#include "fs/xp/xpairportindex.h"
#include "fs/progresshandler.h"
#include "atools.h"

#include "sql/sqlutil.h"

#pragma GCC diagnostic ignored "-Wswitch-enum"

using atools::sql::SqlQuery;
using atools::sql::SqlUtil;

namespace atools {
namespace fs {
namespace xp {

/* *INDENT-OFF* */
enum ProcedureFieldIndex
{
  PROC_ROW_CODE,          // All: 4.1.9.1.
  SEQ_NR,                 //  1 27-29   5.12
  RT_TYPE,                //  2 20      5.7
  SID_STAR_APP_IDENT,     //  3 14-19   5.9&5.10
  TRANS_IDENT,            //  4 21-25   5.11
  FIX_IDENT,              //  5 30-34   5.13
  ICAO_CODE,              //  6 35-36   5.14
  SEC_CODE,               //  7 37      5.4
  SUB_CODE,               //  8 38      5.5
  DESC_CODE,              //  9 40-43   5.17
  TURN_DIR,               // 10 44      5.20
  RNP,                    // 11 45-47   5.211
  PATH_TERM,              // 12 48-49   5.21
  TDV,                    // 13 50      5.22
  RECD_NAVAID,            // 14 51-54   5.23
  RECD_ICAO_CODE,         // 15 55-56   5.14
  RECD_SEC_CODE,          // 16 79      5.4
  RECD_SUB_CODE,          // 17 80      5.5
  ARC_RADIUS,             // 18 57-62   5.204
  THETA,                  // 19 63-66   5.24
  RHO,                    // 20 67-70   5.25
  MAG_CRS,                // 21 71-74   5.26
  RTE_DIST_HOLD_DIST_TIME,// 22 75-78   5.27
  ALT_DESCR,              // 23 83      5.29
  ALTITUDE,               // 24 85-89   5.30
  ALTITUDE2,              // 25 90-94   5,30
  TRANS_ALT,              // 26 95-99   5.53
  SPD_LIMIT_DESCR,        // 27 118     5.261
  SPEED_LIMIT,            // 28 100-102 5.72
  VERT_ANGLE,             // 29 103-106 5.70
  UNKNOWN,                // 30 121-123 5.293 not in
  CENTER_FIX_OR_TAA_PT,   // 31 107-111 5.144/5.271
  CENTER_ICAO_CODE,       // 32 113-114 5.14
  CENTER_SEC_CODE,        // 33 115     5.4
  CENTER_SUB_CODE,        // 34 116     5.5
  MULTI_CD,               // 35 112     5.130/5.272
  GNSS_FMS_IND,           // 36 117     5.222
  RTE_QUAL1,              // 37 119     5.7
  RTE_QUAL2               // 38 120     5.7
};
/* *INDENT-ON* */

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

// Section code P - aiport
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

XpCifpWriter::XpCifpWriter(atools::sql::SqlDatabase& sqlDb, XpAirportIndex *xpAirportIndex,
                           const NavDatabaseOptions& opts, ProgressHandler *progressHandler)
  : XpWriter(sqlDb, opts, progressHandler), airportIndex(xpAirportIndex)
{
  initQueries();
}

XpCifpWriter::~XpCifpWriter()
{
  deInitQueries();
}

void XpCifpWriter::write(const QStringList& line, const XpWriterContext& context)
{
  rc::RowCode rowCode = toRowCode(line.at(PROC_ROW_CODE), context);

  if(rowCode == rc::APPROACH || rowCode == rc::SID || rowCode == rc::STAR)
  {
    int seqNo = line.at(SEQ_NR).toInt();
    char routeType = line.at(RT_TYPE).at(0).toLatin1();
    QString routeIdent = line.at(SID_STAR_APP_IDENT);

    bool procedure = curSeqNo >= seqNo || rowCode != curRowCode || routeType != curRouteType ||
                     routeIdent != curRouteIdent;

    if(procedure)
      finishProcedure();

    curRowCode = rowCode;
    curSeqNo = seqNo;
    curRouteType = routeType;
    curRouteIdent = routeIdent;

    if(procedure)
      writeProcedure(line, context);
    else
      writeProcedureLeg(line, context);
  }
  // ignore RWY and PRDAT
}

void XpCifpWriter::writeProcedure(const QStringList& line, const XpWriterContext& context)
{
  switch(curRowCode)
  {
    case rc::APPROACH:
      {
        rt::ApproachRouteType art = static_cast<rt::ApproachRouteType>(curRouteType);
        if(art == rt::APPROACH_TRANSITION)
        {
          writingTransition = true;
          bindTransition(line, context);
          writeTransitionLeg(line, context);
        }
        else
        {
          writingApproach = true;
          bindApproach(line, context);
          writeApproachLeg(line, context);
        }
      }
      break;

    case rc::SID:
      // TODO SID
      // first approach then transition
      // duplicate all for each runway before writing approach
      break;

    case rc::STAR:
      // TODO STAR
      // first transition then approach
      // duplicate all for each runway in approach before writing transitions
      break;

    case rc::RWY:
    case rc::NONE:
    case rc::PRDAT:
      break;

  }
}

void XpCifpWriter::writeProcedureLeg(const QStringList& line, const XpWriterContext& context)
{
  switch(curRowCode)
  {
    case rc::APPROACH:
      {
        rt::ApproachRouteType art = static_cast<rt::ApproachRouteType>(curRouteType);
        if(art == rt::APPROACH_TRANSITION)
          writeTransitionLeg(line, context);
        else
          writeApproachLeg(line, context);
      }
      break;

    case rc::SID:
      // TODO SID
      break;

    case rc::STAR:
      // TODO STAR
      break;

    case rc::RWY:
    case rc::NONE:
    case rc::PRDAT:
      break;
  }
}

void XpCifpWriter::finishProcedure()
{
  if(writingTransition)
  {
    insertTransitionQuery->exec();
    insertTransitionQuery->clearBoundValues();
    curRowCode = rc::NONE;
    writingTransition = false;
  }

  if(writingApproach)
  {
    insertApproachQuery->exec();
    insertApproachQuery->clearBoundValues();
    curRowCode = rc::NONE;
    writingApproach = false;
    writingMissedApproach = false;
    curApproachId++;
  }
}

void XpCifpWriter::bindApproach(const QStringList& line, const XpWriterContext& context)
{
  QString type = procedureType(curRouteType, context);

  insertApproachQuery->bindValue(":approach_id", curApproachId);
  insertApproachQuery->bindValue(":airport_id", context.cifpAirportId);
  insertApproachQuery->bindValue(":airport_ident", context.cifpAirportIdent);

  QString suffix, rwy;
  QString apprIdent = line.at(SID_STAR_APP_IDENT);

  // TODO KSEA RW16B RW34B
  // Undocumented values
  // VDM1 VDM2 VDM3 VDMA VDMB VDMC VDMD VDME VDMF VDMH VOR1 VOR2 VORA VORB VORC VORD VORE VORY VRBT
  if(apprIdent.startsWith("RNV") || apprIdent.startsWith("VDM") || apprIdent.startsWith("VOR") ||
     apprIdent.startsWith("VRB") || apprIdent.startsWith("NDM") || apprIdent.startsWith("NDB"))
  {
    if(apprIdent.size() > 3)
      suffix = apprIdent.at(3);
  }
  else if(apprIdent.at(3) == '-')
  {
    rwy = apprIdent.mid(1, 3);
    if(apprIdent.size() > 3)
      suffix = apprIdent.at(3);
  }
  else
  {
    if(apprIdent.size() > 4)
      suffix = apprIdent.mid(4, 1);
    rwy = apprIdent.mid(1, 3);
  }

  rwy = rwy.trimmed();
  if(rwy.isEmpty())
    insertApproachQuery->bindValue(":runway_name", QVariant(QVariant::String));
  else
  {
    insertApproachQuery->bindValue(":runway_name", rwy);
    insertApproachQuery->bindValue(":runway_end_id",
                                   airportIndex->getRunwayEndId(context.cifpAirportIdent, rwy));
  }

  insertApproachQuery->bindValue(":type", type);
  insertApproachQuery->bindValue(":suffix", suffix);

  QString gpsIndicator = line.at(GNSS_FMS_IND);
  insertApproachQuery->bindValue(":has_gps_overlay", gpsIndicator != "0" && gpsIndicator != "U");

  // Reset later when writing the FAP leg
  insertApproachQuery->bindValue(":fix_type", navaidType(line.at(SEC_CODE), line.at(SUB_CODE), context));
  insertApproachQuery->bindValue(":fix_ident", line.at(FIX_IDENT));
  insertApproachQuery->bindValue(":fix_region", line.at(ICAO_CODE));

  // not used: fix_airport_ident
  // not used: altitude
  // not used: heading
  // not used: missed_altitude
}

void XpCifpWriter::writeApproachLeg(const QStringList& line, const XpWriterContext& context)
{
  if(!context.includeApproachLeg)
    return;

  insertApproachLegQuery->bindValue(":approach_leg_id", ++curApproachLegId);
  insertApproachLegQuery->bindValue(":approach_id", curApproachId);

  QString waypointDescr = line.at(DESC_CODE);

  if(waypointDescr.at(3) == "F")
  {
    // FAF - use this one to set the approach name
    insertApproachQuery->bindValue(":fix_type", navaidType(line.at(SEC_CODE), line.at(SUB_CODE), context));
    insertApproachQuery->bindValue(":fix_ident", line.at(FIX_IDENT));
    insertApproachQuery->bindValue(":fix_region", line.at(ICAO_CODE));
  }

  if(!writingMissedApproach)
    // First missed approach leg
    writingMissedApproach = waypointDescr.at(2) == 'M';
  insertApproachLegQuery->bindValue(":is_missed", writingMissedApproach);

  bindLeg(line, insertApproachLegQuery, context);

  insertApproachLegQuery->exec();
  insertApproachLegQuery->clearBoundValues();
}

void XpCifpWriter::bindTransition(const QStringList& line, const XpWriterContext& context)
{
  insertTransitionQuery->bindValue(":transition_id", ++curTransitionId);
  insertTransitionQuery->bindValue(":approach_id", curApproachId);
  insertTransitionQuery->bindValue(":type", "F"); // set to D if DME arc leg terminator

  insertTransitionQuery->bindValue(":fix_type", navaidType(line.at(SEC_CODE), line.at(SUB_CODE), context));
  insertTransitionQuery->bindValue(":fix_ident", line.at(TRANS_IDENT));
  insertTransitionQuery->bindValue(":fix_region", line.at(ICAO_CODE));

  // not used: fix_airport_ident
  // not useed  altitude
}

void XpCifpWriter::writeTransitionLeg(const QStringList& line, const XpWriterContext& context)
{
  if(!context.includeApproachLeg)
    return;

  insertTransitionLegQuery->bindValue(":transition_leg_id", ++curTransitionLegId);
  insertTransitionLegQuery->bindValue(":transition_id", curTransitionId);

  if(line.at(PATH_TERM) == "AF")
  {
    // Arc to fix

    // not used: dme_airport_ident
    insertTransitionQuery->bindValue(":dme_radial", line.at(THETA).toFloat() / 10.f);
    insertTransitionQuery->bindValue(":dme_distance", line.at(RHO).toFloat());

    if(!line.at(RECD_NAVAID).trimmed().isEmpty())
    {
      insertTransitionQuery->bindValue(":dme_ident", line.at(RECD_NAVAID).trimmed());
      insertTransitionQuery->bindValue(":dme_region", line.at(RECD_ICAO_CODE));
    }
    else
      qWarning() << context.messagePrefix() << "No recommended navaid fro AF leg";
  }
  else if(line.at(PATH_TERM) == "AF")
    insertTransitionQuery->bindValue(":type", "D");

  bindLeg(line, insertTransitionLegQuery, context);

  insertTransitionLegQuery->exec();
  insertTransitionLegQuery->clearBoundValues();
}

void XpCifpWriter::bindLeg(const QStringList& line, atools::sql::SqlQuery *query, const XpWriterContext& context)
{
  QString waypointDescr = line.at(DESC_CODE);

  // Overfly but not for runways
  bool overfly = (waypointDescr.at(1) == 'Y' || waypointDescr.at(1) == 'B') && waypointDescr.at(0) != 'G';

  query->bindValue(":type", line.at(PATH_TERM));

  QString altDescr = line.at(ALT_DESCR);
  if(altDescr == "+" || altDescr == "-" || altDescr == "B")
    query->bindValue(":alt_descriptor", altDescr);
  else if(altDescr == " " || altDescr == "@")
    query->bindValue(":alt_descriptor", "A");
  // else null

  QString turnDir = line.at(TURN_DIR);
  if(turnDir == "E")
    query->bindValue(":turn_direction", "B");
  else if(turnDir.size() == 1)
    query->bindValue(":turn_direction", turnDir);
  // else null

  query->bindValue(":fix_type", navaidType(line.at(SEC_CODE), line.at(SUB_CODE), context));
  query->bindValue(":fix_ident", line.at(FIX_IDENT));
  query->bindValue(":fix_region", line.at(ICAO_CODE));
  // not used: fix_airport_ident

  if(line.at(PATH_TERM) == "RF")
  {
    if(!line.at(CENTER_FIX_OR_TAA_PT).trimmed().isEmpty())
    {
      // Constant radius arc
      query->bindValue(":recommended_fix_type",
                       navaidType(line.at(CENTER_SEC_CODE), line.at(CENTER_SUB_CODE), context));
      query->bindValue(":recommended_fix_ident", line.at(CENTER_FIX_OR_TAA_PT).trimmed());
      query->bindValue(":recommended_fix_region", line.at(CENTER_ICAO_CODE));
    }
    else
      qWarning() << context.messagePrefix() << "No center fix for RF leg";
  }
  else if(!line.at(RECD_NAVAID).trimmed().isEmpty())
  {
    query->bindValue(":recommended_fix_type", navaidType(line.at(RECD_SEC_CODE), line.at(RECD_SUB_CODE), context));
    query->bindValue(":recommended_fix_ident", line.at(RECD_NAVAID).trimmed());
    query->bindValue(":recommended_fix_region", line.at(RECD_ICAO_CODE));
  }
  // else null

  query->bindValue(":is_flyover", overfly);
  query->bindValue(":is_true_course", 0); // Not used
  query->bindValue(":course", line.at(MAG_CRS).toFloat() / 10.f);

  QString distTime = line.at(RTE_DIST_HOLD_DIST_TIME);
  if(distTime.startsWith("T"))
    // time
    query->bindValue(":time", distTime.mid(1).toFloat() / 10.f);
  else
    // distance
    query->bindValue(":distance", distTime.toFloat() / 10.f);

  query->bindValue(":theta", line.at(THETA).toFloat() / 10.f);
  query->bindValue(":rho", line.at(RHO).toFloat());
  query->bindValue(":altitude1", line.at(ALTITUDE).toFloat());
  query->bindValue(":altitude2", line.at(ALTITUDE2).toFloat());

  int spdLimit = line.at(SPEED_LIMIT).toInt();
  if(spdLimit > 0)
  {
    query->bindValue(":speed_limit", spdLimit);

    QString spdDescr = line.at(SPD_LIMIT_DESCR);
    if(spdDescr == "+" || spdDescr == "-")
      query->bindValue(":speed_limit_type", spdDescr);
    else if(spdDescr == " " || spdDescr == "@")
      query->bindValue(":speed_limit_type", "A");
    // else null
  }
  // else null
}

void XpCifpWriter::finish(const XpWriterContext& context)
{
  Q_UNUSED(context);
  finishProcedure();
}

atools::fs::xp::rc::RowCode XpCifpWriter::toRowCode(const QString& code, const XpWriterContext& context)
{
  if(code == "APPCH")
    return rc::APPROACH;
  else if(code == "SID")
    return rc::SID;
  else if(code == "STAR")
    return rc::STAR;
  else if(code == "RWY")
    return rc::RWY;
  // else if(code == "PRDAT")
  // return rc::PRDAT;
  else
  {
    qWarning() << context.messagePrefix() << "Unexpexted row code" << code;
    return rc::NONE;
  }
}

QString XpCifpWriter::procedureType(char routeType, const XpWriterContext& context)
{
  QString type;
  rt::ApproachRouteType apprRouteType = static_cast<rt::ApproachRouteType>(routeType);

  switch(apprRouteType)
  {
    // New types from X-Plane
    case rt::FLIGHT_MANAGEMENT_SYSTEM_FMS_APPROACH:
      type = "FMS";
      break;
    case rt::INSTRUMENT_GUIDANCE_SYSTEM_IGS_APPROACH:
      type = "IGS";
      break;
    case rt::GNSS_LANDING_SYSTEM_GLSAPPROACH:
      type = "GNSS";
      break;
    case rt::TACAN_APPROACH:
      type = "TCN";
      break;
    case rt::MICROWAVE_LANDING_SYSTEM_MLS_TYPE_A_APPROACH:
    case rt::MICROWAVE_LANDING_SYSTEM_MLS_TYPE_B_AND_C_APPROACH:
    case rt::MICROWAVE_LANDING_SYSTEM_MLS_APPROACH:
      type = "MLS";
      break;
    case rt::PROCEDURE_WITH_CIRCLE_TOLAND_MINIMUMS:
      type = "CTL";
      break;

    // FSX/P3D types
    case rt::GLOBAL_POSITIONING_SYSTEM_GPS_APPROACH:
      type = "GPS";
      break;
    case rt::LOCALIZER_BACKCOURSE_APPROACH:
      type = "LOCB";
      break;
    case rt::INSTRUMENT_LANDING_SYSTEM_ILS_APPROACH:
      type = "ILS";
      break;
    case rt::LOCALIZER_ONLY_LOC_APPROACH:
      type = "LOC";
      break;
    case rt::AREA_NAVIGATION_RNAV_APPROACH_NOTE_1:
      type = "RNAV";
      break;
    case rt::NON_DIRECTIONAL_BEACON_NDB_APPROACH:
      type = "NDB";
      break;
    case rt::VOR_APPROACH:
      type = "VOR";
      break;
    case rt::VOR_APPROACH_USING_VORDME_VORTAC:
    case rt::VORDME_APPROACH:
      type = "VORDME";
      break;
    case rt::NON_DIRECTIONAL_BEACON_AND_DME_NDB_AND_DME_APPROACH:
      type = "NDBDME";
      break;
    case rt::LOCALIZER_DIRECTIONAL_AID_LDA_APPROACH:
      type = "LDA";
      break;
    case rt::SIMPLIFIED_DIRECTIONAL_FACILITY_SDF_APPROACH:
      type = "SDF";
      break;

    case rt::MISSED_APPROACH:
    case rt::PRIMARY_MISSED_APPROACH:
    case rt::ENGINE_OUT_MISSED_APPROACH:
    default:
      qWarning() << context.messagePrefix() << "Unexpected approach route type" << curRouteType;
      break;
  }
  return type;
}

QString XpCifpWriter::navaidType(const QString& sectionCode, const QString& subSectionCode,
                                 const XpWriterContext& context)
{
  if(sectionCode.trimmed().isEmpty())
    return QString();

  sc::SectionCode sc = static_cast<sc::SectionCode>(sectionCode.at(0).toLatin1());

  // case ap::fix::LOCALIZER:
  // return "L";

  // case ap::fix::NONE:
  // return "NONE";

  // case ap::fix::VOR:
  // return "V";

  // case ap::fix::NDB:
  // return "N";

  // case ap::fix::TERMINAL_NDB:
  // return "TN";

  // case ap::fix::WAYPOINT:
  // return "W";

  // case ap::fix::TERMINAL_WAYPOINT:
  // return "TW";

  // case ap::fix::RUNWAY:
  // return "R";

  if(sc == sc::AIRPORT)
  {
    sc::AirportSubSectionCode subSec =
      static_cast<sc::AirportSubSectionCode>(subSectionCode.at(0).toLatin1());

    switch(subSec)
    {
      case atools::fs::xp::sc::TERMINAL_WAYPOINTS:
        return "TW";

      case atools::fs::xp::sc::RUNWAYS:
        return "R";

      case atools::fs::xp::sc::LOCALIZER_MARKER:
      case atools::fs::xp::sc::LOCALIZER_GLIDE_SLOPE:
        return "L";

      case atools::fs::xp::sc::TERMINAL_NDB:
        return "TN";

      case atools::fs::xp::sc::REFERENCE_POINTS:
      case atools::fs::xp::sc::GATES:
      case atools::fs::xp::sc::SIDS:
      case atools::fs::xp::sc::STARS:
      case atools::fs::xp::sc::APPROACH_PROCEDURES:
      case atools::fs::xp::sc::TAA:
      case atools::fs::xp::sc::MLS:
      case atools::fs::xp::sc::PATH_POINT:
      case atools::fs::xp::sc::FLT_PLANNING_ARR_DEP:
      case atools::fs::xp::sc::MSA:
      case atools::fs::xp::sc::GLS_STATION:
      case atools::fs::xp::sc::AP_COMMUNICATIONS:
        break;

    }
    qWarning() << context.messagePrefix() << "Unexpected airport section" << sectionCode << "sub" << subSectionCode;
  }
  else if(sc == sc::ENROUTE)
  {
    sc::EnrouteSubSectionCode subSec =
      static_cast<sc::EnrouteSubSectionCode>(subSectionCode.at(0).toLatin1());
    switch(subSec)
    {
      case atools::fs::xp::sc::WAYPOINTS:
        return "W";

      case atools::fs::xp::sc::VORDME:
        return "V";

      case atools::fs::xp::sc::AIRWAY_MARKERS:
      case atools::fs::xp::sc::HOLDING_PATTERNS:
      case atools::fs::xp::sc::AIRWAYS_AND_ROUTES:
      case atools::fs::xp::sc::PREFERRED_ROUTES:
      case atools::fs::xp::sc::AIRWAY_RESTRICTIONS:
      case atools::fs::xp::sc::ER_COMMUNICATIONS:
        break;
    }
    qWarning() << context.messagePrefix() << "Unexpected enroute section" << sectionCode << "sub" << subSectionCode;
  }
  else if(sc == sc::NAVAID)
  {
    sc::NavaidSubSectionCode subSec = static_cast<sc::NavaidSubSectionCode>(subSectionCode.at(0).toLatin1());

    switch(subSec)
    {
      case atools::fs::xp::sc::VHF:
        return "V";

      case atools::fs::xp::sc::NDB:
        return "N";
    }
    qWarning() << context.messagePrefix() << "Unexpected navaid section" << sectionCode << "sub" << subSectionCode;
  }
  else
    qWarning() << context.messagePrefix() << "Unexpected section" << sectionCode;

  return QString();
}

void XpCifpWriter::initQueries()
{
  deInitQueries();

  SqlUtil util(&db);

  insertApproachQuery = new SqlQuery(db);
  insertApproachQuery->prepare(util.buildInsertStatement("approach"));

  insertApproachLegQuery = new SqlQuery(db);
  insertApproachLegQuery->prepare(util.buildInsertStatement("approach_leg"));

  insertTransitionQuery = new SqlQuery(db);
  insertTransitionQuery->prepare(util.buildInsertStatement("transition"));

  insertTransitionLegQuery = new SqlQuery(db);
  insertTransitionLegQuery->prepare(util.buildInsertStatement("transition_leg"));
}

void XpCifpWriter::deInitQueries()
{
  delete insertApproachQuery;
  insertApproachQuery = nullptr;

  delete insertApproachLegQuery;
  insertApproachLegQuery = nullptr;

  delete insertTransitionQuery;
  insertTransitionQuery = nullptr;

  delete insertTransitionLegQuery;
  insertTransitionLegQuery = nullptr;
}

} // namespace xp
} // namespace fs
} // namespace atools
