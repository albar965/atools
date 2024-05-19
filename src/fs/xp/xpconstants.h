/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_FS_XP_XPCONSTANTS_H
#define ATOOLS_FS_XP_XPCONSTANTS_H

#include <QString>

namespace atools {
namespace fs {
namespace common {
class MagDecReader;
}

namespace xp {

enum ContextFlag
{
  NO_FLAG = 0x0000,
  IS_ADDON = 0x0001,
  // IS_3D = 0x0002, obsolete with X-Plane 11.33
  INCLUDE_ILS = 0x0004,
  INCLUDE_VOR = 0x0008,
  INCLUDE_NDB = 0x0010,
  INCLUDE_MARKER = 0x0020,
  INCLUDE_AIRPORT = 0x0040,
  INCLUDE_APPROACH = 0x0080,
  INCLUDE_APPROACHLEG = 0x0100,
  READ_LOCALIZERS = 0x0200, // Hand made localizers
  READ_USER = 0x0400, // user.dat
  READ_CIFP = 0x0800, // CIFP file
  READ_AIRSPACE = 0x1000, // OpenAir airspace file
  READ_SHORT_REPORT = 0x2000, // Do not create 10 reports per file
  UPDATE_CYCLE = 0x4000 // Fetch airac cycle from header
};

Q_DECLARE_FLAGS(ContextFlags, ContextFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(atools::fs::xp::ContextFlags)

/* Context passed to each line reading call */
struct XpWriterContext
{
  int curFileId = 0, cifpAirportId = 0, fileVersion = 0, lineNumber = 0;
  QString localPath, fileName, filePath, cifpAirportIdent;
  atools::fs::xp::ContextFlags flags = NO_FLAG;
  atools::fs::common::MagDecReader *magDecReader = nullptr;

  /* Prepare a header for warning messages or exceptions*/
  QString messagePrefix() const;

};

/* Row codes for earth_nav.dat */
enum NavRowCode
{
  NDB = 2, /*  NDB (Non-Directional Beacon) Includes NDB component of Locator Outer Markers (LOM) */
  VOR = 3, /*  VOR (including VOR-DME and VORTACs) Includes VORs, VOR-DMEs, TACANs and VORTACs */
  LOC = 4, /*  Localizer component of an ILS (Instrument Landing System) */
  LOC_ONLY = 5, /*  Localizer component of a localizer-only approach Includes for LDAs and SDFs */
  GS = 6, /*  Glideslope component of an ILS Frequency shown is paired frequency, notthe DME channel */
  OM = 7, /*  Outer markers (OM) for an ILS Includes outer maker component of LOMs */
  MM = 8, /*  Middle markers (MM) for an ILS */
  IM = 9, /*  Inner markers (IM) for an ILS */
  DME = 12, /*  DME, including the DME component of an ILS, VORTAC or VOR-DME Paired frequency display suppressed on X-Plane’s charts */
  DME_ONLY = 13, /*  Stand-alone DME, orthe DME component of an NDB-DME Paired frequency will be displayed on X-Plane’s charts */

  /*  Unused below */
  SBAS_GBAS_FINAL = 14, /*  14 Final approach path alignment point of an SBAS or GBAS approach path */
  GBAS = 15, /*  15 GBAS differential ground station of a GLS */
  SBAS_GBAS_THRESHOLD = 16 /*  16 Landing threshold point or fictitious threshold point of an SBAS/GBAS approach */
};

/* Row codes for apt.dat */
enum AirportRowCode
{
  NO_ROWCODE = 0,
  LAND_AIRPORT_HEADER = 1,
  SEAPLANE_BASE_HEADER = 16,
  HELIPORT_HEADER = 17,
  LAND_RUNWAY = 100,
  WATER_RUNWAY = 101,
  HELIPAD = 102,

  PAVEMENT_HEADER = 110, // Must form a closed loop.(TAXIWAY_OR_RAMP)
  LINEAR_FEATURE_HEADER = 120, // Can form closed loop or simple string (PAINTED_LINE_OR_LIGHT_STRING)
  AIRPORT_BOUNDARY_HEADER = 130, // Must form a closed loop

  // Pavement and polygons
  NODE = 111, // All nodes can also include a “style” (line or lights)
  NODE_AND_CONTROL_POINT = 112, // Bezier control points define smooth curves
  NODE_CLOSE = 113, // Implied join to first node in chain
  NODE_AND_CONTROL_POINT_CLOSE = 114, // Implied join to first node in chain WITH_IMPLICIT_CLOSE_OF_LOOP

  // Linear features
  NODE_TERMINATING_A_STRING = 115, // No “styles” used(NO_CLOSE_LOOP)
  NODE_WITH_BEZIER_CONTROL_POINT_NO_CLOSE = 116, // No “styles” used, TERMINATING_A STRING (NO_CLOSE_LOOP)

  AIRPORT_VIEWPOINT = 14, // One or none for each airport
  AEROPLANE_STARTUP_LOCATION = 15, // *** Convert these to new row code 1300 ***
  AIRPORT_LIGHT_BEACON = 18, // One or none for each airport
  WINDSOCK = 19, // Zero, one or many for each airport
  TAXIWAY_SIGN = 20, // Zero, one or many for each airport (INC.RUNWAY_DISTANCE - REMAINING_SIGNS)
  LIGHTING_OBJECT = 21, // Zero, one or many for each airport (VASI, PAPI, WIG - WAG, ETC.)
  AIRPORT_TRAFFIC_FLOW = 1000, // Zero, one or many for an airport. Used if following rules met (rules of same type use ‘or’ logic, rules of a different type use ‘and’ logic). First flow to pass all rules is used.
  TRAFFIC_FLOW_WIND_RULE = 1001, // Zero, one or many for a flow. Multiple rules use ‘or’ logic.
  TRAFFIC_FLOW_MINIMUM_CEILING_RULE = 1002, // Zero or one rule for each flow
  TRAFFIC_FLOW_MINIMUM_VISIBILITY_RULE = 1003, // Zero or one rule for each flow
  TRAFFIC_FLOW_TIME_RULE = 1004, // Zero, one or many for a flow. Multiple rules use ‘or’ logic.
  RUNWAY_IN_USE = 1100, // First constraint met is used. Sequence matters! ARRIVAL / DEPARTURE_CONSTRAINTS
  VFR_TRAFFIC_PATTERN = 1101, // Zero or one pattern for each traffic flow
  HEADER_INDICATING_THAT_TAXI_ROUTE_NETWORK_DATA_FOLLOWS = 1200,
  TAXI_ROUTE_NETWORK_NODE = 1201, // Sequencing must be 0 based, ascending by ID. Must be part of one or more edges.
  TAXI_ROUTE_NETWORK_EDGE = 1202, // Must connect two nodes. Also takes one of 6 sizes (A-F).
  TAXI_ROUTE_EDGE_ACTIVE_ZONE = 1204, // Can refer to up to 4 runway ends

  AIRPORT_LOCATION = 1300, // Not explicitly connected to taxi route network
  RAMP_START_METADATA = 1301, // Includes width, operations type, equipment type, & airlines.

  METADATA_RECORDS = 1302, // Zero or many for each airport.

  TRUCK_PARKING_LOCATION = 1400, // Not explicitly connected to taxi route network.
  TRUCK_DESTINATION_LOCATION = 1401, // Not explicitly connected to taxi route network.

  // 50 – 56 Legacy 25kHz communication frequencies Zero, one or many for each airport. Ignored if row codes 1050-1056 exist.
  COM_WEATHER = 50, // AWOS, ASOS or ATIS
  COM_UNICOM = 51, // UNICOM
  COM_CLEARANCE = 52, // Clearance delivery
  COM_GROUND = 53, // Ground
  COM_TOWER = 54, // Tower
  COM_APPROACH = 55, // Approach
  COM_DEPARTURE = 56, // Departure

  // 1050 –1056 8.33kHz communication frequencies (11.30+) Zero, one or many for each airport
  COM_NEW_WEATHER = 1050, // AWOS, ASOS or ATIS
  COM_NEW_UNICOM = 1051, // UNICOM
  COM_NEW_CLEARANCE = 1052, // Clearance delivery
  COM_NEW_GROUND = 1053, // Ground
  COM_NEW_TOWER = 1054, // Tower
  COM_NEW_APPROACH = 1055, // Approach
  COM_NEW_DEPARTURE = 1056 // Departure
};

/* X-Plane specific surface codes */
enum Surface
{
  UNKNOWN = 0,
  ASPHALT = 1,
  CONCRETE = 2,
  TURF_OR_GRASS = 3,
  DIRT = 4,
  GRAVEL = 5,
  DRY_LAKEBED = 12, // (eg.At KEDW) Example: KEDW(Edwards AFB)
  WATER = 13, // runways Nothing displayed
  SNOW_OR_ICE = 14, // Poor friction.Runway markings cannot be added.
  TRANSPARENT = 15, // Hard surface, but no texture / markings (use in custom scenery)

  // New X-Plane 12 types (WED 2.5)
  ASPHALT_L = 20,
  ASPHALT_L_PATCHED = 21,
  ASPHALT_L_PLAIN = 22,
  ASPHALT_L_WORN = 23,
  ASPHALT_PATCHED = 24,
  ASPHALT_PLAIN = 25,
  ASPHALT_WORN = 26,
  ASPHALT_D = 27,
  ASPHALT_D_PATCHED = 28,
  ASPHALT_D_PLAIN = 29,
  ASPHALT_D_WORN = 30,
  ASPHALT_D2 = 31,
  ASPHALT_D2_PATCHED = 32,
  ASPHALT_D2_PLAIN = 33,
  ASPHALT_D2_WORN = 34,
  ASPHALT_D3 = 35,
  ASPHALT_D3_PATCHED = 36,
  ASPHALT_D3_PLAIN = 37,
  ASPHALT_D3_WORN = 38,
  CONCRETE_L = 50,
  CONCRETE_L_DIRTY = 51,
  CONCRETE_L_WORN = 52,
  CONCRETE_DIRTY = 53,
  CONCRETE_WORN = 54,
  CONCRETE_D = 55,
  CONCRETE_D_DIRTY = 56,
  CONCRETE_D_WORN = 57
};

QString surfaceToDb(atools::fs::xp::Surface value, const XpWriterContext *context);
bool isSurfaceHard(atools::fs::xp::Surface value);
bool isSurfaceSoft(atools::fs::xp::Surface value);
bool isSurfaceWater(atools::fs::xp::Surface value);

/* Runway markings */
enum Marking
{
  NO_MARKING = 0, // No runway markings Disused runways appear like taxiways
  VISUAL = 1, // Visual markings

  NON_PAP = 2, // Non - precision approach markings
  PAP = 3, // Precision approach markings

  // UK style - uses distinctive touch-down zone markings
  UK_NON_PAP = 4, // Non - precision approach markings UK uses distinctive touch - down zone markings
  UK_PAP = 5, // Precision approach markings UK uses distinctive touch - down zone markings

  // EASA differs from FAA for location and number of distance marks on runway before/after touchdown zone bars
  EASA_NON_PAP = 6, // EASA style non-precision approach markings
  EASA_PAP = 7, // EASA style precision approach markings
};

int markingToDb(atools::fs::xp::Marking value, const XpWriterContext *context);

enum ApproachLight
{
  NO_ALS = 0, // No approach lighting
  ALSF_I = 1, // High intensity Approach Light System with sequenced flashing lights
  ALSF_II = 2, // High intensity Approach Light System with sequenced Flashing lights. Red side bar lights(barettes) the last 1000’, that align with TDZ lighting.
  CALVERT = 3, // British - High intensity
  CALVERT_ILS = 4, // British - High intensity with red side bar lights(barettes) the last 1000ft. Barettes align with TDZ lighting
  SSALR = 5, // High intensity, Simplified Short Approach Light System. With Runway Alignment Indicator Lights(RAIL)
  SSALF = 6, // High intensity, Simplified Short Approach Light System. With sequenced flashing lights
  SALS = 7, // High intensity, Short Approach Light System
  MALSR = 8, // Medium - intensity Approach Light System. With Runway Alignment Indicator Lights(RAIL)
  MALSF = 9, // Medium - intensity Approach Light System with sequenced flashing lights
  MALS = 10, // Medium - intensity Approach Light System
  ODALS = 11, // Omni - directional approach light system. Flashing lights, not strobes, not sequenced
  RAIL = 12 // Runway Alignment Indicator Lights. Sequenced strobes and green threshold lights, with no other approach lights
};

QString alsToDb(atools::fs::xp::ApproachLight value, const XpWriterContext *context);

enum ApproachIndicator
{
  NO_APPR_INDICATOR = 0,
  VASI = 1, // Location is centre point between the two VASI units
  PAPI_4L = 2, // 4L (four - light) on left of runway Left - handed: red indication appears first on right 2 lights
  PAPI_4R = 3, // 4R (four light)on right of runway Right - handed: red indication appears first on left 2 lights
  SPACE_SHUTTLE_PAPI = 4, // 20 degree glidepath Deprecated.Use normal PAPI with an appropriate angle.
  TRI_COLOR_VASI = 5,
  RUNWAY_GUARD = 6, // (“wig - wag”) lights Pulsating double amber lights alongside runway entrances
  APAPI_L = 7, // (two-light) on left of runway - new in XP 12
  APAPI_R = 8, // (two-light) on left of runway - new in XP 12
};

QString approachIndicatorToDb(atools::fs::xp::ApproachIndicator value, const XpWriterContext *context);

} // namespace xp
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_XP_XPCONSTANTS_H
