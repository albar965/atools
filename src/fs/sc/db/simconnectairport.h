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

#ifndef ATOOLS_SIMCONNECTAIRPORTFACILITIES_H
#define ATOOLS_SIMCONNECTAIRPORTFACILITIES_H

#include <QVarLengthArray>
#include <QVector>

class QVariant;
namespace atools {
namespace fs {
namespace sc {
namespace db {

// ==============================================================================================================
/*
 * Raw and packed data structures filled by SimConnect used by the data definitions to fetch
 * airport and child structures.
 *
 * These structs do not model the parent/child relations but are only flat structures.
 *
 * Definition names in brackets are documented but not used.
 *
 * Used for SimConnect_AddToFacilityDefinition
 */

/* Avoid padding and alignment in structures */
#pragma pack(push, 1)

/* ==============================================================================================================
 * The layout of these structures has to match the layout requested in the methods
 * void SimConnectLoaderPrivate::addAirportNumFacilityDefinition();
 * void SimConnectLoaderPrivate::addAirportBaseFacilityDefinition();
 * void SimConnectLoaderPrivate::addAirportFrequencyFacilityDefinition();
 * void SimConnectLoaderPrivate::addAirportHelipadFacilityDefinition();
 * void SimConnectLoaderPrivate::addAirportRunwayFacilityDefinition();
 * void SimConnectLoaderPrivate::addAirportStartFacilityDefinition();
 * void SimConnectLoaderPrivate::addAirportProcedureFacilityDefinition();
 * void SimConnectLoaderPrivate::addAirportTaxiFacilityDefinition(); */

/* Airport - Top level before start, frequency etc. ============================================================ */
struct AirportFacilityIcao
{
  // ICAO STRING8
  char icao[8];
};

/* Used to get counts for airport features which are requested depending if count is > 0 or not
 * FACILITY_DATA_AIRPORT_NUM_DEFINITION_ID  */
struct AirportFacilityNum
{
  // ICAO STRING8
  // N_RUNWAYS  INT32
  // N_STARTS INT32
  // N_FREQUENCIES  INT32
  // N_HELIPADS INT32
  // N_APPROACHES INT32
  // N_DEPARTURES INT32
  // N_ARRIVALS INT32
  // N_TAXI_POINTS  INT32
  // N_TAXI_PARKINGS  INT32
  // N_TAXI_PATHS INT32
  // N_TAXI_NAMES INT32
  // N_JETWAYS  INT32
  char icao[8] = "\0"; // Identifier for valid state
  int numRunways;
  int numStarts;
  int numFrequencies;
  int numHelipads;
  int numApproaches;
  int numDepartures;
  int numArrivals;
  int numTaxiPoints;
  int numTaxiParkings;
  int numTaxiPaths;
  int numTaxiNames;
  int numJetways;
};

/* FACILITY_DATA_AIRPORT_BASE_DEFINITION_ID */
struct AirportFacility
{
  // ICAO STRING8
  // REGION STRING8
  // LATITUDE FLOAT64
  // LONGITUDE FLOAT64
  // ALTITUDE FLOAT64
  // (MAGVAR FLOAT32)
  // (NAME CHAR[32])
  // NAME64 STRING64
  // ICAO STRING8
  // REGION STRING8
  // TOWER_LATITUDE FLOAT64
  // TOWER_LONGITUDE FLOAT64
  // TOWER_ALTITUDE FLOAT64
  // TRANSITION_ALTITUDE FLOAT32
  // TRANSITION_LEVEL FLOAT32
  char icao[8];
  char region[8];
  double latitude;
  double longitude;
  double altitude;
  char name[64];
  double towerLatitude;
  double towerLongitude;
  double towerAltitude;
  float transitionAltitude;
  float transitionLevel;
};

/* Child member of the AIRPORT entry point
 * FACILITY_DATA_AIRPORT_RW_DEFINITION_ID */
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
  // (ENABLE INT32)
  // (OFFSET FLOAT32)
  // (SPACING FLOAT32)
  // (SLOPE FLOAT32)
  qint32 system;
  qint32 hasEndLights;
  qint32 hasReilLights;
  qint32 hasTouchdownLights;
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

/* Child member of the AIRPORT entry point
 * FACILITY_DATA_AIRPORT_START_DEFINITION_ID */
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

/* Child member of the AIRPORT entry point
 * FACILITY_DATA_AIRPORT_FREQ_DEFINITION_ID */
struct FrequencyFacility
{
  // TYPE INT32
  // FREQUENCY INT32
  // NAME CHAR[64]
  qint32 type;
  qint32 frequency;
  char name[64];
};

/* Child member of the AIRPORT entry point
 * FACILITY_DATA_AIRPORT_HELIPAD_DEFINITION_ID */
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
 * APPROACH_TRANSITION, FINAL_APPROACH_LEG, and MISSED_APPROACH_LEG members.
 * FACILITY_DATA_AIRPORT_PROC_DEFINITION_ID */
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
  // (HAS_LNAV INT32) Returns whether the approach has lateral navigation
  // (HAS_LNAVVNAV INT32) Returns whether the approach has lateral and vertical navigation
  // (HAS_LP INT32) Returns whether the approach has localizer performance
  // (HAS_LPV INT32) Returns whether the approach has localizer performance with vertical guidance (1) or not (0).
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
  // (NAME CHAR[8])
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
};

/* APPROACH_LEG, FINAL_APPROACH_LEG and MISSED_APPROACH_LEG
 * Child member of the APPROACH, APPROACH_TRANSITION, RUNWAY_TRANSITION, ENROUTE_TRANSITION, DEPARTURE and ARRIVAL entry points */
struct LegFacility
{
  // TYPE INT32
  // FIX_ICAO CHAR[8]
  // FIX_REGION CHAR[8]
  // FIX_TYPE INT32
  // (FIX_LATITUDE FLOAT64)
  // (FIX_LONGITUDE FLOAT64)
  // (FIX_ALTITUDE FLOAT64)
  // FLY_OVER INT32
  // DISTANCE_MINUTE INT32
  // TRUE_DEGREE INT32
  // TURN_DIRECTION INT32
  // ORIGIN_ICAO CHAR[8]
  // ORIGIN_REGION CHAR[8]
  // ORIGIN_TYPE INT32
  // (ORIGIN_LATITUDE FLOAT64)
  // (ORIGIN_LONGITUDE FLOAT64)
  // (ORIGIN_ALTITUDE FLOAT64)
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
  // (ARC_CENTER_FIX_LATITUDE FLOAT64)
  // (ARC_CENTER_FIX_LONGITUDE FLOAT64)
  // (ARC_CENTER_FIX_ALTITUDE FLOAT64)
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
  qint32 flyOver;
  qint32 distanceMinute;
  qint32 trueDegree;
  qint32 turnDirection;
  char originIcao[8];
  char originRegion[8];
  qint32 originType;
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
  float radius;
  qint32 isIaf;
  qint32 isIf;
  qint32 isFaf;
  qint32 isMap;
  float requiredNavigationPerformance;

