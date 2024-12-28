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

#ifndef ATOOLS_SIMCONNECTFACILITIES_H
#define ATOOLS_SIMCONNECTFACILITIES_H

#include <QVector>

class QVariant;
namespace atools {
namespace fs {
namespace sc {
namespace airport {

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

/* Airport - Top level =========================================================================== */
struct AirportFacility
{
  // LATITUDE FLOAT64
  // LONGITUDE FLOAT64
  // ALTITUDE FLOAT64
  // MAGVAR FLOAT32
  // (NAME CHAR[32])
  // NAME64 STRING64
  // ICAO STRING8
  // REGION STRING8
  // TOWER_LATITUDE FLOAT64
  // TOWER_LONGITUDE FLOAT64
  // TOWER_ALTITUDE FLOAT64
  // TRANSITION_ALTITUDE FLOAT32
  // TRANSITION_LEVEL FLOAT32
  double latitude;
  double longitude;
  double altitude;
  float magvar;
  char name[64];
  char icao[8] = "\0"; // Identifier for valid state
  char region[8];
  double towerLatitude;
  double towerLongitude;
  double towerAltitude;
  float transitionAltitude;
  float transitionLevel;
};

/* Child member of the AIRPORT entry point */
struct RunwayFacility
{
  // LATITUDE FLOAT64
  // LONGITUDE FLOAT64
  // ALTITUDE FLOAT64
  // HEADING FLOAT32
  // LENGTH FLOAT32
  // WIDTH FLOAT32
  // PATTERN_ALTITUDE FLOAT32
  // (SLOPE FLOAT32)
  // (TRUE_SLOPE FLOAT32)
  // SURFACE INT32
  // PRIMARY_ILS_ICAO CHAR[8]
  // PRIMARY_ILS_REGION CHAR[8]
  // PRIMARY_ILS_TYPE INT32
  // PRIMARY_NUMBER INT32
  // PRIMARY_DESIGNATOR INT32
  // PRIMARY_THRESHOLD STRUCT
  // PRIMARY_BLASTPAD STRUCT
  // PRIMARY_OVERRUN STRUCT
  // PRIMARY_APPROACH_LIGHTS STRUCT
  // PRIMARY_LEFT_VASI STRUCT
  // PRIMARY_RIGHT_VASI STRUCT
  // SECONDARY_ILS_ICAO CHAR[8]
  // SECONDARY_ILS_REGION CHAR[8]
  // SECONDARY_ILS_TYPE INT32
  // SECONDARY_NUMBER INT32
  // SECONDARY_DESIGNATOR INT32
  // SECONDARY_THRESHOLD STRUCT
  // SECONDARY_BLASTPAD STRUCT
  // SECONDARY_OVERRUN STRUCT
  // SECONDARY_APPROACH_LIGHTS STRUCT
  // SECONDARY_LEFT_VASI STRUCT
  // SECONDARY_RIGHT_VASI STRUCT
  double latitude;
  double longitude;
  double altitude;
  float heading;
  float length;
  float width;
  float patternAltitude;
  qint32 surface;
  qint32 primaryNumber;
  qint32 primaryDesignator;
  char primaryIlsIcao[8];
  char primaryIlsRegion[8];
  qint32 primaryIlsType;
  qint32 secondaryNumber;
  qint32 secondaryDesignator;
  char secondaryIlsIcao[8];
  char secondaryIlsRegion[8];
  qint32 secondaryIlsType;
};

/* Child member of the RUNWAY entry point */
struct PavementFacility
{
  // LENGTH FLOAT32
  // (WIDTH FLOAT32)
  // (ENABLE INT32)
  float length;
};

/* Child member of the RUNWAY entry point */
struct ApproachLightFacility
{
  // SYSTEM INT32
  // (STROBE_COUNT INT32)
  // HAS_END_LIGHTS INT32
  // HAS_REIL_LIGHTS INT32
  // HAS_TOUCHDOWN_LIGHTS INT32
  // (ON_GROUND INT32)
  // ENABLE INT32
  // (OFFSET FLOAT32)
  // (SPACING FLOAT32)
  // (SLOPE FLOAT32)
  qint32 system;
  qint32 hasEndLights;
  qint32 hasReilLights;
  qint32 hasTouchdownLights;
  qint32 enable;
};

/* Child member of the RUNWAY entry point */
struct VasiFacility
{
  // TYPE INT32
  // (BIAS_X FLOAT32)
  // (BIAS_Z FLOAT32)
  // (SPACING FLOAT32)
  // ANGLE FLOAT32
  qint32 type;
  float angle;
};

/* Child member of the AIRPORT entry point */
struct StartFacility
{
  // LATITUDE FLOAT64
  // LONGITUDE FLOAT64
  // ALTITUDE FLOAT64
  // HEADING FLOAT32
  // NUMBER INT32
  // DESIGNATOR INT32
  // TYPE INT32
  double latitude;
  double longitude;
  double altitude;
  float heading;
  qint32 number;
  qint32 designator;
  qint32 type;
};

/* Child member of the AIRPORT entry point */
struct FrequencyFacility
{
  // TYPE INT32
  // FREQUENCY INT32
  // NAME CHAR[64]
  qint32 type;
  qint32 frequency;
  char name[64];
};

/* Child member of the AIRPORT entry point */
struct HelipadFacility
{
  // LATITUDE FLOAT64
  // LONGITUDE FLOAT64
  // ALTITUDE FLOAT64
  // HEADING FLOAT32
  // LENGTH FLOAT32
  // WIDTH FLOAT32
  // SURFACE INT32
  // TYPE INT32
  // (TOUCH_DOWN_LENGTH FLOAT32)
  // (FATO_LENGTH FLOAT32)
  // (FATO_WIDTH FLOAT32)
  double latitude;
  double longitude;
  double altitude;
  float heading;
  float length;
  float width;
  qint32 surface;
  qint32 type;
};

/* Child member of the AIRPORT entry point, and is itself an entry point for the
 * APPROACH_TRANSITION, FINAL_APPROACH_LEG, and MISSED_APPROACH_LEG members. */
struct ApproachFacility
{
  // TYPE INT32
  // SUFFIX INT32
  // RUNWAY_NUMBER INT32
  // RUNWAY_DESIGNATOR INT32
  // FAF_ICAO CHAR[8]
  // FAF_REGION CHAR[8]
  // (FAF_HEADING FLOAT32)
  // FAF_ALTITUDE FLOAT32
  // FAF_TYPE INT32
  // MISSED_ALTITUDE FLOAT32
  // (HAS_LNAV INT32)
  // (HAS_LNAVVNAV INT32)
  // (HAS_LP INT32)
  // (HAS_LPV INT32)
  // IS_RNPAR INT32
  // (IS_RNPAR_MISSED INT32)
  qint32 type;
  qint32 suffix;
  qint32 runwayNumber;
  qint32 runwayDesignator;
  char fafIcao[8];
  char fafRegion[8];
  float fafAltitude;
  qint32 fafType;
  float missedAltitude;
  qint32 rnpAr;
};

/* Child member of the APPROACH entry point, and is itself an entry point for the APPROACH_LEG member. */
struct ApproachTransitionFacility
{
  // TYPE INT32
  // IAF_ICAO CHAR[8]
  // IAF_REGION CHAR[8]
  // IAF_TYPE INT32
  // IAF_ALTITUDE FLOAT32
  // DME_ARC_ICAO CHAR[8]
  // DME_ARC_REGION CHAR[8]
  // DMC_ARC_TYPE INT32
  // DME_ARC_RADIAL INT32
  // DME_ARC_DISTANCE FLOAT32
  // NAME CHAR[8]
  qint32 type;
  char iafIcao[8];
  char iafRegion[8];
  qint32 iafType;
  float iafAltitude;
  char dmeArcIcao[8];
  char dmeArcRegion[8];
  qint32 dmeArcType;
  qint32 dmeArcRadial;
  float dmeArcDistance;
  char name[8];
};

/* APPROACH_LEG, FINAL_APPROACH_LEG and MISSED_APPROACH_LEG
 * Child member of the APPROACH, APPROACH_TRANSITION, RUNWAY_TRANSITION, ENROUTE_TRANSITION, DEPARTURE and ARRIVAL entry points */
struct LegFacility
{
  // TYPE INT32
  // FIX_ICAO CHAR[8]
  // FIX_REGION CHAR[8]
  // FIX_TYPE INT32
  // FIX_LATITUDE FLOAT64
  // FIX_LONGITUDE FLOAT64
  // FIX_ALTITUDE FLOAT64
  // FLY_OVER INT32
  // DISTANCE_MINUTE INT32
  // TRUE_DEGREE INT32
  // TURN_DIRECTION INT32
  // ORIGIN_ICAO CHAR[8]
  // ORIGIN_REGION CHAR[8]
  // ORIGIN_TYPE INT32
  // ORIGIN_LATITUDE FLOAT64
  // ORIGIN_LONGITUDE FLOAT64
  // ORIGIN_ALTITUDE FLOAT64
  // THETA FLOAT32
  // RHO FLOAT32
  // COURSE FLOAT32
  // ROUTE_DISTANCE FLOAT32
  // APPROACH_ALT_DESC INT32
  // ALTITUDE1 FLOAT32
  // ALTITUDE2 FLOAT32
  // SPEED_LIMIT FLOAT32
  // VERTICAL_ANGLE FLOAT32
  // ARC_CENTER_FIX_ICAO CHAR[8]
  // ARC_CENTER_FIX_REGION CHAR[8]
  // ARC_CENTER_FIX_TYPE
  // ARC_CENTER_FIX_LATITUDE FLOAT64
  // ARC_CENTER_FIX_LONGITUDE FLOAT64
  // ARC_CENTER_FIX_ALTITUDE FLOAT64
  // RADIUS FLOAT32
  // IS_IAF INT32
  // IS_IF INT32
  // IS_FAF INT32
  // IS_MAP INT32
  // REQUIRED_NAVIGATION_PERFORMANCE FLOAT32
  // APPROACH_SPEED_DESC INT32
  qint32 type;
  char fixIcao[8];
  char fixRegion[8];
  qint32 fixType;
  double fixLatitude;
  double fixLongitude;
  double fixAltitude;
  qint32 flyOver;
  qint32 distanceMinute;
  qint32 trueDegree;
  qint32 turnDirection;
  char originIcao[8];
  char originRegion[8];
  qint32 originType;
  double originLatitude;
  double originLongitude;
  double originAltitude;
  float theta;
  float rho;
  float course;
  float routeDistance;
  qint32 approachAltDesc;
  float altitude1;
  float altitude2;
  float speedLimit;
  float verticalAngle;
  char arcCenterFixIcao[8];
  char arcCenterFixRegion[8];
  qint32 arcCenterFixType;
  double arcCenterFixLatitude;
  double arcCenterFixLongitude;
  double arcCenterFixAltitude;
  float radius;
  qint32 isIaf;
  qint32 isIf;
  qint32 isFaf;
  qint32 isMap;
  float requiredNavigationPerformance;
};

/* Child member of the AIRPORT entry point, and is itself and
 * entry point for RUNWAY_TRANSITION, ENROUTE_TRANSITION, and APPROACH_LEG. */
struct ArrivalFacility
{
  // NAME CHAR[8]
  char name[8];
};

/* Child member of the AIRPORT entry point, and is itself an entry point
 * for RUNWAY_TRANSITION, ENROUTE_TRANSITION, and APPROACH_LEG. */
struct DepartureFacility
{
  // NAME CHAR[8]
  char name[8];
};

struct RunwayTransitionFacility
{
  // RUNWAY_NUMBER  INT32
  // RUNWAY_DESIGNATOR  INT32
  qint32 runwayNumber;
  qint32 runwayDesignator;
};

struct EnrouteTransitionFacility
{
  // NAME Char[8]
  char name[8];
};

/* Child member of the AIRPORT entry point */
struct TaxiParkingFacility
{
  // TYPE INT32
  // TAXI_POINT_TYPE INT32
  // NAME INT32
  // SUFFIX INT32
  // NUMBER UINT32
  // ORIENTATION INT32
  // HEADING FLOAT32
  // RADIUS FLOAT32
  // BIAS_X FLOAT32
  // BIAS_Z FLOAT32
  qint32 type;
  qint32 taxiPointType;
  qint32 name;
  qint32 suffix;
  quint32 number;
  qint32 orientation;
  float heading;
  float radius;
  float biasX;
  float biasY;
};

/* Child member of the AIRPORT entry point */
struct TaxiPointFacility
{
  // TYPE INT32
  // ORIENTATION INT32
  // BIAS_X FLOAT32
  // BIAS_Z FLOAT32
  qint32 type;
  qint32 orientation;
  float biasX;
  float biasY;
};

/* Child member of the AIRPORT entry point */
struct TaxiPathFacility
{
  // TYPE INT32
  // WIDTH FLOAT32
  // (LEFT_HALF_WIDTH FLOAT32)
  // (RIGHT_HALF_WIDTH FLOAT32)
  // (WEIGHT UINT32)
  // RUNWAY_NUMBER INT32
  // RUNWAY_DESIGNATOR INT32
  // (LEFT_EDGE INT32)
  // LEFT_EDGE_LIGHTED INT32
  // (RIGHT_EDGE INT32)
  // RIGHT_EDGE_LIGHTED INT32
  // (CENTER_LINE INT32)
  // CENTER_LINE_LIGHTED INT32
  // START INT32
  // END INT32
  // NAME_INDEX UINT32
  qint32 type;
  float width;
  qint32 runwayNumber;
  qint32 runwayDesignator;
  qint32 leftEdgeLighted;
  qint32 rightEdgeLighted;
  qint32 centerLineLighted;
  qint32 start;
  qint32 end;
  quint32 nameIndex;
};

/* Child member of the AIRPORT entry point */
struct TaxiNameFacility
{
  // NAME CHAR[32]
  char name[32];
};

/* Child member of the AIRPORT entry point */
struct JetwayFacility
{
  // PARKING_GATE INT32
  // PARKING_SUFFIX INT32
  // PARKING_SPOT INT32
  qint32 parkingGate;
  qint32 parkingSuffix;
  qint32 parkingSpot;
};

#pragma pack(pop)

// ==============================================================================================================
/*
 * Wrappers for the raw data structures.
 * These model the parent/child relations aggregating the facility structures.
 */

// =============================================
class Runway
{
public:
  Runway()
  {
  }

