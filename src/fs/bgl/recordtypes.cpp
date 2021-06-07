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
      return "AIRPORT";

    case rec::WAYPOINT:
      return "WAYPOINT";

    case rec::AIRPORTSUMMARY:
      return "AIRPORTSUMMARY";

    case rec::ILS_VOR:
      return "ILS_VOR";

    case rec::NDB:
      return "NDB";

    case rec::SCENERYOBJECT:
      return "SCENERYOBJECT";

    case rec::MARKER:
      return "MARKER";

    case rec::BOUNDARY:
      return "BOUNDARY";

    case rec::GEOPOL:
      return "GEOPOL";

    case rec::NAMELIST:
      return "NAMELIST";

    case rec::VOR_ILS_ICAO_INDEX:
      return "VOR_ILS_ICAO_INDEX";

    case rec::NDB_ICAO_INDEX:
      return "NDB_ICAO_INDEX";

    case rec::WAYPOINT_ICAO_INDEX:
      return "WAYPOINT_ICAO_INDEX";
  }
  qWarning().nospace().noquote() << "Invalid record type " << type;
  return "INVALID";
}

QString airportRecordTypeStr(rec::AirportRecordType type)
{
  switch(type)
  {
    case rec::MSFS_AIRPORT_PROJECTED_MESH:
      return "MSFS_AIRPORT_PROJECTED_MESH";

    case rec::MSFS_AIRPORT_GROUND_MERGING_TRANSFER:
      return "MSFS_AIRPORT_GROUND_MERGING_TRANSFER";

    case rec::DELETE_AIRPORT_NAVIGATION:
      return "DELETE_AIRPORT_NAVIGATION";

    case rec::MSFS_AIRPORT_PAINTED_LINE:
      return "MSFS_AIRPORT_PAINTED_LINE";

    case rec::MSFS_AIRPORT_PAINTED_HATCHED_AREA:
      return "MSFS_AIRPORT_PAINTED_HATCHED_AREA";

    case rec::MSFS_AIRPORT_TAXIWAY_SIGN:
      return "MSFS_AIRPORT_TAXIWAY_SIGN";

    case rec::MSFS_AIRPORT_TAXIWAY_PARKING_MFGR_NAME:
      return "MSFS_AIRPORT_TAXIWAY_PARKING_MFGR_NAME";

    case rec::MSFS_AIRPORT_JETWAY:
      return "MSFS_AIRPORT_JETWAY";

    case rec::MSFS_AIRPORT_LIGHT_SUPPORT:
      return "MSFS_AIRPORT_LIGHT_SUPPORT";

    case rec::MSFS_SID:
      return "SID_MSFS";

    case rec::MSFS_STAR:
      return "STAR_MSFS";

    case rec::MSFS_UNKNOWN_00CD:
      return "MSFS_UNKNOWN_00CD";

    case rec::NAME:
      return "NAME";

    case rec::TOWER_OBJ:
      return "TOWER_OBJ";

    case rec::RUNWAY:
      return "RUNWAY";

    case rec::RUNWAY_P3D_V4:
      return "RUNWAY_P3D_V4";

    case rec::RUNWAY_MSFS:
      return "RUNWAY_MSFS";

    case rec::AIRPORT_WAYPOINT:
      return "AIRPORT_WAYPOINT";

    case rec::HELIPAD:
      return "HELIPAD";

    case rec::START:
      return "START";

    case rec::COM:
      return "COM";

    case rec::DELETE_AIRPORT:
      return "DELETE_AIRPORT";

    case rec::APRON_FIRST:
      return "APRON_FIRST";

    case rec::APRON_FIRST_P3D_V5:
      return "APRON_FIRST_P3D_V5";

    case rec::APRON_FIRST_MSFS:
      return "APRON_FIRST_MSFS";

    case rec::APRON_SECOND:
      return "APRON_SECOND";

    case rec::APRON_SECOND_P3D_V4:
      return "APRON_SECOND_P3D_V4";

    case rec::APRON_SECOND_P3D_V5:
      return "APRON_SECOND_P3D_V5";

    case rec::APRON_EDGE_LIGHTS:
      return "APRON_EDGE_LIGHTS";

    case rec::TAXI_POINT:
      return "TAXI_POINT";

    case rec::TAXI_POINT_P3DV5:
      return "TAXI_POINT_P3DV5";

    case rec::TAXI_PARKING:
      return "TAXI_PARKING";

    case rec::TAXI_PARKING_P3D_V5:
      return "TAXI_PARKING_P3D_V5";

    case rec::TAXI_PARKING_MSFS:
      return "TAXI_PARKING_MSFS";

    case rec::TAXI_PARKING_FS9:
      return "TAXI_PARKING_FS9";

    case rec::TAXI_PATH:
      return "TAXI_PATH";

    case rec::TAXI_PATH_P3D_V4:
      return "TAXI_PATH_P3D_V4";

    case rec::TAXI_PATH_P3D_V5:
      return "TAXI_PATH_P3D_V5";

    case rec::TAXI_NAME:
      return "TAXI_NAME";

    case rec::JETWAY:
      return "JETWAY";

    case rec::APPROACH:
      return "APPROACH";

    case rec::FENCE_BLAST:
      return "FENCE_BLAST";

    case rec::FENCE_BOUNDARY:
      return "FENCE_BOUNDARY";

    case rec::AIRPORT_UNKNOWN_003B:
      return "AIRPORT_UNKNOWN_003B";

    case rec::TAXI_PATH_MSFS:
      return "TAXI_PATH_MSFS";
  }
  // qWarning().nospace().noquote() << "Invalid airport record type " << type;
  return "INVALID";
}