  bool operator!=(const LegFacility& other) const
  {
    return !(*this == other);
  }

  /* Ignores runway legs. Otherwise deep compare. */
  bool operator==(const LegFacility& leg) const;

};

uint qHash(const LegFacility& leg);

/* STAR. Child member of the AIRPORT entry point, and is itself and
 * entry point for RUNWAY_TRANSITION, ENROUTE_TRANSITION, and APPROACH_LEG.
 * FACILITY_DATA_AIRPORT_PROC_DEFINITION_ID */
struct ArrivalFacility
{
  // NAME CHAR[8]
  char name[8];
};

/* SID. Child member of the AIRPORT entry point, and is itself an entry point
 * for RUNWAY_TRANSITION, ENROUTE_TRANSITION, and APPROACH_LEG.
 * FACILITY_DATA_AIRPORT_PROC_DEFINITION_ID */
struct DepartureFacility
{
  // NAME CHAR[8]
  char name[8];
};

/* Always present for an approach, a SID or a STAR */
struct RunwayTransitionFacility
{
  // RUNWAY_NUMBER  INT32
  // RUNWAY_DESIGNATOR  INT32
  qint32 runwayNumber;
  qint32 runwayDesignator;
};

/* Converts into table transition. */
struct EnrouteTransitionFacility
{
  // NAME Char[8]
  char name[8];
};

/* Child member of the AIRPORT entry point
 * FACILITY_DATA_AIRPORT_TAXI_DEFINITION_ID */
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

/* Child member of the AIRPORT entry point
 * FACILITY_DATA_AIRPORT_TAXI_DEFINITION_ID */
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

/* Child member of the AIRPORT entry point
 * FACILITY_DATA_AIRPORT_TAXI_DEFINITION_ID */
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

/* Child member of the AIRPORT entry point
 * FACILITY_DATA_AIRPORT_TAXI_DEFINITION_ID */
struct TaxiNameFacility
{
  // NAME CHAR[32]
  char name[32];
};

/* Child member of the AIRPORT entry point
 * FACILITY_DATA_AIRPORT_TAXI_DEFINITION_ID */
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