  Runway(const RunwayFacility& runwayFacility)
    : runway(runwayFacility)
  {
  }

  const RunwayFacility& getFacility() const
  {
    return runway;
  }

  /* Primary threshold, blastpad,overrun and secondary threshold, blastpad,overrun */
  const QVector<PavementFacility>& getPavementFacilities() const
  {
    return pavements;
  }

  /* Primary and secondary */
  const QVector<ApproachLightFacility>& getApproachLightFacilities() const
  {
    return approachLights;
  }

  /* Primary left, right and secondary left, right */
  const QVector<VasiFacility>& getVasiFacilities() const
  {
    return vasi;
  }

  bool isHard() const;
  bool isWater() const;
  bool isSoft() const;

private:
  friend class SimConnectLoaderPrivate;

  RunwayFacility runway;
  QVector<PavementFacility> pavements;
  QVector<ApproachLightFacility> approachLights;
  QVector<VasiFacility> vasi;
};

// =============================================
class ApproachTransition
{
public:
  ApproachTransition()
  {
  }

  ApproachTransition(const ApproachTransitionFacility& transitionFacility)
    : transition(transitionFacility)
  {
  }

  const ApproachTransitionFacility& getApproachTransitionFacility() const
  {
    return transition;
  }

  const QVector<LegFacility>& getApproachTransitionLegFacilities() const
  {
    return transitionLegs;
  }

private:
  friend class SimConnectLoaderPrivate;

