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

#ifndef ATOOLS_BGL_RECORDTYPES_H
#define ATOOLS_BGL_RECORDTYPES_H

#include <QString>

namespace atools {
namespace fs {
namespace bgl {

namespace rec {

/* Top level record types */
enum RecordType
{
  AIRPORT = 0x003c,
  WAYPOINT = 0x0022,
  AIRPORTSUMMARY = 0x0032,
  ILS_VOR = 0x0013,
  NDB = 0x0017,
  SCENERYOBJECT = 0x000e,
  MARKER = 0x0018,
  BOUNDARY = 0x0020,
  GEOPOL = 0x0023,
  NAMELIST = 0x0027,
  VOR_ILS_ICAO_INDEX = 0x0028,
  NDB_ICAO_INDEX = 0x0029,
  WAYPOINT_ICAO_INDEX = 0x002A
};

QString recordTypeStr(RecordType type);

/* Sub record types for airports */
enum AirportRecordType
{
  NAME = 0x0019,
  TOWER_OBJ = 0x0066,
  RUNWAY = 0x0004,
  RUNWAY_P3D_V4 = 0x003e,
  RUNWAY_MSFS = 0x00ce,
  AIRPORT_WAYPOINT = 0x0022,
  HELIPAD = 0x0026,
  START = 0x0011,
  COM = 0x0012,
  DELETE_AIRPORT = 0x0033,
  DELETE_AIRPORT_NAVIGATION = 0x00db,
  APRON_FIRST = 0x0037,
  APRON_FIRST_P3D_V5 = 0x00af,
  APRON_FIRST_MSFS = 0x00d3,
  APRON_SECOND = 0x0030,
  APRON_SECOND_P3D_V4 = 0x0041,
  APRON_SECOND_P3D_V5 = 0x00b0,
  APRON_EDGE_LIGHTS = 0x0031,
  TAXI_POINT = 0x001a,
  TAXI_POINT_P3DV5 = 0x00ac,
  TAXI_PARKING = 0x003d,
  TAXI_PARKING_P3D_V5 = 0x00ad,
  TAXI_PARKING_MSFS = 0x00e7,
  TAXI_PARKING_FS9 = 0x001b,
  TAXI_PATH = 0x001c,
  TAXI_PATH_P3D_V4 = 0x0040,
  TAXI_PATH_P3D_V5 = 0x00AE,
  TAXI_PATH_MSFS = 0x00d4,
  TAXI_NAME = 0x001d,
  JETWAY = 0x003a,
  APPROACH = 0x0024,
  FENCE_BLAST = 0x0038,
  FENCE_BOUNDARY = 0x0039,

  AIRPORT_UNKNOWN_003B = 0x003b,

  MSFS_SID = 0x0042,
  MSFS_STAR = 0x0048,
  MSFS_AIRPORT_LIGHT_SUPPORT = 0x0057,
  MSFS_UNKNOWN_00CD = 0x00cd,
  MSFS_AIRPORT_PAINTED_LINE = 0x00cf,
  MSFS_AIRPORT_PAINTED_HATCHED_AREA = 0x00d8,
  MSFS_AIRPORT_TAXIWAY_SIGN = 0x00d9,
  MSFS_AIRPORT_TAXIWAY_PARKING_MFGR_NAME = 0x00dd,
  MSFS_AIRPORT_JETWAY = 0x00de,
  MSFS_AIRPORT_PROJECTED_MESH = 0x00e8,
  MSFS_AIRPORT_GROUND_MERGING_TRANSFER = 0x00e9
};

QString airportRecordTypeStr(AirportRecordType type);
bool airportRecordTypeValid(AirportRecordType type);

/* Sub record types for runways */
enum RunwayRecordType
{
  OFFSET_THRESHOLD_PRIM = 0x0005,
  OFFSET_THRESHOLD_SEC = 0x0006,
  BLAST_PAD_PRIM = 0x0007,
  BLAST_PAD_SEC = 0x0008,
  OVERRUN_PRIM = 0x0009,
  OVERRUN_SEC = 0x000A,
  OVERRUN_PRIM_MSFS = 0x0065,
  OVERRUN_SEC_MSFS = 0x0066,
  VASI_PRIM_LEFT = 0x000B,
  VASI_PRIM_RIGHT = 0x000C,
  VASI_SEC_LEFT = 0x000D,
  VASI_SEC_RIGHT = 0x000E,
  APP_LIGHTS_PRIM = 0x000f,
  APP_LIGHTS_PRIM_MSFS = 0x00df,
  APP_LIGHTS_SEC = 0x0010,
  APP_LIGHTS_SEC_MSFS = 0x00e0,