  explicit Runway(const RunwayFacility& runwayFacility)
    : runway(runwayFacility)
  {
  }

  const RunwayFacility& getFacility() const
  {
    return runway;
  }

  /* Primary threshold, blastpad,overrun and secondary threshold, blastpad,overrun */
  const QVarLengthArray<PavementFacility, 6>& getPavementFacilities() const
  {
    return pavements;
  }

  /* Primary and secondary */
  const QVarLengthArray<ApproachLightFacility, 2>& getApproachLightFacilities() const
  {
    return approachLights;
  }

  /* Primary left, right and secondary left, right */
  const QVarLengthArray<VasiFacility, 4>& getVasiFacilities() const
  {
    return vasi;
  }

  bool isHard() const;
  bool isWater() const;
  bool isSoft() const;

private:
  friend class SimConnectLoaderPrivate;

  RunwayFacility runway;

  /* Below are always fixed size and are created on the stack.
   * Facilities are added in the order as they appear in the dispatchFunction (as registered defintions). */
  QVarLengthArray<PavementFacility, 6> pavements;
  QVarLengthArray<ApproachLightFacility, 2> approachLights;
  QVarLengthArray<VasiFacility, 4> vasi;
};

// =============================================
class ApproachTransition
{
public:
  ApproachTransition()
  {
  }

  explicit ApproachTransition(const ApproachTransitionFacility& transitionFacility)
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

  explicit Approach(const ApproachFacility& approachFacility)
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

  explicit RunwayTransition(const RunwayTransitionFacility& transitionFacility)
    : transition(transitionFacility)
  {
  }

  const RunwayTransitionFacility& getTransitionFacility() const
  {
    return transition;
  }

  RunwayTransitionFacility& getTransitionFacility()
  {
    return transition;
  }

  const QVector<LegFacility>& getLegFacilities() const
  {
    return legs;
  }

  bool operator!=(const RunwayTransition& other) const
  {
    return !(*this == other);
  }

  /* Ignores runway legs. Otherwise deep compare including legs. */
  bool operator==(const RunwayTransition& other) const;

  const QString& getRunwayGroup() const
  {
    return runwayGroup;
  }

  void setRunwayGroup(const QString& value)
  {
    runwayGroup = value;
  }

private:
  friend class SimConnectLoaderPrivate;

  QString runwayGroup;
  RunwayTransitionFacility transition;
  QVector<LegFacility> legs;
};

/* Ignores runway legs. Otherwise hashes legs too. */
uint qHash(const RunwayTransition& trans);

// =============================================
class EnrouteTransition
{
public:
  EnrouteTransition()
  {
  }

  explicit EnrouteTransition(const EnrouteTransitionFacility& transitionFacility)
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

// STAR =============================================
class Arrival
{
public:
  Arrival()
  {
  }

  explicit Arrival(const ArrivalFacility& approachFacility)
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

  void setRunwayTransitions(const QVector<RunwayTransition>& value)
  {
    runwayTransitions = value;
  }

private:
  friend class SimConnectLoaderPrivate;

  ArrivalFacility arrival;
  QVector<LegFacility> approachLegs;
  QVector<RunwayTransition> runwayTransitions;
  QVector<EnrouteTransition> enrouteTransitions;
};

// SID =============================================
class Departure
{
public:
  Departure()
  {
  }

  explicit Departure(const DepartureFacility& departureFacility)
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

