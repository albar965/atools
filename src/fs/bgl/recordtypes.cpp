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

#include "fs/bgl/recordtypes.h"

#include <QDebug>

namespace atools {
namespace fs {
namespace bgl {
namespace rec {

QString recordTypeStr(rec::RecordType type)
{
  switch(type)
  {
    case rec::AIRPORT:
      return QStringLiteral("AIRPORT");

    case rec::WAYPOINT:
      return QStringLiteral("WAYPOINT");

    case rec::AIRPORTSUMMARY:
      return QStringLiteral("AIRPORTSUMMARY");

    case rec::ILS_VOR:
      return QStringLiteral("ILS_VOR");

    case rec::NDB:
      return QStringLiteral("NDB");

    case rec::SCENERYOBJECT:
      return QStringLiteral("SCENERYOBJECT");

    case rec::MARKER:
      return QStringLiteral("MARKER");

    case rec::BOUNDARY:
      return QStringLiteral("BOUNDARY");

    case rec::BOUNDARY_MSFS2024:
      return QStringLiteral("BOUNDARY_MSFS2024");

    case rec::GEOPOL:
      return QStringLiteral("GEOPOL");

    case rec::NAMELIST:
      return QStringLiteral("NAMELIST");

    case rec::VOR_ILS_ICAO_INDEX:
      return QStringLiteral("VOR_ILS_ICAO_INDEX");

    case rec::NDB_ICAO_INDEX:
      return QStringLiteral("NDB_ICAO_INDEX");

    case rec::WAYPOINT_ICAO_INDEX:
      return QStringLiteral("WAYPOINT_ICAO_INDEX");

    case ILS_VOR_MSFS2024:
      return QStringLiteral("ILS_VOR_MSFS2024");

    case WAYPOINT_MSFS2024:
      return QStringLiteral("WAYPOINT_MSFS2024");

    case NDB_MSFS2024:
      return QStringLiteral("NDB_MSFS2024");
  }
  qWarning().nospace().noquote() << "Invalid record type " << type;
  return QStringLiteral("INVALID");
}

QString airportRecordTypeStr(rec::AirportRecordType type)
{
  switch(type)
  {
    case rec::APRON_FIRST_MSFS_NEW:
      return QStringLiteral("APRON_FIRST_MSFS_NEW");

    case rec::MSFS_APPROACH_NEW:
      return QStringLiteral("MSFS_APPROACH_NEW");

    case rec::MSFS_AIRPORT_PROJECTED_MESH:
      return QStringLiteral("MSFS_AIRPORT_PROJECTED_MESH");

    case rec::MSFS_AIRPORT_GROUND_MERGING_TRANSFER:
      return QStringLiteral("MSFS_AIRPORT_GROUND_MERGING_TRANSFER");

    case rec::DELETE_AIRPORT_NAVIGATION:
      return QStringLiteral("DELETE_AIRPORT_NAVIGATION");

    case rec::MSFS_AIRPORT_PAINTED_LINE:
      return QStringLiteral("MSFS_AIRPORT_PAINTED_LINE");

    case rec::MSFS_AIRPORT_PAINTED_HATCHED_AREA:
      return QStringLiteral("MSFS_AIRPORT_PAINTED_HATCHED_AREA");

    case rec::MSFS_AIRPORT_TAXIWAY_SIGN:
      return QStringLiteral("MSFS_AIRPORT_TAXIWAY_SIGN");

    case rec::MSFS_AIRPORT_TAXIWAY_PARKING_MFGR_NAME:
      return QStringLiteral("MSFS_AIRPORT_TAXIWAY_PARKING_MFGR_NAME");

    case rec::MSFS_AIRPORT_JETWAY:
      return QStringLiteral("MSFS_AIRPORT_JETWAY");

    case rec::MSFS_AIRPORT_LIGHT_SUPPORT:
      return QStringLiteral("MSFS_AIRPORT_LIGHT_SUPPORT");

    case rec::MSFS_SID:
      return QStringLiteral("SID_MSFS");

    case rec::MSFS_STAR:
      return QStringLiteral("STAR_MSFS");

    case rec::MSFS_UNKNOWN_00CD:
      return QStringLiteral("MSFS_UNKNOWN_00CD");

    case rec::NAME:
      return QStringLiteral("NAME");

    case rec::TOWER_OBJ:
      return QStringLiteral("TOWER_OBJ");

    case rec::RUNWAY:
      return QStringLiteral("RUNWAY");

    case rec::RUNWAY_P3D_V4:
      return QStringLiteral("RUNWAY_P3D_V4");

    case rec::RUNWAY_MSFS:
      return QStringLiteral("RUNWAY_MSFS");

    case rec::AIRPORT_WAYPOINT:
      return QStringLiteral("AIRPORT_WAYPOINT");

    case rec::HELIPAD:
      return QStringLiteral("HELIPAD");

    case rec::START:
      return QStringLiteral("START");

    case rec::COM:
      return QStringLiteral("COM");

    case rec::DELETE_AIRPORT:
      return QStringLiteral("DELETE_AIRPORT");

    case rec::APRON_FIRST:
      return QStringLiteral("APRON_FIRST");

    case rec::APRON_FIRST_P3D_V5:
      return QStringLiteral("APRON_FIRST_P3D_V5");

    case rec::APRON_FIRST_MSFS:
      return QStringLiteral("APRON_FIRST_MSFS");

    case rec::APRON_SECOND:
      return QStringLiteral("APRON_SECOND");

    case rec::APRON_SECOND_P3D_V4:
      return QStringLiteral("APRON_SECOND_P3D_V4");

    case rec::APRON_SECOND_P3D_V5:
      return QStringLiteral("APRON_SECOND_P3D_V5");

    case rec::APRON_EDGE_LIGHTS:
      return QStringLiteral("APRON_EDGE_LIGHTS");

    case rec::TAXI_POINT:
      return QStringLiteral("TAXI_POINT");

    case rec::TAXI_POINT_P3DV5:
      return QStringLiteral("TAXI_POINT_P3DV5");

    case rec::TAXI_PARKING:
      return QStringLiteral("TAXI_PARKING");

    case rec::TAXI_PARKING_P3D_V5:
      return QStringLiteral("TAXI_PARKING_P3D_V5");

    case rec::TAXI_PARKING_MSFS:
      return QStringLiteral("TAXI_PARKING_MSFS");

    case rec::TAXI_PARKING_FS9:
      return QStringLiteral("TAXI_PARKING_FS9");

    case rec::TAXI_PATH:
      return QStringLiteral("TAXI_PATH");

    case rec::TAXI_PATH_P3D_V4:
      return QStringLiteral("TAXI_PATH_P3D_V4");

    case rec::TAXI_PATH_P3D_V5:
      return QStringLiteral("TAXI_PATH_P3D_V5");

    case rec::TAXI_NAME:
      return QStringLiteral("TAXI_NAME");

    case rec::JETWAY:
      return QStringLiteral("JETWAY");

    case rec::APPROACH:
      return QStringLiteral("APPROACH");

    case rec::FENCE_BLAST:
      return QStringLiteral("FENCE_BLAST");

    case rec::FENCE_BOUNDARY:
      return QStringLiteral("FENCE_BOUNDARY");

    case rec::AIRPORT_UNKNOWN_003B:
      return QStringLiteral("AIRPORT_UNKNOWN_003B");

    case rec::TAXI_PATH_MSFS:
      return QStringLiteral("TAXI_PATH_MSFS");

    case rec::MSFS_AIRPORT_UNKNOWN_0058:
      return QStringLiteral("MSFS_AIRPORT_UNKNOWN_0058");

    case rec::MSFS_AIRPORT_UNKNOWN_0059:
      return QStringLiteral("MSFS_AIRPORT_UNKNOWN_0059");

    case rec::MSFS_AIRPORT_UNKNOWN_005A:
      return QStringLiteral("MSFS_AIRPORT_UNKNOWN_005A");

    case rec::MSFS_AIRPORT_UNKNOWN_005B:
      return QStringLiteral("MSFS_AIRPORT_UNKNOWN_005B");
  }
  // qWarning().nospace().noquote() << "Invalid airport record type " << type;
  return QStringLiteral("INVALID");
}

bool airportRecordTypeValid(rec::AirportRecordType type)
{
  switch(type)
  {
    case rec::APRON_FIRST_MSFS_NEW:
    case rec::MSFS_APPROACH_NEW:

    case rec::DELETE_AIRPORT_NAVIGATION:
    case rec::MSFS_AIRPORT_PAINTED_LINE:
    case rec::MSFS_AIRPORT_PAINTED_HATCHED_AREA:
    case rec::MSFS_AIRPORT_TAXIWAY_SIGN:
    case rec::MSFS_AIRPORT_TAXIWAY_PARKING_MFGR_NAME:
    case rec::MSFS_AIRPORT_JETWAY:

    // Unknown but common records from MSFS to silence warnings
    case rec::MSFS_AIRPORT_LIGHT_SUPPORT:
    case rec::MSFS_UNKNOWN_00CD:

    // Known record but structure unknown
    case rec::MSFS_SID:
    case rec::MSFS_STAR:

    case rec::AIRPORT_WAYPOINT:
    case rec::APPROACH:
    case rec::APRON_EDGE_LIGHTS:
    case rec::APRON_FIRST:
    case rec::APRON_FIRST_P3D_V5:
    case rec::APRON_FIRST_MSFS:
    case rec::APRON_SECOND:
    case rec::APRON_SECOND_P3D_V4:
    case rec::APRON_SECOND_P3D_V5:
    case rec::COM:
    case rec::DELETE_AIRPORT:
    case rec::FENCE_BLAST:
    case rec::FENCE_BOUNDARY:
    case rec::HELIPAD:
    case rec::JETWAY:
    case rec::NAME:
    case rec::RUNWAY:
    case rec::RUNWAY_P3D_V4:
    case rec::RUNWAY_MSFS:
    case rec::START:
    case rec::TAXI_NAME:
    case rec::TAXI_PARKING:
    case rec::TAXI_PARKING_FS9:
    case rec::TAXI_PARKING_P3D_V5:
    case rec::TAXI_PARKING_MSFS:
    case rec::TAXI_PATH:
    case rec::TAXI_PATH_P3D_V4:
    case rec::TAXI_PATH_P3D_V5:
    case rec::TAXI_PATH_MSFS:
    case rec::TAXI_POINT:
    case rec::TAXI_POINT_P3DV5:
    case rec::TOWER_OBJ:
    case rec::MSFS_AIRPORT_PROJECTED_MESH:
    case rec::MSFS_AIRPORT_GROUND_MERGING_TRANSFER:

    // Unknown records to silence warnings
    case rec::AIRPORT_UNKNOWN_003B:
    case rec::MSFS_AIRPORT_UNKNOWN_0058:
    case rec::MSFS_AIRPORT_UNKNOWN_0059:
    case rec::MSFS_AIRPORT_UNKNOWN_005A:
    case rec::MSFS_AIRPORT_UNKNOWN_005B:
      return true;
  }
  return false;
}

QString runwayRecordTypeStr(rec::RunwayRecordType type)
{
  switch(type)
  {
    // Unknown but common records from MSFS to silence warnings
    case rec::MSFS_RUNWAY_DEFORMATION:
      return QStringLiteral("MSFS_RUNWAY_DEFORMATION");

    case rec::MSFS_RUNWAY_FACILITY_MATERIAL:
      return QStringLiteral("MSFS_RUNWAY_FACILITY_MATERIAL");

    case rec::OFFSET_THRESHOLD_PRIM:
      return QStringLiteral("OFFSET_THRESHOLD_PRIM");

    case rec::OFFSET_THRESHOLD_SEC:
      return QStringLiteral("OFFSET_THRESHOLD_SEC");

    case rec::BLAST_PAD_PRIM:
      return QStringLiteral("BLAST_PAD_PRIM");

    case rec::BLAST_PAD_SEC:
      return QStringLiteral("BLAST_PAD_SEC");

    case rec::OVERRUN_PRIM:
      return QStringLiteral("OVERRUN_PRIM");

    case rec::OVERRUN_PRIM_MSFS:
      return QStringLiteral("OVERRUN_PRIM_MSFS");

    case rec::OVERRUN_SEC:
      return QStringLiteral("OVERRUN_SEC");

    case rec::OVERRUN_SEC_MSFS:
      return QStringLiteral("OVERRUN_SEC_MSFS");

    case rec::VASI_PRIM_LEFT:
      return QStringLiteral("VASI_PRIM_LEFT");

    case rec::VASI_PRIM_RIGHT:
      return QStringLiteral("VASI_PRIM_RIGHT");

    case rec::VASI_SEC_LEFT:
      return QStringLiteral("VASI_SEC_LEFT");

    case rec::VASI_SEC_RIGHT:
      return QStringLiteral("VASI_SEC_RIGHT");

    case rec::APP_LIGHTS_PRIM:
      return QStringLiteral("APP_LIGHTS_PRIM");

    case rec::APP_LIGHTS_PRIM_MSFS:
      return QStringLiteral("APP_LIGHTS_PRIM_MSFS");

    case rec::APP_LIGHTS_SEC:
      return QStringLiteral("APP_LIGHTS_SEC");

    case rec::APP_LIGHTS_SEC_MSFS:
      return QStringLiteral("APP_LIGHTS_SEC_MSFS");
  }
  qWarning().nospace().noquote() << "Invalid runway record type " << type;
  return QStringLiteral("INVALID");
}

QString approachRecordTypeStr(rec::ApprRecordType type)
{
  switch(type)
  {
    case rec::LEGS:
      return QStringLiteral("LEGS");

    case rec::LEGS_MSFS:
      return QStringLiteral("LEGS_MSFS");

    case rec::LEGS_MSFS_116:
      return QStringLiteral("LEGS_MSFS_116");

    case rec::LEGS_MSFS_118:
      return QStringLiteral("LEGS_MSFS_118");

    case rec::MISSED_LEGS:
      return QStringLiteral("MISSED_LEGS");

    case rec::MISSED_LEGS_MSFS:
      return QStringLiteral("MISSED_LEGS_MSFS");

    case rec::MISSED_LEGS_MSFS_116:
      return QStringLiteral("MISSED_LEGS_MSFS_116");

    case rec::MISSED_LEGS_MSFS_118:
      return QStringLiteral("MISSED_LEGS_MSFS_118");

    case rec::TRANSITION:
      return QStringLiteral("TRANSITION");

    // case rec::TRANSITION_LEGS_MSFS:
    case rec::TRANSITION_MSFS:
      // case rec::TRANSITION_LEGS_MSFS: Duplicate in MSFS record definition
      return QStringLiteral("TRANSITION_MSFS/TRANSITION_LEGS_MSFS");

    case rec::TRANSITION_LEGS_MSFS_118:
      return QStringLiteral("TRANSITION_LEGS_MSFS_118");

    case rec::TRANSITION_MSFS_116:
      return QStringLiteral("TRANSITION_MSFS_116");

    case rec::TRANSITION_LEGS:
      return QStringLiteral("TRANSITION_LEGS");

    case rec::TRANSITION_LEGS_MSFS_116:
      return QStringLiteral("TRANSITION_LEGS_MSFS_116");

    case rec::RUNWAY_TRANSITIONS_MSFS:
      return QStringLiteral("RUNWAY_TRANSITIONS_MSFS");

    case rec::ENROUTE_TRANSITIONS_MSFS:
      return QStringLiteral("ENROUTE_TRANSITIONS_MSFS");

    case rec::ENROUTE_TRANSITIONS_MSFS_116:
      return QStringLiteral("ENROUTE_TRANSITIONS_MSFS_116");

    case rec::RUNWAY_TRANSITION_LEGS_MSFS:
      return QStringLiteral("RUNWAY_TRANSITION_LEGS_MSFS");

    case rec::RUNWAY_TRANSITION_LEGS_MSFS_116:
      return QStringLiteral("RUNWAY_TRANSITION_LEGS_MSFS_116");

    case rec::RUNWAY_TRANSITION_LEGS_MSFS_118:
      return QStringLiteral("RUNWAY_TRANSITION_LEGS_MSFS_118");

    case rec::COMMON_ROUTE_LEGS_MSFS:
      return QStringLiteral("COMMON_ROUTE_LEGS_MSFS");

    case rec::COMMON_ROUTE_LEGS_MSFS_116:
      return QStringLiteral("COMMON_ROUTE_LEGS_MSFS_116");

    case rec::COMMON_ROUTE_LEGS_MSFS_118:
      return QStringLiteral("COMMON_ROUTE_LEGS_MSFS_118");

    case rec::ENROUTE_TRANSITION_LEGS_MSFS:
      return QStringLiteral("ENROUTE_TRANSITION_LEGS_MSFS");

    case rec::ENROUTE_TRANSITION_LEGS_MSFS_116:
      return QStringLiteral("ENROUTE_TRANSITION_LEGS_MSFS_116");

    case rec::ENROUTE_TRANSITION_LEGS_MSFS_118:
      return QStringLiteral("ENROUTE_TRANSITION_LEGS_MSFS_118");
  }
  qWarning().nospace().noquote() << "Invalid approach record type " << type;
  return QStringLiteral("INVALID");
}

QString ilsvorRecordTypeStr(rec::IlsVorRecordType type)
{
  switch(type)
  {
    case rec::LOCALIZER:
      return QStringLiteral("LOCALIZER");

    case LOCALIZER_MSFS2024:
      return QStringLiteral("LOCALIZER_MSFS2024");

    case rec::GLIDESLOPE:
      return QStringLiteral("GLIDESLOPE");

    case rec::DME:
      return QStringLiteral("DME");

    case DME_MSFS2024:
      return QStringLiteral("DME_MSFS2024");

    case rec::ILS_VOR_NAME:
      return QStringLiteral("ILS_VOR_NAME");
  }
  qWarning().nospace().noquote() << "Invalid ILS/VOR type " << type;
  return QStringLiteral("INVALID");
}

QString ndbRecordTypeStr(rec::NdbRecordType type)
{
  switch(type)
  {
    case rec::NDB_NAME:
      return QStringLiteral("NDB_NAME");
  }
  qWarning().nospace().noquote() << "Invalid NDB type " << type;
  return QStringLiteral("INVALID");
}

QString sceneryObjRecordTypeStr(rec::SceneryObjRecordType type)
{
  switch(type)
  {
    case rec::SCENERYOBJECT_LIB_OBJECT:
      return QStringLiteral("SCENERYOBJECT_LIB_OBJECT");

    case rec::SCENERYOBJECT_ATTACHED_OBJECT:
      return QStringLiteral("SCENERYOBJECT_ATTACHED_OBJECT");

    case rec::SCENERYOBJECT_EFFECT:
      return QStringLiteral("SCENERYOBJECT_EFFECT");

    case rec::SCENERYOBJECT_GEN_BUILDING:
      return QStringLiteral("SCENERYOBJECT_GEN_BUILDING");

    case rec::SCENERYOBJECT_WINDSOCK:
      return QStringLiteral("SCENERYOBJECT_WINDSOCK");

    case rec::SCENERYOBJECT_EXT_BRIDGE:
      return QStringLiteral("SCENERYOBJECT_EXT_BRIDGE");

    case rec::SCENERYOBJECT_TRIGGER:
      return QStringLiteral("SCENERYOBJECT_TRIGGER");
  }
  qWarning().nospace().noquote() << "Invalid scenery object record type " << type;

  return QStringLiteral("INVALID");
}

QString boundaryRecordTypeStr(rec::BoundaryRecordType type)
{
  switch(type)
  {
    case rec::BOUNDARY_COM:
      return QStringLiteral("BOUNDARY_COM");

    case rec::BOUNDARY_NAME:
      return QStringLiteral("BOUNDARY_NAME");

    case rec::BOUNDARY_LINES:
      return QStringLiteral("BOUNDARY_LINES");
  }
  qWarning().nospace().noquote() << "Invalid boundary record type " << type;
  return QStringLiteral("INVALID");
}

bool approachRecordTypeMsfs(ApprRecordType type)
{
  // Common MSFS records
  return approachRecordTypeMsfs116(type) ||
         approachRecordTypeMsfs118(type) ||
         type == rec::LEGS_MSFS ||
         type == rec::MISSED_LEGS_MSFS ||
         type == rec::TRANSITION_LEGS_MSFS ||
         type == rec::RUNWAY_TRANSITION_LEGS_MSFS ||
         type == rec::COMMON_ROUTE_LEGS_MSFS ||
         type == rec::ENROUTE_TRANSITION_LEGS_MSFS;
}

bool approachRecordTypeMsfs116(ApprRecordType type)
{
  // New MSFS records since 1.16.1 ======
  return type == rec::LEGS_MSFS_116 ||
         type == rec::MISSED_LEGS_MSFS_116 ||
         type == rec::TRANSITION_LEGS_MSFS_116 ||
         type == rec::RUNWAY_TRANSITION_LEGS_MSFS_116 ||
         type == rec::COMMON_ROUTE_LEGS_MSFS_116 ||
         type == rec::ENROUTE_TRANSITION_LEGS_MSFS_116;
}

bool approachRecordTypeMsfs118(ApprRecordType type)
{
  // New MSFS records since 1.18.9 ======
  return type == rec::LEGS_MSFS_118 ||
         type == rec::MISSED_LEGS_MSFS_118 ||
         type == rec::TRANSITION_LEGS_MSFS_118 ||
         type == rec::COMMON_ROUTE_LEGS_MSFS_118 ||
         type == rec::ENROUTE_TRANSITION_LEGS_MSFS_118 ||
         type == rec::RUNWAY_TRANSITION_LEGS_MSFS_118;
}

} // namespace rec
} // namespace bgl
} // namespace fs
} // namespace atools
