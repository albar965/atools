/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_SIMCONNECTNAVFACILITIES_H
#define ATOOLS_SIMCONNECTNAVFACILITIES_H

#include <QList>
#include <QDebug>

namespace atools {
namespace fs {
namespace sc {
namespace db {

// ==============================================================================================================
/*
 * Raw and packed data structures filled by SimConnect used by the data definitions.
 * These do not model the parent/child relations but are only flat structures.
 *
 * Definition names in brackets are documented but not used.
 *
 * Used for SimConnect_AddToFacilityDefinition
 */

/* Avoid padding and alignment in structures */
#pragma pack(push, 1)

/* ==============================================================================================================
 * The layout of these structures has to match the layout requested in the method
 * void SimConnectLoaderPrivate::void addNavFacilityDefinition() */

/* WAYPOINT. The waypoint entry point can have the ROUTE child member
 * FACILITY_DATA_WAYPOINT_DEFINITION_ID */
struct WaypointFacility
{
  // LATITUDE FLOAT64
  // LONGITUDE FLOAT64
  // (ALTITUDE FLOAT64)
  // TYPE INT32
  // (MAGVAR FLOAT32)
  // (N_ROUTES INT32)
  // ICAO CHAR[8]
  // REGION CHAR[8]
  // (IS_TERMINAL_WPT INT32)
  double latitude;
  double longitude;
  qint32 type;
  char icao[8];
  char region[8];
};

/* ROUTE. This is a child member of the WAYPOINT entry point */
struct RouteFacility
{
  // NAME CHAR[32]
  // TYPE INT32
  // NEXT_ICAO  CHAR[8]
  // NEXT_REGION  CHAR[8]
  // NEXT_TYPE  INT32
  // (NEXT_LATITUDE  FLOAT64)
  // (NEXT_LONGITUDE FLOAT64)
  // NEXT_ALTITUDE  FLOAT32
  // PREV_ICAO  CHAR[8]
  // PREV_REGION  CHAR[8]
  // PREV_TYPE  INT32
  // (NEXT_LATITUDE  FLOAT64)
  // (NEXT_LONGITUDE FLOAT64)
  // PREV_ALTITUDE  FLOAT32
  char name[32];
  qint32 type;
  char nextIcao[8];
  char nextRegion[8];
  qint32 nextType;
  float nextAltitude;
  char prevIcao[8];
  char prevRegion[8];
  qint32 prevType;
  float prevAltitude;
};

/* NDB
 * FACILITY_DATA_NDB_DEFINITION_ID */
struct NdbFacility
{
  // LATITUDE FLOAT64
  // LONGITUDE  FLOAT64
  // ALTITUDE FLOAT64
  // FREQUENCY  UINT32
  // TYPE INT32
  // RANGE  FLOAT32
  // (MAGVAR FLOAT32)
  // ICAO CHAR[8]
  // REGION CHAR[8]
  // (IS_TERMINAL_NDB  INT32)
  // NAME CHAR[64]
  // (BFO_REQUIRED INT32)
  double latitude;
  double longitude;
  double altitude;
  quint32 frequency;
  qint32 type;
  float range;
  char icao[8];
  char region[8];
  char name[64];
};

/* VOR and ILS
 * FACILITY_DATA_VOR_DEFINITION_ID */
struct VorFacility
{
  // VOR_LATITUDE FLOAT64
  // VOR_LONGITUDE  FLOAT64
  // VOR_ALTITUDE FLOAT64
  // DME_LATITUDE FLOAT64
  // DME_LONGITUDE  FLOAT64
  // DME_ALTITUDE FLOAT64
  // GS_LATITUDE  FLOAT64
  // GS_LONGITUDE FLOAT64
  // GS_ALTITUDE  FLOAT64
  // (TACAN_LATITUDE FLOAT64)
  // (TACAN_LONGITUDE  FLOAT64)
  // (TACAN_ALTITUDE FLOAT64)
  // IS_NAV INT32
  // IS_DME INT32
  // IS_TACAN INT32
  // HAS_GLIDE_SLOPE  INT32
  // DME_AT_NAV INT32
  // DME_AT_GLIDE_SLOPE INT32
  // HAS_BACK_COURSE  INT32
  // FREQUENCY  UINT32
  // TYPE INT32
  // NAV_RANGE  FLOAT32
  // MAGVAR FLOAT32
  // ICAO CHAR[8]
  // REGION CHAR[8]
  // LOCALIZER  FLOAT32
  // LOCALIZER_WIDTH  FLOAT32
  // GLIDE_SLOPE  FLOAT32
  // NAME CHAR[64]
  // (DME_BIAS FLOAT32)
  // LS_CATEGORY  INT32
  // (IS_TRUE_REFERENCED INT32)
  double vorLatitude;
  double vorLongitude;
  double vorAltitude;
  double dmeLatitude;
  double dmeLongitude;
  double dmeAltitude;
  double gsLatitude;
  double gsLongitude;
  double gsAltitude;
  // double tacanLatitude;
  // double tacanLongitude;
  // double tacanAltitude;
  qint32 isNav;
  qint32 isDme;
  qint32 isTacan;
  qint32 hasGlideSlope;
  qint32 dmeAtNav;
  qint32 dmeAtGlideSlope;
  qint32 hasBackCourse;
  quint32 frequency;
  qint32 type;
  float navRange;
  float magvar;
  char icao[8];
  char region[8];
  float localizer;
  float localizerWidth;
  float glideSlope;
  char name[64];
  qint32 lsCategory;

  bool isIls() const
  {
    return localizerWidth > 0.f;
  }

};

#pragma pack(pop)

// ==============================================================================================================
/*
 * Wrapper for the raw data structure.
 * Models the parent/child relations aggregating the facility structures.
 */

class Waypoint
{
public:
  Waypoint()
  {
  }

  Waypoint(const WaypointFacility& waypointParam)
    : waypoint(waypointParam)
  {
  }

  const WaypointFacility& getWaypointFacility() const
  {
    return waypoint;
  }

  const QList<RouteFacility>& getRouteFacilities() const
  {
    return routes;
  }

  int getNumVictorAirway() const;

  int getNumJetAirway() const;

private:
  friend class SimConnectLoaderPrivate;

  WaypointFacility waypoint;
  QList<RouteFacility> routes;
};

/* VOR type which differs slightly from the definition in the BGL files */
enum VorType
{
  VOR_UNKNOWN = 0,
  TERMINAL = 1,
  LOW_ALTITUDE = 2,
  LOW_ALT = 3,
  HIGH_ALTITUDE = 4,
  HIGH_ALT = 5,
  ILS = 6,
  VOT = 7
};

/* ILS category */
enum LsCategory
{
  NONE = 0,
  CAT1 = 1,
  CAT2 = 2,
  CAT3 = 3,
  LOCALIZER = 4,
  IGS = 5,
  LDA_NO_GS = 6,
  LDA_WITH_GS = 7,
  SDF_NO_GS = 8,
  SDF_WITH_GS = 9,
  LAST = SDF_WITH_GS
};

/* Waypoint type to route waypoint */
QString waypointTypeToRouteDb(char type);

/* Type for VOR, TACAN, etc. */
QVariant vorTypeToDb(VorType type, bool isNav, bool isTacan);

/* Check for valid ILS type */
bool lsTypeValid(LsCategory type);

/* ILS type or null variant if not valid */
QVariant lsTypeToDb(LsCategory type);

} // namespace db
} // namespace sc
} // namespace fs
} // namespace atools

#endif // ATOOLS_SIMCONNECTNAVFACILITIES_H