  void setRunwayTransitions(const QVector<RunwayTransition>& value)
  {
    runwayTransitions = value;
  }

private:
  friend class SimConnectLoaderPrivate;

  DepartureFacility departure;
  QVector<LegFacility> approachLegs;
  QVector<RunwayTransition> runwayTransitions;
  QVector<EnrouteTransition> enrouteTransitions;
};

// =============================================
/* Top level for all child structures */
class Airport
{
public:
  Airport()
  {
  }

  explicit Airport(const AirportFacilityNum& airportFacility)
    : airportNum(airportFacility)
  {
  }

  /* true if not airport at all. */
  bool isEmptyByNum() const
  {
    return airportNum.numRunways == 0 && airportNum.numHelipads == 0 && airportNum.numTaxiParkings == 0 && airportNum.numStarts == 0;
  }

  bool isCoordinateNull() const;

  /* true if invalid or default constructed */
  bool isValid() const
  {
    return strlen(airportNum.icao) > 0;
  }

  QString getIcao() const
  {
    return airportNum.icao;
  }

  QString getRegion() const
  {
    return airport.region;
  }

  const AirportFacility& getAirportFacility() const
  {
    return airport;
  }

  const AirportFacilityNum& getAirportFacilityNum() const
  {
    return airportNum;
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

  QVector<Arrival>& getArrivals()
  {
    return arrivals;
  }

  QVector<Departure>& getDepartures()
  {
    return departures;
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

  /* Get first matching frequency or null variant if not available */
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

  /* Largest type or null variant if none */
  const QVariant getLargestParkingGate() const;
  const QVariant getLargestParkingRamp() const;

private:
  friend class SimConnectLoaderPrivate;

  /* Only for counting facilites */
  AirportFacilityNum airportNum;

  /* Airport base information */
  AirportFacility airport;

  /* Airport facilities */
  QVector<Runway> runways;
  QVector<StartFacility> starts;
  QVector<FrequencyFacility> frequencies;
  QVector<HelipadFacility> helipads;

  /* Procedures */
  QVector<Approach> approaches;
  QVector<Arrival> arrivals;
  QVector<Departure> departures;

  /* Taxi structure */
  QVector<TaxiParkingFacility> taxiParkings;
  QVector<TaxiPointFacility> taxiPoints;
  QVector<TaxiPathFacility> taxiPaths;
  QVector<TaxiNameFacility> taxiNames;
  QVector<JetwayFacility> jetways;
};

/* VASI facility indexes in the order as they are sent from SimConnect
 * and registered by void SimConnectLoaderPrivate::addAirportRunwayFacilityDefinition(); */
enum
{
  VASI_PRIMARY_LEFT,
  VASI_PRIMARY_RIGHT,
  VASI_SECONDARY_LEFT,
  VASI_SECONDARY_RIGHT
};

/* ALS facility indexes in the order as they are sent from SimConnect
 * and registered by void SimConnectLoaderPrivate::addAirportRunwayFacilityDefinition(); */
enum
{
  APPROACH_LIGHTS_PRIMARY,
  APPROACH_LIGHTS_SECONDARY
};

/* Runway pavement indexes in the order as they are sent from SimConnect
 * and registered by void SimConnectLoaderPrivate::addAirportRunwayFacilityDefinition(); */
enum RunwayPavement
{
  PRIMARY_THRESHOLD,
  PRIMARY_BLASTPAD,
  PRIMARY_OVERRUN,
  SECONDARY_THRESHOLD,
  SECONDARY_BLASTPAD,
  SECONDARY_OVERRUN
};

/* MSFS 2024 surface definitions differ from FSX, P3D and MSFS 2020 */
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

QDebug operator<<(QDebug out, const LegFacility& obj);
QDebug operator<<(QDebug out, const RunwayTransition& obj);
QDebug operator<<(QDebug out, const EnrouteTransition& obj);
QDebug operator<<(QDebug out, const Arrival& obj);
QDebug operator<<(QDebug out, const Departure& obj);

} // namespace db
} // namespace sc
} // namespace fs
} // namespace atools

#endif // ATOOLS_SIMCONNECTAIRPORTFACILITIES_H