  ApproachTransitionFacility transition;
  QVector<LegFacility> transitionLegs;
};

// =============================================
class Approach
{
public:
  Approach()
  {
  }

  Approach(const ApproachFacility& approachFacility)
    : approach(approachFacility)
  {
  }

  const ApproachFacility& getApproachFacility() const
  {
    return approach;
  }

  const QVector<ApproachTransition>& getApproachTransitions() const
  {
    return approachTransitions;
  }

  const QVector<LegFacility>& getFinalApproachLegFacilities() const
  {
    return finalApproachLegs;
  }

  const QVector<LegFacility>& getMissedApproachLegFacilities() const
  {
    return missedApproachLegs;
  }

private:
  friend class SimConnectLoaderPrivate;

  ApproachFacility approach;
  QVector<ApproachTransition> approachTransitions;
  QVector<LegFacility> finalApproachLegs;
  QVector<LegFacility> missedApproachLegs;
};

// =============================================
class RunwayTransition
{
public:
  RunwayTransition()
  {
  }

  RunwayTransition(const RunwayTransitionFacility& transitionFacility)
    : transition(transitionFacility)
  {
  }

  const RunwayTransitionFacility& getTransitionFacility() const
  {
    return transition;
  }

  const QVector<LegFacility>& getLegFacilities() const
  {
    return legs;
  }

private:
  friend class SimConnectLoaderPrivate;

