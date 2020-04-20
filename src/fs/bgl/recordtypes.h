/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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
  AIRPORT_WAYPOINT = 0x0022,
  HELIPAD = 0x0026,
  START = 0x0011,
  COM = 0x0012,
  DELETE_AIRPORT = 0x0033,
  APRON_FIRST = 0x0037,
  APRON_FIRST_P3D_V5 = 0x00af,
  APRON_SECOND = 0x0030,
  APRON_SECOND_P3D_V4 = 0x0041,
  APRON_SECOND_P3D_V5 = 0x00b0,
  APRON_EDGE_LIGHTS = 0x0031,
  TAXI_POINT = 0x001a,
  TAXI_POINT_P3DV5 = 0x00ac,
  TAXI_PARKING = 0x003d,
  TAXI_PARKING_P3D_V5 = 0x00ad,
  TAXI_PARKING_FS9 = 0x001b,
  TAXI_PATH = 0x001c,
  TAXI_PATH_P3D_V4 = 0x0040,
  TAXI_PATH_P3D_V5 = 0x00AE,
  TAXI_NAME = 0x001d,
  JETWAY = 0x003a,
  APPROACH = 0x0024,
  FENCE_BLAST = 0x0038,
  FENCE_BOUNDARY = 0x0039,
  UNKNOWN_REC = 0x003b
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
  VASI_PRIM_LEFT = 0x000B,
  VASI_PRIM_RIGHT = 0x000C,
  VASI_SEC_LEFT = 0x000D,
  VASI_SEC_RIGHT = 0x000E,
  APP_LIGHTS_PRIM = 0x000f,
  APP_LIGHTS_SEC = 0x0010
};

QString runwayRecordTypeStr(RunwayRecordType type);

/* Sub record types for approaches */
enum ApprRecordType
{
  LEGS = 0x002d,
  MISSED_LEGS = 0x002e,
  TRANSITION = 0x002c,
  TRANSITION_LEGS = 0x002f
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