bool airportRecordTypeValid(rec::AirportRecordType type)
{
  switch(type)
  {
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
      return "MSFS_RUNWAY_DEFORMATION";

    case rec::MSFS_RUNWAY_FACILITY_MATERIAL:
      return "MSFS_RUNWAY_FACILITY_MATERIAL";

    case rec::OFFSET_THRESHOLD_PRIM:
      return "OFFSET_THRESHOLD_PRIM";

    case rec::OFFSET_THRESHOLD_SEC:
      return "OFFSET_THRESHOLD_SEC";

    case rec::BLAST_PAD_PRIM:
      return "BLAST_PAD_PRIM";

    case rec::BLAST_PAD_SEC:
      return "BLAST_PAD_SEC";

    case rec::OVERRUN_PRIM:
      return "OVERRUN_PRIM";

    case rec::OVERRUN_PRIM_MSFS:
      return "OVERRUN_PRIM_MSFS";

    case rec::OVERRUN_SEC:
      return "OVERRUN_SEC";

    case rec::OVERRUN_SEC_MSFS:
      return "OVERRUN_SEC_MSFS";

    case rec::VASI_PRIM_LEFT:
      return "VASI_PRIM_LEFT";

    case rec::VASI_PRIM_RIGHT:
      return "VASI_PRIM_RIGHT";

    case rec::VASI_SEC_LEFT:
      return "VASI_SEC_LEFT";

    case rec::VASI_SEC_RIGHT:
      return "VASI_SEC_RIGHT";

    case rec::APP_LIGHTS_PRIM:
      return "APP_LIGHTS_PRIM";

    case rec::APP_LIGHTS_PRIM_MSFS:
      return "APP_LIGHTS_PRIM_MSFS";

    case rec::APP_LIGHTS_SEC:
      return "APP_LIGHTS_SEC";

    case rec::APP_LIGHTS_SEC_MSFS:
      return "APP_LIGHTS_SEC_MSFS";
  }
  qWarning().nospace().noquote() << "Invalid runway record type " << type;
  return "INVALID";
}