  RunwayTransitionFacility transition;
  QVector<LegFacility> legs;
};

// =============================================
class EnrouteTransition
{
public:
  EnrouteTransition()
  {
  }

  EnrouteTransition(const EnrouteTransitionFacility& transitionFacility)
    : transition(transitionFacility)
  {
  }

  const EnrouteTransitionFacility& getTransitionFacility() const
  {
    return transition;
  }

  const QVector<LegFacility>& getLegFacilities() const
  {
    return legs;
  }

private:
  friend class SimConnectLoaderPrivate;

  EnrouteTransitionFacility transition;
  QVector<LegFacility> legs;
};

// =============================================
class Arrival
{
public:
  Arrival()
  {
  }

  Arrival(const ArrivalFacility& approachFacility)
    : arrival(approachFacility)
  {
  }

  const ArrivalFacility& getArrivalFacility() const
  {
    return arrival;
  }

  const QVector<LegFacility>& getApproachLegFacilities() const
  {
    return approachLegs;
  }

  const QVector<RunwayTransition>& getRunwayTransitions() const
  {
    return runwayTransitions;
  }

  const QVector<EnrouteTransition>& getEnrouteTransitions() const
  {
    return enrouteTransitions;
  }

private:
  friend class SimConnectLoaderPrivate;