  MSFS_RUNWAY_DEFORMATION = 0x003e,
  MSFS_RUNWAY_FACILITY_MATERIAL = 0x00cb
};

QString runwayRecordTypeStr(RunwayRecordType type);

/* Sub record types for approaches and transitions */
enum ApprRecordType
{
  /* Approach legs */
  LEGS = 0x002d,
  LEGS_MSFS = 0x00e1,
  LEGS_MSFS_NEW = 0x00ec,

  /* Missed approache legs */
  MISSED_LEGS = 0x002e,
  MISSED_LEGS_MSFS = 0x00e2,
  MISSED_LEGS_MSFS_NEW = 0x00ed,

  /* Approach transitions */
  TRANSITION = 0x002c,
  TRANSITION_MSFS = 0x00e3,
  TRANSITION_MSFS_NEW = 0x0049,

  /* Approach transition legs */
  TRANSITION_LEGS = 0x002f,
  TRANSITION_LEGS_MSFS = 0x00e3,
  TRANSITION_LEGS_MSFS_NEW = 0x00ee,

  /* MSFS SID and STAR */
  RUNWAY_TRANSITIONS_MSFS = 0x0046,

  ENROUTE_TRANSITIONS_MSFS = 0x0047,
  ENROUTE_TRANSITIONS_MSFS_NEW = 0x004a,

  RUNWAY_TRANSITION_LEGS_MSFS = 0x00e4,
  RUNWAY_TRANSITION_LEGS_MSFS_NEW = 0x00ef,

  COMMON_ROUTE_LEGS_MSFS = 0x00e5,
  COMMON_ROUTE_LEGS_MSFS_NEW = 0x00f0,

  ENROUTE_TRANSITION_LEGS_MSFS = 0x00e6,
  ENROUTE_TRANSITION_LEGS_MSFS_NEW = 0x00f1
};

QString approachRecordTypeStr(ApprRecordType type);

/* Top level record types for ILS or VOR */
enum IlsVorRecordType
{
  LOCALIZER = 0x0014,
  GLIDESLOPE = 0x0015,
  DME = 0x0016,
  ILS_VOR_NAME = 0x0019
};

QString ilsvorRecordTypeStr(IlsVorRecordType type);

/* Top level record type for NDB stations */
enum NdbRecordType
{
  NDB_NAME = 0x0019
};

QString ndbRecordTypeStr(NdbRecordType type);

/* Scenery object top level records types (not used yet) */
enum SceneryObjRecordType
{
  SCENERYOBJECT_LIB_OBJECT = 0x000b,
  SCENERYOBJECT_ATTACHED_OBJECT = 0x1002,
  SCENERYOBJECT_EFFECT = 0x000d,
  SCENERYOBJECT_GEN_BUILDING = 0x000a,
  SCENERYOBJECT_WINDSOCK = 0x000c,
  SCENERYOBJECT_EXT_BRIDGE = 0x0012,
  SCENERYOBJECT_TRIGGER = 0x0010

};

QString sceneryObjRecordTypeStr(SceneryObjRecordType type);

/* Sub record types for airspace boundaries */
enum BoundaryRecordType
{
  BOUNDARY_COM = 0x0012,
  BOUNDARY_NAME = 0x0019,
  BOUNDARY_LINES = 0x0021
};

QString boundaryRecordTypeStr(BoundaryRecordType type);

} // namespace rec

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_RECORDTYPES_H