QString approachRecordTypeStr(rec::ApprRecordType type)
{
  switch(type)
  {
    case rec::LEGS:
      return "LEGS";

    case rec::LEGS_MSFS:
      return "LEGS_MSFS";

    case rec::LEGS_MSFS_NEW:
      return "LEGS_MSFS_NEW";

    case rec::MISSED_LEGS:
      return "MISSED_LEGS";

    case rec::MISSED_LEGS_MSFS:
      return "MISSED_LEGS_MSFS";

    case rec::MISSED_LEGS_MSFS_NEW:
      return "MISSED_LEGS_MSFS_NEW";

    case rec::TRANSITION:
      return "TRANSITION";

    // case rec::TRANSITION_LEGS_MSFS:
    case rec::TRANSITION_MSFS:
      // case rec::TRANSITION_LEGS_MSFS: Duplicate in MSFS record definition
      return "TRANSITION_MSFS/TRANSITION_LEGS_MSFS";

    case rec::TRANSITION_MSFS_NEW:
      return "TRANSITION_MSFS_NEW";

    case rec::TRANSITION_LEGS:
      return "TRANSITION_LEGS";

    case rec::TRANSITION_LEGS_MSFS_NEW:
      return "TRANSITION_LEGS_MSFS_NEW";

    case rec::RUNWAY_TRANSITIONS_MSFS:
      return "RUNWAY_TRANSITIONS_MSFS";

    case rec::ENROUTE_TRANSITIONS_MSFS:
      return "ENROUTE_TRANSITIONS_MSFS";

    case rec::ENROUTE_TRANSITIONS_MSFS_NEW:
      return "ENROUTE_TRANSITIONS_MSFS_NEW";

    case rec::RUNWAY_TRANSITION_LEGS_MSFS:
      return "RUNWAY_TRANSITION_LEGS_MSFS";

    case rec::RUNWAY_TRANSITION_LEGS_MSFS_NEW:
      return "RUNWAY_TRANSITION_LEGS_MSFS_NEW";

    case rec::COMMON_ROUTE_LEGS_MSFS:
      return "COMMON_ROUTE_LEGS_MSFS";

    case rec::COMMON_ROUTE_LEGS_MSFS_NEW:
      return "COMMON_ROUTE_LEGS_MSFS_NEW";

    case rec::ENROUTE_TRANSITION_LEGS_MSFS:
      return "ENROUTE_TRANSITION_LEGS_MSFS";

    case rec::ENROUTE_TRANSITION_LEGS_MSFS_NEW:
      return "ENROUTE_TRANSITION_LEGS_MSFS_NEW";
  }
  qWarning().nospace().noquote() << "Invalid approach record type " << type;
  return "INVALID";
}

QString ilsvorRecordTypeStr(rec::IlsVorRecordType type)
{
  switch(type)
  {
    case rec::LOCALIZER:
      return "LOCALIZER";

    case rec::GLIDESLOPE:
      return "GLIDESLOPE";

    case rec::DME:
      return "DME";

    case rec::ILS_VOR_NAME:
      return "ILS_VOR_NAME";
  }
  qWarning().nospace().noquote() << "Invalid ILS/VOR type " << type;
  return "INVALID";
}

QString ndbRecordTypeStr(rec::NdbRecordType type)
{
  switch(type)
  {
    case rec::NDB_NAME:
      return "NDB_NAME";
  }
  qWarning().nospace().noquote() << "Invalid NDB type " << type;
  return "INVALID";
}

QString sceneryObjRecordTypeStr(rec::SceneryObjRecordType type)
{
  switch(type)
  {
    case rec::SCENERYOBJECT_LIB_OBJECT:
      return "SCENERYOBJECT_LIB_OBJECT";

    case rec::SCENERYOBJECT_ATTACHED_OBJECT:
      return "SCENERYOBJECT_ATTACHED_OBJECT";

    case rec::SCENERYOBJECT_EFFECT:
      return "SCENERYOBJECT_EFFECT";

    case rec::SCENERYOBJECT_GEN_BUILDING:
      return "SCENERYOBJECT_GEN_BUILDING";

    case rec::SCENERYOBJECT_WINDSOCK:
      return "SCENERYOBJECT_WINDSOCK";

    case rec::SCENERYOBJECT_EXT_BRIDGE:
      return "SCENERYOBJECT_EXT_BRIDGE";

    case rec::SCENERYOBJECT_TRIGGER:
      return "SCENERYOBJECT_TRIGGER";
  }
  qWarning().nospace().noquote() << "Invalid scenery object record type " << type;

  return "INVALID";
}

QString boundaryRecordTypeStr(rec::BoundaryRecordType type)
{
  switch(type)
  {
    case rec::BOUNDARY_COM:
      return "BOUNDARY_COM";

    case rec::BOUNDARY_NAME:
      return "BOUNDARY_NAME";

    case rec::BOUNDARY_LINES:
      return "BOUNDARY_LINES";
  }
  qWarning().nospace().noquote() << "Invalid boundary record type " << type;
  return "INVALID";
}

} // namespace rec
} // namespace bgl
} // namespace fs
} // namespace atools