  ArrivalFacility arrival;
  QVector<LegFacility> approachLegs;
  QVector<RunwayTransition> runwayTransitions;
  QVector<EnrouteTransition> enrouteTransitions;
};

// =============================================
class Departure
{
public:
  Departure()
  {
  }

  Departure(const DepartureFacility& departureFacility)
    : departure(departureFacility)
  {
  }

  const DepartureFacility& getDepartureFacility() const
  {
    return departure;
  }

  const QVector<LegFacility>& getApproachLegFacilities() const
  {
    return approachLegs;
  }

  const QVector<RunwayTransition>& getRunwayTransitions() const
  {
    return runwayTransitions;
  }

  const QVector<EnrouteTransition>& getEnrouteTransitions() const
  {
    return enrouteTransitions;
  }

private:
  friend class SimConnectLoaderPrivate;

  DepartureFacility departure;
  QVector<LegFacility> approachLegs;
  QVector<RunwayTransition> runwayTransitions;
  QVector<EnrouteTransition> enrouteTransitions;
};

// =============================================
class Airport
{
public:
  Airport()
  {
  }

  Airport(const AirportFacility& airportFacility)
    : airport(airportFacility)
  {
  }

  /* true if not airport at all. */
  bool isPoiDummy() const
  {
    return runways.isEmpty() && taxiPaths.isEmpty() && helipads.isEmpty() && taxiParkings.isEmpty() && taxiPoints.isEmpty() &&
           starts.isEmpty();
  }

  /* true if invalid or default constructed */
  bool isValid()
  {
    return ::strlen(airport.icao) > 0;
  }

  const AirportFacility& getAirportFacility() const
  {
    return airport;
  }

  const QVector<Runway>& getRunways() const
  {
    return runways;
  }

  const QVector<StartFacility>& getStartFacilities() const
  {
    return starts;
  }

  const QVector<FrequencyFacility>& getFrequencyFacilities() const
  {
    return frequencies;
  }

  const QVector<HelipadFacility>& getHelipadFacilities() const
  {
    return helipads;
  }

  const QVector<Approach>& getApproaches() const
  {
    return approaches;
  }

  const QVector<Arrival>& getArrivals() const
  {
    return arrivals;
  }

  const QVector<Departure>& getDepartures() const
  {
    return departures;
  }

  const QVector<TaxiParkingFacility>& getTaxiParkingFacilities() const
  {
    return taxiParkings;
  }

  const QVector<TaxiPointFacility>& getTaxiPointFacilities() const
  {
    return taxiPoints;
  }

  const QVector<TaxiPathFacility>& getTaxiPathFacilities() const
  {
    return taxiPaths;
  }

  const QVector<TaxiNameFacility>& getTaxiNameFacilities() const
  {
    return taxiNames;
  }

  const QVector<JetwayFacility>& getJetwayFacilities() const
  {
    return jetways;
  }

  /* Get first matching frequency */
  const QVariant getTowerFrequency() const;
  const QVariant getAtisFrequency() const;
  const QVariant getAwosFrequency() const;
  const QVariant getAsosFrequency() const;
  const QVariant getUnicomFrequency() const;

  /* Counts for parking types */
  int getNumParkingGate() const;
  int getNumParkingGaRamp() const;
  int getNumParkingCargo() const;
  int getNumParkingMilCargo() const;
  int getNumParkingMilCombat() const;

  /* Counts for runway features */
  int getNumRunwayHard() const;
  int getNumRunwaySoft() const;
  int getNumRunwayWater() const;
  int getNumRunwayEndVasi() const;
  int getNumRunwayEndAls() const;
  int getNumRunwayEndIls() const;
  int getLongestRunwayIndex() const;

  /* Largest type or null variant */
  const QVariant getLargestParkingGate() const;
  const QVariant getLargestParkingRamp() const;

private:
  friend class SimConnectLoaderPrivate;

  AirportFacility airport;
  QVector<Runway> runways;
  QVector<StartFacility> starts;
  QVector<FrequencyFacility> frequencies;
  QVector<HelipadFacility> helipads;
  QVector<Approach> approaches;
  QVector<Arrival> arrivals;
  QVector<Departure> departures;
  QVector<TaxiParkingFacility> taxiParkings;
  QVector<TaxiPointFacility> taxiPoints;
  QVector<TaxiPathFacility> taxiPaths;
  QVector<TaxiNameFacility> taxiNames;
  QVector<JetwayFacility> jetways;
};

/* MSFS 2024 surface definition */
enum Surface
{
  CONCRETE = 0,
  GRASS = 1,
  WATER_FSX = 2,
  GRASS_BUMPY = 3,
  ASPHALT = 4,
  SHORT_GRASS = 5,
  LONG_GRASS = 6,
  HARD_TURF = 7,
  SNOW = 8,
  ICE = 9,
  URBAN = 10,
  FOREST = 11,
  DIRT = 12,
  CORAL = 13,
  GRAVEL = 14,
  OIL_TREATED = 15,
  STEEL_MATS = 16,
  BITUMINUS = 17,
  BRICK = 18,
  MACADAM = 19,
  PLANKS = 20,
  SAND = 21,
  SHALE = 22,
  TARMAC = 23,
  WRIGHT_FLYER_TRACK = 24,
  OCEAN = 26,
  WATER = 27,
  POND = 28,
  LAKE = 29,
  RIVER = 30,
  WASTE_WATER = 31,
  PAINT = 32,

  UNKNOWN = 254,
  UNDEFINED = 255,

  TAXIPATH = ASPHALT
};

/* Maps to simplified default surface types */
QVariant surfaceToDb(Surface surface);

} // namespace airport
} // namespace sc
} // namespace fs
} // namespace atools

#endif // ATOOLS_SIMCONNECTFACILITIES_H
