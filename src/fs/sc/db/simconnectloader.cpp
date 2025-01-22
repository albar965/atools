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

#include "fs/sc/db/simconnectloader.h"

#include "atools.h"

#include "fs/sc/db/simconnectairport.h"
#include "fs/sc/db/simconnectid.h"
#include "fs/sc/db/simconnectnav.h"
#include "fs/sc/db/simconnectwriter.h"
#include "fs/sc/simconnectapi.h"
#include "geo/pos.h"
#include "util/csvfilereader.h"
#include "zip/gzip.h"

#include <QString>
#include <QStringBuilder>
#include <QDebug>
#include <QCoreApplication>
#include <QThread>
#include <QRegExp>
#include <QFile>

namespace atools {
namespace fs {
namespace sc {
namespace db {

#if !defined(SIMCONNECT_BUILD_WIN32)

/* Facility definitions for airport and children plus all navaids */
enum FacilityDataDefinitionId : SIMCONNECT_DATA_DEFINITION_ID
{
  FACILITY_DATA_NONE_DEFINITION_ID = 0,

  /* Airport and children =========================================================== */
  FACILITY_DATA_AIRPORT_DEFINITION_ID = 1000,

  /* First facility to detect present features for futher requests */
  FACILITY_DATA_AIRPORT_NUM_DEFINITION_ID = FACILITY_DATA_AIRPORT_DEFINITION_ID + 1,

  /* Airport base information */
  FACILITY_DATA_AIRPORT_BASE_DEFINITION_ID = FACILITY_DATA_AIRPORT_DEFINITION_ID + 2,

  /* Airport ICAO for hash set lookup to class Airport plus frequencies after. Only requested if present. */
  FACILITY_DATA_AIRPORT_FREQ_DEFINITION_ID = FACILITY_DATA_AIRPORT_DEFINITION_ID + 3,

  /* Airport ICAO for hash set lookup to class Airport plus helipades after. Only requested if present. */
  FACILITY_DATA_AIRPORT_HELIPAD_DEFINITION_ID = FACILITY_DATA_AIRPORT_DEFINITION_ID + 4,

  /* Airport ICAO for hash set lookup to class Airport plus runways after. Only requested if present. */
  FACILITY_DATA_AIRPORT_RW_DEFINITION_ID = FACILITY_DATA_AIRPORT_DEFINITION_ID + 5,

  /* Airport ICAO for hash set lookup to class Airport plus start positions after. Only requested if present. */
  FACILITY_DATA_AIRPORT_START_DEFINITION_ID = FACILITY_DATA_AIRPORT_DEFINITION_ID + 6,

  /* Airport ICAO for hash set lookup to class Airport plus procedures and all children after. Only requested if present. */
  FACILITY_DATA_AIRPORT_PROC_DEFINITION_ID = FACILITY_DATA_AIRPORT_DEFINITION_ID + 7,

  /* Airport ICAO for hash set lookup to class Airport plus taxi paths, nodes and parking after. Only requested if present. */
  FACILITY_DATA_AIRPORT_TAXI_DEFINITION_ID = FACILITY_DATA_AIRPORT_DEFINITION_ID + 8,

  /* Navaids and children =========================================================== */
  FACILITY_DATA_NAVAID_DEFINITION_ID = 2000,

  /* Waypoints and route/airway children */
  FACILITY_DATA_WAYPOINT_ROUTE_DEFINITION_ID = FACILITY_DATA_NAVAID_DEFINITION_ID + 1,

  /* Waypoints only*/
  FACILITY_DATA_WAYPOINT_DEFINITION_ID = FACILITY_DATA_NAVAID_DEFINITION_ID + 2,

  /* NDB */
  FACILITY_DATA_NDB_DEFINITION_ID = FACILITY_DATA_NAVAID_DEFINITION_ID + 3,

  /* VOR and ILS */
  FACILITY_DATA_VOR_DEFINITION_ID = FACILITY_DATA_NAVAID_DEFINITION_ID + 4
};

const static SIMCONNECT_DATA_REQUEST_ID FACILITY_LIST_REQUEST_AIRPORT_ID = 100;
const static SIMCONNECT_DATA_REQUEST_ID FACILITY_DATA_AIRPORT_REQUEST_ID = 1000; // 840000 airports
const static SIMCONNECT_DATA_REQUEST_ID FACILITY_DATA_NDB_REQUEST_ID = 100000; // 4000 NDB
const static SIMCONNECT_DATA_REQUEST_ID FACILITY_DATA_VOR_REQUEST_ID = 200000; // 5000 VOR
const static SIMCONNECT_DATA_REQUEST_ID FACILITY_DATA_WAYPOINT_ROUTE_REQUEST_ID = 300000; // 45000 waypoints with airways
const static SIMCONNECT_DATA_REQUEST_ID FACILITY_DATA_WAYPOINT_REQUEST_ID = 400000; // 200000 waypoint

// Do no send progress updates more often than this
const static int UPDATE_RATE_MS = 2000;

// ====================================================================================================================
// SimConnectLoaderPrivate ============================================================================================
// Keeps SimConnect types out of the header file
// ====================================================================================================================
class SimConnectLoaderPrivate
{
public:
  SimConnectLoaderPrivate(const atools::win::ActivationContext *activationContext, atools::sql::SqlDatabase& sqlDb,
                          const QString& libraryName, bool verboseLogging)
    : verbose(verboseLogging)
  {
    qDebug() << Q_FUNC_INFO << "Creating DLL and API for" << libraryName;

    // Create API and bind DLL functions
    api = new SimConnectApi;
    if(!api->bindFunctions(activationContext, libraryName))
      errors.append("Error binding functions.");
    else
      writer = new SimConnectWriter(sqlDb, verbose);
  }

  ~SimConnectLoaderPrivate()
  {
    ATOOLS_DELETE_LOG(writer);
    ATOOLS_DELETE_LOG(api);
  }

  // Load airport idents into airportIds
  bool requestAirportList();

  // Load all airport details for idents in airportIds and write to database in callback after loading
  bool loadAirports();

  // Traverse airway network and write navaids loaded from above
  bool loadNavaids();

  bool loadDisconnectedNavaids();

  // Add all facility definitions for airports and their children
  // The definitions have to match the structs in fs/sc/db/simconnectairport.h
  void addAirportNumFacilityDefinition();
  void addAirportBaseFacilityDefinition();
  void addAirportFrequencyFacilityDefinition();
  void addAirportHelipadFacilityDefinition();
  void addAirportRunwayFacilityDefinition();
  void addAirportStartFacilityDefinition();
  void addAirportProcedureFacilityDefinition();
  void addAirportTaxiFacilityDefinition();

  // Add all facility definitions for navaids and airways/routes
  // The definitions have to match the structs in fs/sc/db/simconnectnav.h
  void addNavFacilityDefinition();

  // Add navaid to be loaded later in loadNavaids()
  void addNavaid(const char *icao, const char *region, char type,
                 float lonX = atools::geo::Pos::INVALID_VALUE, float latY = atools::geo::Pos::INVALID_VALUE)
  {
    addNavaid(FacilityId(icao, region, type, lonX, latY));
  }

  void addNavaidsForLeg(const LegFacility *leg)
  {
    addNavaid(leg->fixIcao, leg->fixRegion, leg->fixType);
    addNavaid(leg->originIcao, leg->originRegion, leg->originType);
    addNavaid(leg->arcCenterFixIcao, leg->arcCenterFixRegion, leg->arcCenterFixType);
  }

  void addNavaid(const FacilityId& id)
  {
    // Skip runways, airports, already loaded ids and ids already in request
    if(id.isValid() && id.getType() != ID_RUNWAY && id.getType() != ID_AIPORT &&
       !navaidIdsRequested.contains(id) && !navaidIdSet.contains(id) && !facilityIdLoaded(id))
    {
      if(verbose)
        qDebug() << Q_FUNC_INFO << "Adding navaid" << id;
      navaidIds.append(id);
      navaidIdSet.insert(id);
    }
  }

  // Open facility definition using builder pattern
  SimConnectLoaderPrivate& defId(SIMCONNECT_DATA_DEFINITION_ID id)
  {
    definitionId = id;
    return *this;
  }

  // Open facility defition
  SimConnectLoaderPrivate& open(const char *name)
  {
    add(QString("OPEN " % QString(name)).toLatin1().constData());
    return *this;
  }

  // Close facility definition
  SimConnectLoaderPrivate& close(const char *name)
  {
    add(QString("CLOSE " % QString(name)).toLatin1().constData());
    return *this;
  }

  // Add facility definition using builder pattern
  SimConnectLoaderPrivate& add(const char *name)
  {
    HRESULT hr = api->AddToFacilityDefinition(definitionId, name);

    if(hr != S_OK)
    {
      QString err = QString("Error adding facility definition \"%1\"").arg(name);
      qWarning() << Q_FUNC_INFO << err;
      errors.append(err);
    }
    return *this;
  }

  // Add procedure legs definition also opening and closing name
  SimConnectLoaderPrivate& addLegs(const char *name);

  // Add arrival or departure transition definition
  SimConnectLoaderPrivate& addArrivalDepartureTrans();

  // Call writer and write all airports from the airportFacilities to the database. airportFacilities is not consumed
  bool writeAirportsToDatabase();

  // Write all waypointFacilities (and routes), vorFacilities (and ILS) and ndbFacilities to the database
  bool writeNavaidsToDatabase();

  // Request aiports for the given definitionId
  bool requestAirports(FacilityDataDefinitionId definitionId);

  // Request all navaids from navaidIds and consume navaids from the list
  // Traverses the route network and keeps adding waypoints until done
  bool requestNavaids(bool fetchRoutes);

  // Read navaids not connectedd to the airway network or procedures
  void disconnectedNavaids(const QString& typeFilter);

  // Free all memory
  void clear();

  // Wrappers for SC_RequestFacilityData and SC_RequestFacilityData_EX1
  bool requestFacilityData(SIMCONNECT_DATA_DEFINITION_ID defineId, SIMCONNECT_DATA_REQUEST_ID requestId, const FacilityId& id);
  bool requestFacilityData(SIMCONNECT_DATA_DEFINITION_ID defineId, SIMCONNECT_DATA_REQUEST_ID requestId, const IcaoId& id);

  // Call progress but do not increment progress count
  bool callProgressUpdate()
  {
    return callProgress(QString(), false /* incProgress */);
  }

  bool callProgress(const QString& message, bool incProgress = true);

  // SimConnect dispatch function. pContext is this
  static void dispatchFunction(SIMCONNECT_RECV *pData, DWORD cbData, void *pContext);

  // SimConnect dispatch method
  void dispatchProcedure(SIMCONNECT_RECV *pData);

  // Used by the loading of disconnected ids to compare navaids based on coordinates/vicinity without the unreliable region code
  void addFacilityIdLoaded(const FacilityId& id)
  {
    if(!facilityIdLoaded(id))
      facilityIdentPosHash.insert(id.noRegion(), id);
  }

  bool facilityIdLoaded(const FacilityId& id);

  // ========================================================================

  // Current definition id when defining facility structures
  SIMCONNECT_DATA_DEFINITION_ID definitionId = 0;

  // Current facility definition used in requestAirports() and requestNavaids() and the dispatchProcedure
  FacilityDataDefinitionId currentFacilityDefinition = FACILITY_DATA_NONE_DEFINITION_ID;

  // Current parent airport used in dispatchProcedure
  Airport *currentAirport = nullptr;

  QStringList errors;

  // Stores already requested navaids to avoid double fetch calls
  FacilityIdSet navaidIdsRequested;

  // Navaids filled by dispatchProcedure from airport procedure references or route points.
  // Navaids are added while traversing airways/routes and are consumed when loading requesting
  FacilityIdList navaidIds;

  // Avoid duplicate insert into navaidIds list. This set is filled but not consumed while loading.
  FacilityIdSet navaidIdSet;

  // Used to weed out facilities having an invalid region (ignored) but same ident and type at the same position
  QMultiHash<FacilityId, FacilityId> facilityIdentPosHash;

  // All aiports to fetch - filled by loadAirports()
  IcaoIdVector airportIds;

  // Used to select airport ident to load if not empty
  QList<QRegExp> airportIcaoFiltersInc;

  // SimConnect DLL bindings
  atools::fs::sc::SimConnectApi *api;

  // Airport facility structures filled when requesting airport facility counts (FACILITY_DATA_AIRPORT_NUM_DEFINITION_ID)
  QHash<IcaoId, Airport> airportFacilities;

  // Waypoint and airway/route facilities
  QMap<unsigned long, atools::fs::sc::db::Waypoint> waypointFacilities; // Key is pFacilityData->UserRequestId
  QList<NdbFacility> ndbFacilities;
  QList<VorFacility> vorFacilities;

  // Maps sendId to FacilityId to allow more details when printing error messages in SimConnect exceptions
  QHash<unsigned long, FacilityId> requests;

  // Parent types needed in dispatchProcedure to detect procedure leg types (approach, missed, SID, STAR, etc.)
  SIMCONNECT_FACILITY_DATA_TYPE legsParentType = SIMCONNECT_FACILITY_DATA_AIRPORT;
  SIMCONNECT_FACILITY_DATA_TYPE legsParentType2 = SIMCONNECT_FACILITY_DATA_AIRPORT;

  // Database writer - called after fetching all types
  atools::fs::sc::db::SimConnectWriter *writer = nullptr;

  SimConnectLoaderProgressCallback progressCallback = nullptr;
  int progressCounter = 0; // Counter for dot animation
  QString lastMessage; // Repeat last message when calling  callProgressUpdate()

  // Used to send progress reports not too often
  quint32 progressTimerElapsed = 0L;
  QElapsedTimer timer;

  bool verbose = false,
       facilityListFetched = false, // used in requestAirportList() to detect end of call
       aborted = false;

  // Currently loaded but not written yet features
  int airportsLoaded = 0, waypointsLoaded = 0, vorLoaded = 0, ilsLoaded = 0, ndbLoaded = 0;

  int facilitiesFetchedBatch = 0, // Counter to detect end of batch
      batchSize = 2000, fileId = 0, numException = 0;

  int airportFetchDelay = 50, navaidFetchDelay = 50;
  const int MAX_ERRORS = 1000;
};

// ====================================================================================================================
SimConnectLoaderPrivate& SimConnectLoaderPrivate::addArrivalDepartureTrans()
{
  addLegs("APPROACH_LEG").
  open("RUNWAY_TRANSITION").add("RUNWAY_NUMBER").add("RUNWAY_DESIGNATOR").addLegs("APPROACH_LEG").close("RUNWAY_TRANSITION").
  open("ENROUTE_TRANSITION").add("NAME").addLegs("APPROACH_LEG").close("ENROUTE_TRANSITION");
  return *this;
}

SimConnectLoaderPrivate& SimConnectLoaderPrivate::addLegs(const char *name)
{
  open(name).
  add("TYPE").
  add("FIX_ICAO").add("FIX_REGION").add("FIX_TYPE").
  add("FLY_OVER").add("DISTANCE_MINUTE").add("TRUE_DEGREE").add("TURN_DIRECTION").
  add("ORIGIN_ICAO").add("ORIGIN_REGION").add("ORIGIN_TYPE").
  add("THETA").add("RHO").add("COURSE").add("ROUTE_DISTANCE").add("APPROACH_ALT_DESC").add("ALTITUDE1").add("ALTITUDE2").
  add("SPEED_LIMIT").add("VERTICAL_ANGLE").
  add("ARC_CENTER_FIX_ICAO").add("ARC_CENTER_FIX_REGION").add("ARC_CENTER_FIX_TYPE").
  add("RADIUS").add("IS_IAF").add("IS_IF").add("IS_FAF").add("IS_MAP").add("REQUIRED_NAVIGATION_PERFORMANCE").
  close(name);
  return *this;
}

void SimConnectLoaderPrivate::addNavFacilityDefinition()
{
  // Waypoint and route  =======================================================
  defId(FACILITY_DATA_WAYPOINT_ROUTE_DEFINITION_ID);
  open("WAYPOINT").
  add("LATITUDE").add("LONGITUDE").add("TYPE").add("ICAO").add("REGION").
  open("ROUTE").
  add("NAME").add("TYPE").add("NEXT_ICAO").add("NEXT_REGION").add("NEXT_TYPE").add("NEXT_ALTITUDE").
  add("PREV_ICAO").add("PREV_REGION").add("PREV_TYPE").add("PREV_ALTITUDE").
  close("ROUTE");
  close("WAYPOINT");

  // Waypoint only  =======================================================
  defId(FACILITY_DATA_WAYPOINT_DEFINITION_ID);
  open("WAYPOINT").
  add("LATITUDE").add("LONGITUDE").add("TYPE").add("ICAO").add("REGION").
  close("WAYPOINT");

  // NDB  =======================================================
  defId(FACILITY_DATA_NDB_DEFINITION_ID);
  open("NDB").
  add("LATITUDE").add("LONGITUDE").add("ALTITUDE").
  add("FREQUENCY").add("TYPE").add("RANGE").add("ICAO").add("REGION").add("NAME").
  close("NDB");

  // VOR and ILS  =======================================================
  defId(FACILITY_DATA_VOR_DEFINITION_ID);
  open("VOR").
  add("VOR_LATITUDE").add("VOR_LONGITUDE").add("VOR_ALTITUDE").
  add("DME_LATITUDE").add("DME_LONGITUDE").add("DME_ALTITUDE").
  add("GS_LATITUDE").add("GS_LONGITUDE").add("GS_ALTITUDE").
  add("IS_NAV").add("IS_DME").add("IS_TACAN").add("HAS_GLIDE_SLOPE").add("DME_AT_NAV").add("DME_AT_GLIDE_SLOPE").add("HAS_BACK_COURSE").
  add("FREQUENCY").add("TYPE").add("NAV_RANGE").add("MAGVAR").add("ICAO").add("REGION").
  add("LOCALIZER").add("LOCALIZER_WIDTH").add("GLIDE_SLOPE").add("NAME").
  add("LS_CATEGORY").
  close("VOR");
}

void SimConnectLoaderPrivate::addAirportNumFacilityDefinition()
{
  // Loaded first to detect features to request
  defId(FACILITY_DATA_AIRPORT_NUM_DEFINITION_ID).
  open("AIRPORT").
  add("ICAO").
  add("N_RUNWAYS").add("N_STARTS").add("N_FREQUENCIES").add("N_HELIPADS").add("N_APPROACHES").add("N_DEPARTURES").
  add("N_ARRIVALS").add("N_TAXI_POINTS").add("N_TAXI_PARKINGS").add("N_TAXI_PATHS").add("N_TAXI_NAMES").add("N_JETWAYS").
  close("AIRPORT");
}

void SimConnectLoaderPrivate::addAirportBaseFacilityDefinition()
{
  // Loaded second
  defId(FACILITY_DATA_AIRPORT_BASE_DEFINITION_ID).
  open("AIRPORT").
  add("ICAO").add("REGION").add("LATITUDE").add("LONGITUDE").add("ALTITUDE").add("NAME64").
  add("TOWER_LATITUDE").add("TOWER_LONGITUDE").add("TOWER_ALTITUDE").
  add("TRANSITION_ALTITUDE").add("TRANSITION_LEVEL").
  close("AIRPORT");
}

void SimConnectLoaderPrivate::addAirportFrequencyFacilityDefinition()
{
  // Airport COM prefixed with the ICAO (AirportFacilityIcao) to find the airport facility in the hash airportFacilities
  defId(FACILITY_DATA_AIRPORT_FREQ_DEFINITION_ID).
  open("AIRPORT").
  add("ICAO").
  open("FREQUENCY").
  add("TYPE").add("FREQUENCY").add("NAME").
  close("FREQUENCY");
  close("AIRPORT");
}

void SimConnectLoaderPrivate::addAirportHelipadFacilityDefinition()
{
  // Airport helipads prefixed with the ICAO to find the airport facility in the hash airportFacilities
  defId(FACILITY_DATA_AIRPORT_HELIPAD_DEFINITION_ID).
  open("AIRPORT").
  add("ICAO").
  open("HELIPAD").
  add("LATITUDE").add("LONGITUDE").add("ALTITUDE").add("HEADING").add("LENGTH").add("WIDTH").add("SURFACE").add("TYPE").
  close("HELIPAD");
  close("AIRPORT");
}

void SimConnectLoaderPrivate::addAirportRunwayFacilityDefinition()
{
  // Airport runways prefixed with the ICAO to find the airport facility in the hash airportFacilities
  defId(FACILITY_DATA_AIRPORT_RW_DEFINITION_ID).
  open("AIRPORT").
  add("ICAO").
  open("RUNWAY").
  add("LATITUDE").add("LONGITUDE").add("ALTITUDE").
  add("HEADING").add("LENGTH").add("WIDTH").
  add("PATTERN_ALTITUDE").
  add("SURFACE");

  // Primary =======================
  add("PRIMARY_NUMBER").add("PRIMARY_DESIGNATOR").
  add("PRIMARY_ILS_ICAO").add("PRIMARY_ILS_REGION").add("PRIMARY_ILS_TYPE").
  open("PRIMARY_THRESHOLD").add("LENGTH").close("PRIMARY_THRESHOLD").
  open("PRIMARY_BLASTPAD").add("LENGTH").close("PRIMARY_BLASTPAD").
  open("PRIMARY_OVERRUN").add("LENGTH").close("PRIMARY_OVERRUN").
  open("PRIMARY_APPROACH_LIGHTS").add("SYSTEM").add("HAS_END_LIGHTS").add("HAS_REIL_LIGHTS").
  add("HAS_TOUCHDOWN_LIGHTS").close("PRIMARY_APPROACH_LIGHTS").
  open("PRIMARY_LEFT_VASI").add("TYPE").add("ANGLE").close("PRIMARY_LEFT_VASI").
  open("PRIMARY_RIGHT_VASI").add("TYPE").add("ANGLE").close("PRIMARY_RIGHT_VASI");

  // Secondary =======================
  add("SECONDARY_NUMBER").add("SECONDARY_DESIGNATOR").
  add("SECONDARY_ILS_ICAO").add("SECONDARY_ILS_REGION").add("SECONDARY_ILS_TYPE").
  open("SECONDARY_THRESHOLD").add("LENGTH").close("SECONDARY_THRESHOLD").
  open("SECONDARY_BLASTPAD").add("LENGTH").close("SECONDARY_BLASTPAD").
  open("SECONDARY_OVERRUN").add("LENGTH").close("SECONDARY_OVERRUN").
  open("SECONDARY_APPROACH_LIGHTS").add("SYSTEM").add("HAS_END_LIGHTS").add("HAS_REIL_LIGHTS").
  add("HAS_TOUCHDOWN_LIGHTS").close("SECONDARY_APPROACH_LIGHTS").
  open("SECONDARY_LEFT_VASI").add("TYPE").add("ANGLE").close("SECONDARY_LEFT_VASI").
  open("SECONDARY_RIGHT_VASI").add("TYPE").add("ANGLE").close("SECONDARY_RIGHT_VASI");

  close("RUNWAY");
  close("AIRPORT");
}

void SimConnectLoaderPrivate::addAirportStartFacilityDefinition()
{
  // Airport start positions prefixed with the ICAO to find the airport facility in the hash airportFacilities
  // Requires runways to be loaded before
  defId(FACILITY_DATA_AIRPORT_START_DEFINITION_ID).
  open("AIRPORT").
  add("ICAO").
  open("START").
  add("LATITUDE").add("LONGITUDE").add("ALTITUDE").add("HEADING").add("NUMBER").add("DESIGNATOR").add("TYPE").
  close("START").
  close("AIRPORT");
}

void SimConnectLoaderPrivate::addAirportProcedureFacilityDefinition()
{
  // Airport procedures and children prefixed with the ICAO to find the airport facility in the hash airportFacilities
  // Approach ================================
  defId(FACILITY_DATA_AIRPORT_PROC_DEFINITION_ID).
  open("AIRPORT").
  add("ICAO");

  open("APPROACH").
  add("TYPE").add("SUFFIX").add("RUNWAY_NUMBER").add("RUNWAY_DESIGNATOR").add("FAF_ICAO").add("FAF_REGION").
  add("FAF_ALTITUDE").add("FAF_TYPE").add("MISSED_ALTITUDE").add("IS_RNPAR");

  // Approach Legs =============
  addLegs("FINAL_APPROACH_LEG");

  // Missed Approach Legs =============
  addLegs("MISSED_APPROACH_LEG");

  // Approach Transition ===========
  open("APPROACH_TRANSITION").
  add("TYPE").add("IAF_ICAO").add("IAF_REGION").add("IAF_TYPE").add("IAF_ALTITUDE").add("DME_ARC_ICAO").
  add("DME_ARC_REGION").add("DME_ARC_TYPE").add("DME_ARC_RADIAL").add("DME_ARC_DISTANCE").
  addLegs("APPROACH_LEG").
  close("APPROACH_TRANSITION");

  close("APPROACH");

  // SID / Departure ================================
  open("DEPARTURE").
  add("NAME").addArrivalDepartureTrans().
  close("DEPARTURE");

  // STAR / Arrival ================================
  open("ARRIVAL").
  add("NAME").addArrivalDepartureTrans().
  close("ARRIVAL");

  close("AIRPORT");
}

void SimConnectLoaderPrivate::addAirportTaxiFacilityDefinition()
{
  // Airport taxi structure prefixed with the ICAO to find the airport facility in the hash airportFacilities
  defId(FACILITY_DATA_AIRPORT_TAXI_DEFINITION_ID).
  open("AIRPORT").
  add("ICAO");

  // Taxi Parking ================================
  open("TAXI_PARKING").
  add("TYPE").add("TAXI_POINT_TYPE").add("NAME").add("SUFFIX").add("NUMBER").add("ORIENTATION").add("HEADING").
  add("RADIUS").add("BIAS_X").add("BIAS_Z").
  close("TAXI_PARKING");

  // Taxi Point ================================
  open("TAXI_POINT").
  add("TYPE").add("ORIENTATION").add("BIAS_X").add("BIAS_Z").
  close("TAXI_POINT");

  // Taxi Path ================================
  open("TAXI_PATH").
  add("TYPE").add("WIDTH").add("RUNWAY_NUMBER").
  add("RUNWAY_DESIGNATOR").add("LEFT_EDGE_LIGHTED").add("RIGHT_EDGE_LIGHTED").add("CENTER_LINE_LIGHTED").
  add("START").add("END").add("NAME_INDEX").
  close("TAXI_PATH");

  // Taxi Name ================================
  open("TAXI_NAME").
  add("NAME").
  close("TAXI_NAME");

  // Jetways =======================================================
  open("JETWAY").
  add("PARKING_GATE").add("PARKING_SUFFIX").add("PARKING_SPOT").
  close("JETWAY");

  close("AIRPORT");
}

bool SimConnectLoaderPrivate::loadAirports()
{
  aborted = callProgress(SimConnectLoader::tr("Loading airport count"));

  // Get list of all airports into airportIds
  if((aborted = requestAirportList()))
    return true;

  // Initially fill airportFacilities with airports to count airport facilities
  aborted = callProgress(SimConnectLoader::tr("Loading airport facility numbers"));
  if((aborted = requestAirports(FACILITY_DATA_AIRPORT_NUM_DEFINITION_ID)))
    return true;

  // Remove all empty/dummy/POI airports and references to them
  IcaoIdSet ids;
  for(auto it = airportFacilities.constBegin(); it != airportFacilities.constEnd(); ++it)
  {
    if(it->isEmptyByNum())
      ids.insert(it.key());
  }

  for(const IcaoId& id : ids)
    airportFacilities.remove(id);

  airportIds.clear();
  for(const Airport& id : airportFacilities)
    airportIds.append(IcaoId(id.getAirportFacilityNum().icao));

  // Fill airportFacilities with base information
  aborted = callProgress(SimConnectLoader::tr("Loading airport base information"));
  if((aborted = requestAirports(FACILITY_DATA_AIRPORT_BASE_DEFINITION_ID)))
    return true;

  // Remove all airports having an invalid coordinate and references to them
  ids.clear();
  for(auto it = airportFacilities.constBegin(); it != airportFacilities.constEnd(); ++it)
  {
    if(it->isCoordinateNull())
      ids.insert(it.key());
  }

  for(const IcaoId& id : ids)
    airportFacilities.remove(id);

  airportIds.clear();
  for(const Airport& id : airportFacilities)
    airportIds.append(IcaoId(id.getAirportFacilityNum().icao));

  // Fetch COM and fill into the respective airport facility structure in the hash airportFacilities
  aborted = callProgress(SimConnectLoader::tr("Loading airport COM"));
  if((aborted = requestAirports(FACILITY_DATA_AIRPORT_FREQ_DEFINITION_ID)))
    return true;

  // Fetch helipads and fill into the respective airport facility structure in the hash airportFacilities
  aborted = callProgress(SimConnectLoader::tr("Loading airport helipads"));
  if((aborted = requestAirports(FACILITY_DATA_AIRPORT_HELIPAD_DEFINITION_ID)))
    return true;

  // Fetch runways and fill into the respective airport facility structure in the hash airportFacilities
  // Also adds ILS references from runways to navaidIds
  aborted = callProgress(SimConnectLoader::tr("Loading airport runways"));
  if((aborted = requestAirports(FACILITY_DATA_AIRPORT_RW_DEFINITION_ID)))
    return true;

  // Fetch start position and fill into the respective airport facility structure in the hash airportFacilities
  // Also connect runway starts to runways
  aborted = callProgress(SimConnectLoader::tr("Loading airport start positions"));
  if((aborted = requestAirports(FACILITY_DATA_AIRPORT_START_DEFINITION_ID)))
    return true;

  // Fetch approaches, SID, STAR and legs and fill into the respective airport facility structure in the hash airportFacilities
  // Also adds all navaid references from fixes and recommended fixes to navaidIds
  aborted = callProgress(SimConnectLoader::tr("Loading airport procedures"));
  if((aborted = requestAirports(FACILITY_DATA_AIRPORT_PROC_DEFINITION_ID)))
    return true;

  // Fetch taxiway and parking and fill into the respective airport facility structure in the hash airportFacilities
  aborted = callProgress(SimConnectLoader::tr("Loading airport taxiways and parking"));
  if((aborted = requestAirports(FACILITY_DATA_AIRPORT_TAXI_DEFINITION_ID)))
    return true;

  if(!aborted)
  {
    // Write all into the database - consumes records from airportFacilities
    aborted = writer->writeAirportsToDatabase(airportFacilities, fileId);
    airportFacilities.clear();
  }
  return aborted;
}

bool SimConnectLoaderPrivate::loadNavaids()
{
  // Consume navaidIds and insert navaids found on routes into the list for further fetching
  // Breadth-first search through network
  aborted = callProgress(SimConnectLoader::tr("Loading waypoints, VOR, ILS, NDB and airways"));
  if(!aborted)
    aborted = requestNavaids(true /* fetchRoutes */);

  if(!aborted)
    // Write all into the database - clears facility lists when done
    aborted = writeNavaidsToDatabase();

  return aborted;
}

bool SimConnectLoaderPrivate::loadDisconnectedNavaids()
{
  // Clear and then fill navaidIds and navaidIdSet avoiding duplicates
  if(!aborted)
    disconnectedNavaids("VNW"); // Load VOR, NDB and waypoints if not loaded previously

  qDebug() << Q_FUNC_INFO << "Number of disconnected to fetch" << navaidIds.size();

  if(!aborted)
    aborted = callProgress(SimConnectLoader::tr("Loading disconnected waypoints, VOR, ILS and NDB"));

  // Request but do not fetch routes
  aborted = requestNavaids(false /* fetchRoutes */);
  if(!aborted)
  {
    aborted = callProgress(SimConnectLoader::tr("Writing disconnected waypoints, VOR, ILS and NDB to database"));
    aborted = writeNavaidsToDatabase();
  }
  return aborted;
}

bool SimConnectLoaderPrivate::writeAirportsToDatabase()
{
  qDebug() << Q_FUNC_INFO << airportFacilities.size();

  if(!aborted)
  {
    // Write to database and commit batch ===================================================
    aborted = writer->writeAirportsToDatabase(airportFacilities, fileId);
    errors.append(writer->getErrors());
  }

  airportFacilities.clear();
  return aborted;
}

bool SimConnectLoaderPrivate::writeNavaidsToDatabase()
{
  qDebug() << Q_FUNC_INFO << "Waypoints" << waypointFacilities.size() << "VOR" << vorFacilities.size() << "NDB"
           << ndbFacilities.size();
  if(!aborted)
  {
    aborted = writer->writeWaypointsAndAirwaysToDatabase(waypointFacilities, fileId);
    errors.append(writer->getErrors());
  }
  waypointFacilities.clear();

  if(!aborted)
  {
    aborted = writer->writeVorAndIlsToDatabase(vorFacilities, fileId);
    errors.append(writer->getErrors());
  }
  vorFacilities.clear();

  if(!aborted)
  {
    aborted = writer->writeNdbToDatabase(ndbFacilities, fileId);
    errors.append(writer->getErrors());
  }
  ndbFacilities.clear();

  return aborted;
}

bool SimConnectLoaderPrivate::requestFacilityData(SIMCONNECT_DATA_DEFINITION_ID defineId, SIMCONNECT_DATA_REQUEST_ID requestId,
                                                  const IcaoId& id)
{
  HRESULT hr = api->RequestFacilityData(defineId, requestId, id.getIdent());
  if(hr != S_OK)
  {
    errors.append("Error in RequestFacilityData");
    return false;
  }

  // Store sendId for later reference if catching SIMCONNECT_RECV_ID_EXCEPTION
  unsigned long sendId = 0;
  api->GetLastSentPacketID(&sendId);
  requests.insert(sendId, FacilityId(id.getIdent()));

  if(verbose)
    qDebug() << Q_FUNC_INFO << "Requested sendId" << sendId << "defineId" << defineId << "requestId" << requestId << id;

  return true;
}

bool SimConnectLoaderPrivate::requestFacilityData(SIMCONNECT_DATA_DEFINITION_ID defineId, SIMCONNECT_DATA_REQUEST_ID requestId,
                                                  const FacilityId& id)
{
  HRESULT hr;

  if(id.getType() != '\0')
    hr = api->RequestFacilityData_EX1(defineId, requestId, id.getIdent(), id.getRegion(), id.getType());
  else
    hr = api->RequestFacilityData(defineId, requestId, id.getIdent());

  if(hr != S_OK)
  {
    errors.append("Error in RequestFacilityData");
    return false;
  }

  // Store sendId for later reference if catching SIMCONNECT_RECV_ID_EXCEPTION
  unsigned long sendId = 0;
  api->GetLastSentPacketID(&sendId);
  requests.insert(sendId, id);

  if(verbose)
    qDebug() << Q_FUNC_INFO << "Requested sendId" << sendId << "defineId" << defineId << "requestId" << requestId << id;

  return true;
}

bool SimConnectLoaderPrivate::requestAirportList()
{
  airportIds.clear();
  facilityListFetched = false;

  // Request a list of airport idents (SIMCONNECT_RECV_ID_AIRPORT_LIST)
  HRESULT hr = api->RequestFacilitiesList(SIMCONNECT_FACILITY_LIST_TYPE_AIRPORT, FACILITY_LIST_REQUEST_AIRPORT_ID);

  // Repeat until requested number of idents was fetched in dispatch
  while(!facilityListFetched && !aborted)
  {
    if(verbose)
      qDebug() << Q_FUNC_INFO << "SimConnect_CallDispatch";
    api->CallDispatch(dispatchFunction, this);
    if(hr != S_OK)
    {
      errors.append("Error in CallDispatch");
      return false;
    }
  }

  // Filter airport idents if requested =============================
  if(!airportIcaoFiltersInc.isEmpty())
  {
    airportIds.erase(std::remove_if(airportIds.begin(), airportIds.end(), [ = ](const IcaoId& id) -> bool {
              bool foundMatch = false;
              for(const QRegExp& regexp : qAsConst(airportIcaoFiltersInc))
              {
                if(regexp.exactMatch(id.getIdentStr()))
                {
                  foundMatch = true;
                  break;
                }
              }
              return !foundMatch;
            }), airportIds.end());
  }

  std::sort(airportIds.begin(), airportIds.end());

  qDebug() << Q_FUNC_INFO << "Fetched" << airportIds.size() << "airports";

  return aborted;
}

bool SimConnectLoaderPrivate::requestAirports(FacilityDataDefinitionId definitionId)
{
  if(aborted)
    return true;

  currentFacilityDefinition = definitionId;

  int requested = 0;
  int size = airportIds.size();
  for(int i = 0; i < size && !aborted; i++)
  {
    bool fetch = true;
    HRESULT hr;
    const IcaoId& id = airportIds.at(i);

    if(airportFacilities.contains(id))
    {
      const Airport& airport = airportFacilities.value(id);
      const AirportFacilityNum& airportFacilityNum = airport.getAirportFacilityNum();
      switch(definitionId)
      {
        case FACILITY_DATA_AIRPORT_BASE_DEFINITION_ID: // Facility is not there yet - will be added in dispatch
          fetch = !airport.isEmptyByNum();
          break;

        case FACILITY_DATA_AIRPORT_FREQ_DEFINITION_ID: // Facility already added - will be filled in dispatch
          fetch = !airport.isCoordinateNull() && airportFacilityNum.numFrequencies != 0;
          break;

        case FACILITY_DATA_AIRPORT_HELIPAD_DEFINITION_ID:
          fetch = !airport.isCoordinateNull() && airportFacilityNum.numHelipads != 0; // Skip if no helipads
          break;

        case FACILITY_DATA_AIRPORT_RW_DEFINITION_ID:
          fetch = !airport.isCoordinateNull() && airportFacilityNum.numRunways != 0; // Skip if no runways
          break;

        case FACILITY_DATA_AIRPORT_START_DEFINITION_ID:
          fetch = !airport.isCoordinateNull() && airportFacilityNum.numStarts != 0; // Skip if no starts
          break;

        case FACILITY_DATA_AIRPORT_PROC_DEFINITION_ID: // Skip if no procedures
          fetch = !airport.isCoordinateNull() &&
                  (airportFacilityNum.numApproaches != 0 || airportFacilityNum.numArrivals != 0 || airportFacilityNum.numDepartures != 0);
          break;

        case FACILITY_DATA_AIRPORT_TAXI_DEFINITION_ID: // Skip if no taxi structures
          fetch = !airport.isCoordinateNull() &&
                  (airportFacilityNum.numTaxiNames != 0 || airportFacilityNum.numTaxiParkings != 0 ||
                   airportFacilityNum.numTaxiPaths != 0 || airportFacilityNum.numTaxiPoints != 0 || airportFacilityNum.numJetways != 0);
          break;

        // Not used here
        case FACILITY_DATA_AIRPORT_NUM_DEFINITION_ID:
        case FACILITY_DATA_NONE_DEFINITION_ID:
        case FACILITY_DATA_AIRPORT_DEFINITION_ID:
        case FACILITY_DATA_NAVAID_DEFINITION_ID:
        case FACILITY_DATA_WAYPOINT_ROUTE_DEFINITION_ID:
        case FACILITY_DATA_WAYPOINT_DEFINITION_ID:
        case FACILITY_DATA_NDB_DEFINITION_ID:
        case FACILITY_DATA_VOR_DEFINITION_ID:
          break;
      }
    }

    if(fetch)
    {
      // Request a list of airport details (SIMCONNECT_RECV_ID_FACILITY_DATA -> SIMCONNECT_FACILITY_DATA_AIRPORT ...)
      if(!requestFacilityData(currentFacilityDefinition, FACILITY_DATA_AIRPORT_REQUEST_ID + static_cast<unsigned int>(i), id))
      {
        errors.append("Error in RequestFacilityData for airports");
        return false;
      }

      requested++;
    }

    // Keep calling RequestFacilityData facility data until batch size matches or last call is done
    if(((i % batchSize) == 0 && i > 0) || i == size - 1)
    {
      // Fetch airport details now
      // Counts incremented by SIMCONNECT_RECV_ID_FACILITY_DATA_END or SIMCONNECT_RECV_ID_EXCEPTION
      facilitiesFetchedBatch = numException = 0;

      while(facilitiesFetchedBatch + numException < requested && !aborted)
      {
        callProgressUpdate();
        if(aborted)
          return true;

        hr = api->CallDispatch(dispatchFunction, this);
        if(hr != S_OK)
        {
          errors.append("Error in CallDispatch for airports");
          return false;
        }

        if(errors.size() > MAX_ERRORS)
        {
          errors.append(SimConnectLoader::tr("Too many errors reading airport data. Stopping."));
          return false;
        }
        QThread::msleep(airportFetchDelay);
      }
      requested = 0;

#ifdef DEBUG_INFORMATION
      qDebug() << Q_FUNC_INFO << i << definitionId;
#endif
    }
  }

  requests.clear();

  return aborted;
}

bool SimConnectLoaderPrivate::requestNavaids(bool fetchRoutes)
{
  if(aborted)
    return true;

  int requested = 0;
  int i = 0;
  while(!navaidIds.isEmpty() && !aborted)
  {
    const FacilityId id = navaidIds.takeFirst();

    // Get right request id based on type of navaid
    // Not unique reqests are enqued again in SIMCONNECT_RECV_ID_FACILITY_MINIMAL_LIST
    SIMCONNECT_DATA_REQUEST_ID requestId;
    if(id.getType() == 'W')
    {
      if(fetchRoutes)
      {
        currentFacilityDefinition = FACILITY_DATA_WAYPOINT_ROUTE_DEFINITION_ID;
        requestId = FACILITY_DATA_WAYPOINT_ROUTE_REQUEST_ID + static_cast<unsigned int>(i);
      }
      else
      {
        currentFacilityDefinition = FACILITY_DATA_WAYPOINT_DEFINITION_ID;
        requestId = FACILITY_DATA_WAYPOINT_REQUEST_ID + static_cast<unsigned int>(i);
      }
    }
    else if(id.getType() == 'V')
    {
      currentFacilityDefinition = FACILITY_DATA_VOR_DEFINITION_ID;
      requestId = FACILITY_DATA_VOR_REQUEST_ID + static_cast<unsigned int>(i);
    }
    else if(id.getType() == 'N')
    {
      currentFacilityDefinition = FACILITY_DATA_NDB_DEFINITION_ID;
      requestId = FACILITY_DATA_NDB_REQUEST_ID + static_cast<unsigned int>(i);
    }
    else
    {
      qWarning() << Q_FUNC_INFO << "Unknown type" << id;
      continue;
    }

    if(!requestFacilityData(currentFacilityDefinition, requestId, id))
    {
      errors.append("Error in RequestFacilityData for navaids");
      return false;
    }

    navaidIdsRequested.insert(id);
    requested++;

    // Keep calling RequestFacilityData facility data until batch size matches or last call is done
    if(((i % batchSize) == 0 && i > 0) || navaidIds.isEmpty())
    {
      // Fetch airport details now
      // Counts incremented by SIMCONNECT_RECV_ID_FACILITY_DATA_END or SIMCONNECT_RECV_ID_EXCEPTION
      facilitiesFetchedBatch = numException = 0;

      while(facilitiesFetchedBatch + numException < requested && !aborted)
      {
        callProgressUpdate();
        if(aborted)
          return true;

        HRESULT hr = api->CallDispatch(dispatchFunction, this);
        if(hr != S_OK)
        {
          errors.append("Error in CallDispatch for navaids");
          return false;
        }

        if(errors.size() > MAX_ERRORS)
        {
          errors.append(SimConnectLoader::tr("Too many errors reading navaid data. Stopping."));
          return false;
        }
        QThread::msleep(navaidFetchDelay);
      }
      requested = 0;

#ifdef DEBUG_INFORMATION
      qDebug() << Q_FUNC_INFO << navaidIds.size();
#endif
    }

    i++;
  }

  requests.clear();

  return aborted;
}

void SimConnectLoaderPrivate::disconnectedNavaids(const QString& typeFilter)
{
  // Query to generate navaids.csv.gz
#if 0
  /* *INDENT-OFF* */
sqlite3 -csv ~/.config/ABarthel/little_navmap_db/little_navmap_msfs.sqlite \
  "select ident, type, format('%.4f', lonx) as lonx, format('%.4f', laty) as laty from ( \
  select ident, region, 'V' as type, lonx, laty from vor \
  union \
  select ident, region, 'N' as type, lonx, laty from ndb \
  union \
  select i.ident, a.region, 'V' as type, i.lonx, i.laty from ils i join airport a on i.loc_airport_ident = a.ident \
  union select ident, region, 'W' as type, lonx, laty from waypoint ) \
  where lonx != 0 and laty != 0 order by ident, region;" > $APROJECTS/atools/resources/navdata/navaids.csv && \
    gzip $APROJECTS/atools/resources/navdata/navaids.csv && \
    ls -lh $APROJECTS/atools/resources/navdata/navaids.csv.gz
  /* *INDENT-ON* */
#endif

  if(!aborted)
  {
    // Clear facilities but not the indexes of loaded navaids navaidIdSet and navaidIdsRequested to avoid loading duplicates
    ndbFacilities.clear();
    vorFacilities.clear();
    waypointFacilities.clear();
    navaidIds.clear();

    // Put the facilities already saved in the database into a multihash with the id minus region as key
    // and the complete without region but plus position as values
    for(const FacilityId& id : writer->getNavaidIds())
      addFacilityIdLoaded(id);

    // File contains all navaids from previous simulator versions - regions are omitted since they are useless
    QFile file(":/atools/resources/navdata/navaids.csv.gz");
    if(file.open(QIODevice::ReadOnly))
    {
      QTextStream stream(atools::zip::gzipDecompress(file.readAll()), QIODevice::ReadOnly);

      // CSV columns
      enum {IDENT, TYPE, LONX, LATY};

      atools::util::CsvFileReader csvReader;
      csvReader.readCsvFile(stream);

      for(const QStringList& row : csvReader.getValues())
      {
        if(!row.at(IDENT).isEmpty())
        {
          QChar type = row.at(TYPE).at(0);
          if(typeFilter.isEmpty() || typeFilter.contains(type))
            // Add coordinates to allow deduplication by ident, type and coordinate
            // Use null region to to disambiguation in minimal list
            addNavaid(FacilityId(row.at(IDENT), nullptr, type, row.at(LONX).toFloat(), row.at(LATY).toFloat()));
        }
      }
    }
    else
      qWarning() << Q_FUNC_INFO << "Cannot open" << file.fileName() << "error" << file.errorString();
  }
}

bool SimConnectLoaderPrivate::facilityIdLoaded(const FacilityId& id)
{
  if(!facilityIdentPosHash.isEmpty())
  {
    // Create key without region
    FacilityId idNoRegion = id.noRegion();

    // Iterate over all same keys having values with different regions and/or coordinates
    for(auto it = facilityIdentPosHash.find(idNoRegion); it != facilityIdentPosHash.end() && it.key() == idNoRegion; ++it)
    {
      // Ignore facilites of the same ident type at the same location
      if(it->getPos().almostEqual(id.getPos(), atools::geo::Pos::POS_EPSILON_10NM))
        return true;
    }
  }
  return false;
}

void SimConnectLoaderPrivate::clear()
{
  errors.clear();
  airportIds.clear();
  navaidIds.clear();
  navaidIdSet.clear();
  navaidIdsRequested.clear();
  airportFacilities.clear();
  waypointFacilities.clear();
  ndbFacilities.clear();
  vorFacilities.clear();
  requests.clear();
  facilityIdentPosHash.clear();
  currentFacilityDefinition = FACILITY_DATA_NONE_DEFINITION_ID;
  airportsLoaded = waypointsLoaded = vorLoaded = ilsLoaded = ndbLoaded = 0;
}

bool SimConnectLoaderPrivate::callProgress(const QString& message, bool incProgress)
{
  // Reset timers used in progress callback in thread context
  if(progressCallback)
  {
    // Reuse last message
    if(!message.isEmpty())
      lastMessage = message;

    // Do not call too often
    if((timer.elapsed() - progressTimerElapsed) > UPDATE_RATE_MS || incProgress)
    {
      progressTimerElapsed = timer.elapsed();

      // Add dot animation
      QString dots;
      if(message.isEmpty())
        dots = SimConnectLoader::tr(" ") % SimConnectLoader::tr(".").repeated((progressCounter++) % 10);

      aborted = progressCallback(lastMessage % dots, incProgress, airportsLoaded, waypointsLoaded, vorLoaded, ilsLoaded, ndbLoaded);
    }
  }
  return aborted;
}

void CALLBACK SimConnectLoaderPrivate::dispatchFunction(SIMCONNECT_RECV *pData, DWORD cbData, void *pContext)
{
  Q_UNUSED(cbData)
  static_cast<SimConnectLoaderPrivate *>(pContext)->dispatchProcedure(pData);
}

void CALLBACK SimConnectLoaderPrivate::dispatchProcedure(SIMCONNECT_RECV *pData)
{
  if(verbose)
    qDebug() << Q_FUNC_INFO << "pData->dwID" << pData->dwID << "pData->dwSize" << pData->dwSize;

  switch(pData->dwID)
  {
    // List of airport idents ==============================================================
    case SIMCONNECT_RECV_ID_AIRPORT_LIST:
      {
        const SIMCONNECT_RECV_AIRPORT_LIST *pAirportList = static_cast<const SIMCONNECT_RECV_AIRPORT_LIST *>(pData);
        for(unsigned int i = 0; i < pAirportList->dwArraySize; i++)
          airportIds.append(IcaoId(pAirportList->rgData[i].Ident));

        // Check if all airports were fetched and set flag if done
        if(pAirportList->dwEntryNumber >= pAirportList->dwOutOf - 1)
          facilityListFetched = true;
      }
      break;

    case SIMCONNECT_RECV_ID_FACILITY_DATA:
      {
        // - UserRequestId  Double word containing the client defined request ID.
        // - UniqueRequestId  The unique request ID, so the client can identify it.
        // - ParentUniqueRequestId  If the current message is about a child object,
        // this field will contain the parent's UniqueRequestId, otherwise it will be 0.
        // - Type Specifies the type of the object, will be a value from the SIMCONNECT_FACILITY_DATA_TYPE enum.
        // - IsListItem If the current message is about a child object, this specifies if it is an orphan object or not.
        // - ItemIndex  If IsListItem is true then this specifies the index in the list.
        // - ListSize If IsListItem is true, then this specifies the list size.
        // - Data Buffer of data. Have to cast it to a struct which matches the definition.
        const SIMCONNECT_RECV_FACILITY_DATA *recvFacilityData = static_cast<const SIMCONNECT_RECV_FACILITY_DATA *>(pData);
        const SIMCONNECT_FACILITY_DATA_TYPE facilityDataType = static_cast<SIMCONNECT_FACILITY_DATA_TYPE>(recvFacilityData->Type);
        const void *facilityData = reinterpret_cast<const void *>(&recvFacilityData->Data);

        if(verbose)
          qDebug() << Q_FUNC_INFO << "pFacilityData->UserRequestId" << recvFacilityData->UserRequestId
                   << "pFacilityData->UniqueRequestId" << recvFacilityData->UniqueRequestId
                   << "pFacilityData->ParentUniqueRequestId" << recvFacilityData->ParentUniqueRequestId
                   << "pFacilityData->Type" << recvFacilityData->Type
                   << "pFacilityData->IsListItem" << recvFacilityData->IsListItem
                   << "pFacilityData->ItemIndex" << recvFacilityData->ItemIndex
                   << "pFacilityData->ListSize" << recvFacilityData->ListSize;

        // ==============================================================================
        // Navdata ======================================================================
        if(facilityDataType == SIMCONNECT_FACILITY_DATA_WAYPOINT)
        {
          // Waypoint with or without route
          const WaypointFacility *waypointFacility = static_cast<const WaypointFacility *>(facilityData);
          waypointFacilities.insert(recvFacilityData->UserRequestId, Waypoint(*waypointFacility));

          // Save key without region for disambiguation when loading disconnected ids
          addFacilityIdLoaded(FacilityId(waypointFacility->icao, waypointFacility->region, ID_WAYPOINT,
                                         waypointFacility->longitude, waypointFacility->latitude));
          waypointsLoaded++;
        }
        else if(facilityDataType == SIMCONNECT_FACILITY_DATA_ROUTE)
        {
          const RouteFacility *routeFacility = static_cast<const RouteFacility *>(facilityData);

          Waypoint *waypoint = nullptr;
          if(waypointFacilities.contains(recvFacilityData->UserRequestId))
            waypoint = &waypointFacilities[recvFacilityData->UserRequestId];

          // Enqueue waypoints to fetch details - ids are only added if not already done
          addNavaid(routeFacility->prevIcao, routeFacility->prevRegion, routeFacility->prevType);
          addNavaid(routeFacility->nextIcao, routeFacility->nextRegion, routeFacility->nextType);

          if(waypoint != nullptr)
            waypoint->routes.append(*routeFacility);
          else
            qWarning() << Q_FUNC_INFO << "Waypoint for" << recvFacilityData->UserRequestId << "not found";
        }
        else if(facilityDataType == SIMCONNECT_FACILITY_DATA_NDB)
        {
          const NdbFacility *ndbFacility = static_cast<const NdbFacility *>(facilityData);
          ndbFacilities.append(*ndbFacility);

          // Save key without region for disambiguation when loading disconnected ids
          addFacilityIdLoaded(FacilityId(ndbFacility->icao, ndbFacility->region, ID_NDB,
                                         ndbFacility->longitude, ndbFacility->latitude));
          ndbLoaded++;
        }
        else if(facilityDataType == SIMCONNECT_FACILITY_DATA_VOR)
        {
          const VorFacility *vorFacility = static_cast<const VorFacility *>(facilityData);
          vorFacilities.append(*vorFacility);

          // Save key without region for disambiguation when loading disconnected ids
          addFacilityIdLoaded(FacilityId(vorFacility->icao, vorFacility->region, ID_VORILS,
                                         vorFacility->vorLongitude, vorFacility->vorLatitude));

          if(vorFacility->isIls())
            ilsLoaded++;
          else
            vorLoaded++;
        }
        // ==============================================================================
        // Airport top level ==============================================================
        // Used in several data definitions
        else if(facilityDataType == SIMCONNECT_FACILITY_DATA_AIRPORT)
        {
          switch(currentFacilityDefinition)
          {
            case FACILITY_DATA_AIRPORT_NUM_DEFINITION_ID:
              {
                // First step in airport loading - add facility to hash map and remember current airport
                const AirportFacilityNum *airportFacilityNum = static_cast<const AirportFacilityNum *>(facilityData);
                currentAirport = &(*airportFacilities.insert(IcaoId(airportFacilityNum->icao), Airport(*airportFacilityNum)));
                legsParentType = legsParentType2 = facilityDataType;
              }
              break;

            case FACILITY_DATA_AIRPORT_BASE_DEFINITION_ID:
              {
                // Find airport facility created above and fill base informaiton
                const AirportFacility *airportFacility = static_cast<const AirportFacility *>(facilityData);
                auto it = airportFacilities.find(IcaoId(airportFacility->icao));
                if(it != airportFacilities.end())
                {
                  currentAirport = &(*it); // Get airport from hash map
                  currentAirport->airport = *static_cast<const AirportFacility *>(facilityData); // Fill base data
                }
                else
                {
                  currentAirport = nullptr;
                  qWarning() << Q_FUNC_INFO << "Airport" << airportFacility->icao << "not found. Definition" << currentFacilityDefinition;
                }
                airportsLoaded++;
              }
              break;

            case FACILITY_DATA_AIRPORT_FREQ_DEFINITION_ID:
            case FACILITY_DATA_AIRPORT_HELIPAD_DEFINITION_ID:
            case FACILITY_DATA_AIRPORT_RW_DEFINITION_ID:
            case FACILITY_DATA_AIRPORT_START_DEFINITION_ID:
            case FACILITY_DATA_AIRPORT_PROC_DEFINITION_ID:
            case FACILITY_DATA_AIRPORT_TAXI_DEFINITION_ID:
              {
                // Find airport facility created above - will filled with the requested details further down
                const AirportFacilityIcao *airportFacilityIcao = static_cast<const AirportFacilityIcao *>(facilityData);
                auto it = airportFacilities.find(IcaoId(airportFacilityIcao->icao));
                if(it != airportFacilities.end())
                  currentAirport = &(*it);
                else
                {
                  currentAirport = nullptr;
                  qWarning() << Q_FUNC_INFO << "Airport" << airportFacilityIcao->icao << "not found. Definition"
                             << currentFacilityDefinition;
                }
              }
              break;

            case FACILITY_DATA_AIRPORT_DEFINITION_ID:
            case FACILITY_DATA_NONE_DEFINITION_ID:
            case FACILITY_DATA_NAVAID_DEFINITION_ID:
            case FACILITY_DATA_WAYPOINT_ROUTE_DEFINITION_ID:
            case FACILITY_DATA_WAYPOINT_DEFINITION_ID:
            case FACILITY_DATA_NDB_DEFINITION_ID:
            case FACILITY_DATA_VOR_DEFINITION_ID:
              break;
          }
        }
        else
        {
          // ==============================================================================
          // Airport children ==============================================================
          if(currentAirport != nullptr)
          {
            switch(facilityDataType)
            {
              // Runway ================================================================================
              case SIMCONNECT_FACILITY_DATA_RUNWAY:
                {
                  const RunwayFacility *runway = static_cast<const RunwayFacility *>(facilityData);
                  currentAirport->runways.append(Runway(*runway));

                  // Add ILS ids to be fetched later in loadNavaids()
                  addNavaid(runway->primaryIlsIcao, runway->primaryIlsRegion, ID_VORILS);
                  addNavaid(runway->secondaryIlsIcao, runway->secondaryIlsRegion, ID_VORILS);
                }
                break;

              // Primary and secondary Threshold, blastpad or overrun ==============================================================
              case SIMCONNECT_FACILITY_DATA_PAVEMENT:
                {
                  const PavementFacility *pavement = static_cast<const PavementFacility *>(facilityData);

                  if(!currentAirport->runways.isEmpty())
                    currentAirport->runways.last().pavements.append(*pavement);
                  else
                    qWarning() << Q_FUNC_INFO << "Runways for pavement empty" << recvFacilityData->UserRequestId;
                }
                break;

              // Primary and secondary approach lights ================================================================================
              case SIMCONNECT_FACILITY_DATA_APPROACH_LIGHTS:
                {
                  const ApproachLightFacility *lights = static_cast<const ApproachLightFacility *>(facilityData);

                  if(!currentAirport->runways.isEmpty())
                    currentAirport->runways.last().approachLights.append(*lights);
                  else
                    qWarning() << Q_FUNC_INFO << "Runways for approach lights empty" << recvFacilityData->UserRequestId;
                }
                break;

              // Left/right primary and secondary VASI ==============================================================================
              case SIMCONNECT_FACILITY_DATA_VASI:
                {
                  const VasiFacility *vasi = static_cast<const VasiFacility *>(facilityData);

                  if(!currentAirport->runways.isEmpty())
                    currentAirport->runways.last().vasi.append(*vasi);
                  else
                    qWarning() << Q_FUNC_INFO << "Runways for VASI empty" << recvFacilityData->UserRequestId;
                }
                break;

              // Start positions ================================================================================
              case SIMCONNECT_FACILITY_DATA_START:
                currentAirport->starts.append(*static_cast<const StartFacility *>(facilityData));
                break;

              // COM ================================================================================
              case SIMCONNECT_FACILITY_DATA_FREQUENCY:
                currentAirport->frequencies.append(*static_cast<const FrequencyFacility *>(facilityData));
                break;

              // Helipad ================================================================================
              case SIMCONNECT_FACILITY_DATA_HELIPAD:
                currentAirport->helipads.append(*static_cast<const HelipadFacility *>(facilityData));
                break;

              // Approach ================================================================================
              case SIMCONNECT_FACILITY_DATA_APPROACH:
                {
                  const ApproachFacility *approach = static_cast<const ApproachFacility *>(facilityData);
                  currentAirport->approaches.append(Approach(*approach));

                  // Add referenced FAF to be fetched later in loadNavaids()
                  addNavaid(approach->fafIcao, approach->fafRegion, approach->fafType);
                }
                break;

              // ================================================================================
              case SIMCONNECT_FACILITY_DATA_FINAL_APPROACH_LEG:
                {
                  const LegFacility *leg = static_cast<const LegFacility *>(facilityData);

                  // Add referenced fixes to be fetched later in loadNavaids()
                  addNavaidsForLeg(leg);

                  if(!currentAirport->approaches.isEmpty())
                    currentAirport->approaches.last().finalApproachLegs.append(*leg);
                  else
                    qWarning() << Q_FUNC_INFO << "Approaches for final legs empty" << recvFacilityData->UserRequestId;
                }
                break;

              // ================================================================================
              case SIMCONNECT_FACILITY_DATA_MISSED_APPROACH_LEG:
                {
                  const LegFacility *leg = static_cast<const LegFacility *>(facilityData);

                  // Add referenced fixes to be fetched later in loadNavaids()
                  addNavaidsForLeg(leg);

                  if(!currentAirport->approaches.isEmpty())
                    currentAirport->approaches.last().missedApproachLegs.append(*leg);
                  else
                    qWarning() << Q_FUNC_INFO << "Approaches for missed legs empty" << recvFacilityData->UserRequestId;
                }
                break;

              // ================================================================================
              case SIMCONNECT_FACILITY_DATA_APPROACH_TRANSITION:
                {
                  legsParentType = facilityDataType;
                  const ApproachTransitionFacility *transition =
                    static_cast<const ApproachTransitionFacility *>(facilityData);

                  // Add referenced IAF and DME Arc fixes to be fetched later in loadNavaids()
                  addNavaid(transition->iafIcao, transition->iafRegion, transition->iafType);
                  addNavaid(transition->dmeArcIcao, transition->dmeArcRegion, transition->dmeArcType);

                  if(!currentAirport->approaches.isEmpty())
                    currentAirport->approaches.last().approachTransitions.append(ApproachTransition(*transition));
                  else
                    qWarning() << Q_FUNC_INFO << "Approaches for transitions empty" << recvFacilityData->UserRequestId;
                }
                break;

              // Procedure legs ================================================================================
              case SIMCONNECT_FACILITY_DATA_APPROACH_LEG:
                {
                  const LegFacility *leg = static_cast<const LegFacility *>(facilityData);

                  // Add referenced fixes to be fetched later in loadNavaids()
                  addNavaidsForLeg(leg);

                  // Add legs depending on parent type ==================
                  if(legsParentType == SIMCONNECT_FACILITY_DATA_APPROACH_TRANSITION)
                  {
                    if(!currentAirport->approaches.isEmpty() && !currentAirport->approaches.last().approachTransitions.isEmpty())
                      currentAirport->approaches.last().approachTransitions.last().transitionLegs.append(*leg);
                    else
                      qWarning() << Q_FUNC_INFO << "Transitions for legs empty" << recvFacilityData->UserRequestId;
                  }
                  else if(legsParentType == SIMCONNECT_FACILITY_DATA_ARRIVAL)
                  {
                    // STAR legs =======================================================================
                    if(!currentAirport->arrivals.isEmpty())
                    {
                      if(legsParentType2 == SIMCONNECT_FACILITY_DATA_RUNWAY_TRANSITION)
                      {
                        if(!currentAirport->arrivals.last().runwayTransitions.isEmpty())
                          currentAirport->arrivals.last().runwayTransitions.last().legs.append(*leg);
                        else
                          qWarning() << Q_FUNC_INFO << "Arrival for runway trans legs empty" << recvFacilityData->UserRequestId;
                      }
                      else if(legsParentType2 == SIMCONNECT_FACILITY_DATA_ENROUTE_TRANSITION)
                      {
                        if(!currentAirport->arrivals.last().enrouteTransitions.isEmpty())
                          currentAirport->arrivals.last().enrouteTransitions.last().legs.append(*leg);
                        else
                          qWarning() << Q_FUNC_INFO << "Arrival for enroute trans legs empty" << recvFacilityData->UserRequestId;
                      }
                      else
                        currentAirport->arrivals.last().approachLegs.append(*leg);
                    }
                    else
                      qWarning() << Q_FUNC_INFO << "Arrivals empty" << recvFacilityData->UserRequestId;
                  }
                  else if(legsParentType == SIMCONNECT_FACILITY_DATA_DEPARTURE)
                  {
                    // SID legs =======================================================================
                    if(!currentAirport->departures.isEmpty())
                    {
                      if(legsParentType2 == SIMCONNECT_FACILITY_DATA_RUNWAY_TRANSITION)
                      {
                        if(!currentAirport->departures.last().runwayTransitions.isEmpty())
                          currentAirport->departures.last().runwayTransitions.last().legs.append(*leg);
                        else
                          qWarning() << Q_FUNC_INFO << "Departures for runway trans legs empty" << recvFacilityData->UserRequestId;
                      }
                      else if(legsParentType2 == SIMCONNECT_FACILITY_DATA_ENROUTE_TRANSITION)
                      {
                        if(!currentAirport->departures.last().enrouteTransitions.isEmpty())
                          currentAirport->departures.last().enrouteTransitions.last().legs.append(*leg);
                        else
                          qWarning() << Q_FUNC_INFO << "Departures for enroute trans legs empty" << recvFacilityData->UserRequestId;
                      }
                      else
                        currentAirport->departures.last().approachLegs.append(*leg);
                    }
                    else
                      qWarning() << Q_FUNC_INFO << "Departures empty" << recvFacilityData->UserRequestId;
                  }
                }
                break;

              // SID ================================================================================
              case SIMCONNECT_FACILITY_DATA_DEPARTURE:
                currentAirport->departures.append(Departure(*static_cast<const DepartureFacility *>(facilityData)));
                legsParentType = facilityDataType;
                legsParentType2 = SIMCONNECT_FACILITY_DATA_AIRPORT;
                break;

              // STAR ================================================================================
              case SIMCONNECT_FACILITY_DATA_ARRIVAL:
                currentAirport->arrivals.append(Arrival(*static_cast<const ArrivalFacility *>(facilityData)));
                legsParentType = facilityDataType;
                legsParentType2 = SIMCONNECT_FACILITY_DATA_AIRPORT;
                break;

              // SID/STAR runway transitions ================================================================================
              case SIMCONNECT_FACILITY_DATA_RUNWAY_TRANSITION:
                {
                  const RunwayTransitionFacility *transition = static_cast<const RunwayTransitionFacility *>(facilityData);

                  if(legsParentType == SIMCONNECT_FACILITY_DATA_ARRIVAL)
                  {
                    if(!currentAirport->arrivals.isEmpty())
                      currentAirport->arrivals.last().runwayTransitions.append(RunwayTransition(*transition));
                  }
                  else if(legsParentType == SIMCONNECT_FACILITY_DATA_DEPARTURE)
                  {
                    if(!currentAirport->departures.isEmpty())
                      currentAirport->departures.last().runwayTransitions.append(RunwayTransition(*transition));
                  }
                  else
                    qWarning() << Q_FUNC_INFO << "Wrong parent type for runway trans" << recvFacilityData->UserRequestId;

                  legsParentType2 = facilityDataType;
                }
                break;

              // SID/STAR transitions ================================================================================
              case SIMCONNECT_FACILITY_DATA_ENROUTE_TRANSITION:
                {
                  const EnrouteTransitionFacility *transition = static_cast<const EnrouteTransitionFacility *>(facilityData);

                  if(legsParentType == SIMCONNECT_FACILITY_DATA_ARRIVAL)
                  {
                    if(!currentAirport->arrivals.isEmpty())
                      currentAirport->arrivals.last().enrouteTransitions.append(EnrouteTransition(*transition));
                    else
                      qWarning() << Q_FUNC_INFO << "Arrivals for enroute trans empty" << recvFacilityData->UserRequestId;
                  }
                  else if(legsParentType == SIMCONNECT_FACILITY_DATA_DEPARTURE)
                  {
                    if(!currentAirport->departures.isEmpty())
                      currentAirport->departures.last().enrouteTransitions.append(EnrouteTransition(*transition));
                    else
                      qWarning() << Q_FUNC_INFO << "Departures for enroute trans empty" << recvFacilityData->UserRequestId;
                  }
                  else
                    qWarning() << Q_FUNC_INFO << "Wrong parent type for enroute trans" << recvFacilityData->UserRequestId;

                  legsParentType2 = facilityDataType;
                }
                break;

              // Taxi ================================================================================
              // Parking spots ==================================================================
              case SIMCONNECT_FACILITY_DATA_TAXI_PARKING:
                currentAirport->taxiParkings.append(*static_cast<const TaxiParkingFacility *>(facilityData));
                break;

              // Taxi intersections ===================================================================
              case SIMCONNECT_FACILITY_DATA_TAXI_POINT:
                currentAirport->taxiPoints.append(*static_cast<const TaxiPointFacility *>(facilityData));
                break;

              // Paths ================================================================================
              case SIMCONNECT_FACILITY_DATA_TAXI_PATH:
                currentAirport->taxiPaths.append(*static_cast<const TaxiPathFacility *>(facilityData));
                break;

              // Names ================================================================================
              case SIMCONNECT_FACILITY_DATA_TAXI_NAME:
                currentAirport->taxiNames.append(*static_cast<const TaxiNameFacility *>(facilityData));
                break;

              // Jetways ================================================================================
              case SIMCONNECT_FACILITY_DATA_JETWAY:
                currentAirport->jetways.append(*static_cast<const JetwayFacility *>(facilityData));
                break;

              // Not used here
              case SIMCONNECT_FACILITY_DATA_AIRPORT:
              case SIMCONNECT_FACILITY_DATA_VOR:
              case SIMCONNECT_FACILITY_DATA_NDB:
              case SIMCONNECT_FACILITY_DATA_WAYPOINT:
              case SIMCONNECT_FACILITY_DATA_ROUTE:
              case SIMCONNECT_FACILITY_DATA_VDGS:
              case SIMCONNECT_FACILITY_DATA_HOLDING_PATTERN:
                break;
            } // switch(facilityType)
          } // if(curAirport != nullptr)
          else
            qWarning() << Q_FUNC_INFO << "Airport for" << recvFacilityData->UserRequestId << "not found";
        } // if(facilityType == SIMCONNECT_FACILITY_DATA_AIRPORT) ... else
      }
      break;

    // List for disambiguation =================================================================================
    case SIMCONNECT_RECV_ID_FACILITY_MINIMAL_LIST:
      {
        const SIMCONNECT_RECV_FACILITY_MINIMAL_LIST *pList = static_cast<const SIMCONNECT_RECV_FACILITY_MINIMAL_LIST *>(pData);

        // Enqueue navaid ids from minimal list for further fetch operations
        for(unsigned int i = 0; i < pList->dwArraySize; i++)
        {
          const SIMCONNECT_FACILITY_MINIMAL& facility = pList->rgData[i];
          if(verbose)
            qDebug() << Q_FUNC_INFO << "Minimal list" << i << facility.icao.Ident << facility.icao.Region << facility.icao.Type;

          // Add coordinates to allow deduplication by ident, type and coordinate
          addNavaid(facility.icao.Ident, facility.icao.Region, facility.icao.Type, facility.lla.Longitude, facility.lla.Latitude);
        }

        facilitiesFetchedBatch++;
      }
      break;

    // End of facility data =================================================================================
    case SIMCONNECT_RECV_ID_FACILITY_DATA_END:
      {
        // End of a facility block for one facility definition, i.e. airport structure and children
        const SIMCONNECT_RECV_FACILITY_DATA_END *pFacilityDataEnd = static_cast<const SIMCONNECT_RECV_FACILITY_DATA_END *>(pData);
        if(verbose)
          qDebug() << Q_FUNC_INFO << "SIMCONNECT_RECV_ID_FACILITY_DATA_END" << pFacilityDataEnd->dwID << pFacilityDataEnd->RequestId;
        facilitiesFetchedBatch++;
      }
      break;

    case SIMCONNECT_RECV_ID_EXCEPTION:
      {
        // Count errors too to avoid endless looping
        const SIMCONNECT_RECV_EXCEPTION *pException = static_cast<const SIMCONNECT_RECV_EXCEPTION *>(pData);
        qWarning() << Q_FUNC_INFO << "dwException " << pException->dwException << "dwSendID" << pException->dwSendID
                   << "dwIndex" << pException->dwIndex << "dwID" << pException->dwID << "requested" << requests.value(pException->dwSendID);
        numException++;
      }
      break;
  } // switch(pData->dwID)
}

#endif

// ==================================================================================================================
// SimConnectLoader =================================================================================================
SimConnectLoader::SimConnectLoader(const win::ActivationContext *activationContext, const QString& libraryName,
                                   atools::sql::SqlDatabase& sqlDb, bool verbose)
{
#if !defined(SIMCONNECT_BUILD_WIN32)
  p = new SimConnectLoaderPrivate(activationContext, sqlDb, libraryName, verbose);
#else
  Q_UNUSED(activationContext)
  Q_UNUSED(libraryName)
  Q_UNUSED(sqlDb)
  Q_UNUSED(verbose)
#endif
}

SimConnectLoader::~SimConnectLoader()
{
#if !defined(SIMCONNECT_BUILD_WIN32)
  delete p;
#endif
}

bool SimConnectLoader::loadAirports(int fileId)
{
#if !defined(SIMCONNECT_BUILD_WIN32)
  p->fileId = fileId;
  return p->loadAirports();
#else
  Q_UNUSED(fileId)
  return false;
#endif
}

bool SimConnectLoader::loadNavaids(int fileId)
{
#if !defined(SIMCONNECT_BUILD_WIN32)
  p->fileId = fileId;
  return p->loadNavaids();
#else
  Q_UNUSED(fileId)
  return false;
#endif
}

bool SimConnectLoader::loadDisconnectedNavaids(int fileId)
{
#if !defined(SIMCONNECT_BUILD_WIN32)
  p->fileId = fileId;
  return p->loadDisconnectedNavaids();
#else
  Q_UNUSED(fileId)
  return false;
#endif
}

bool SimConnectLoader::prepareLoading(bool loadFacilityDefinitions, bool initSqlQueries)
{
#if !defined(SIMCONNECT_BUILD_WIN32)
  if(initSqlQueries)
    p->writer->initQueries();

  p->aborted = false;

  HRESULT hr = p->api->Open(QCoreApplication::applicationName().toLatin1().constData(), nullptr, 0, nullptr, 0);

  if(hr != S_OK)
  {
    p->errors.append("Error opening SimConnect.");
    return false;
  }
  else if(loadFacilityDefinitions)
  {
    p->addAirportNumFacilityDefinition();
    p->addAirportBaseFacilityDefinition();
    p->addAirportFrequencyFacilityDefinition();
    p->addAirportHelipadFacilityDefinition();
    p->addAirportRunwayFacilityDefinition();
    p->addAirportStartFacilityDefinition();
    p->addAirportProcedureFacilityDefinition();
    p->addAirportTaxiFacilityDefinition();
    p->addNavFacilityDefinition();
  }
#endif
  return true;
}

bool SimConnectLoader::finishLoading()
{
#if !defined(SIMCONNECT_BUILD_WIN32)
  p->writer->deInitQueries();
  p->clear();
  HRESULT hr = p->api->Close();
  if(hr != S_OK)
  {
    p->errors.append("Error closing SimConnect.");
    return false;
  }
#endif
  return true;
}

int SimConnectLoader::getNumSteps() const
{
#if !defined(SIMCONNECT_BUILD_WIN32)
  // grep -c "aborted = callProgress(" simconnect*.cpp
  // SimConnect reportOtherInc "Loading airport count"
  // SimConnect reportOtherInc "Loading airport facility numbers"
  // SimConnect reportOtherInc "Loading airport base information"
  // SimConnect reportOtherInc "Loading airport COM"
  // SimConnect reportOtherInc "Loading airport helipads"
  // SimConnect reportOtherInc "Loading airport runways"
  // SimConnect reportOtherInc "Loading airport start positions"
  // SimConnect reportOtherInc "Loading airport procedures"
  // SimConnect reportOtherInc "Loading airport taxiways and parking"
  // SimConnect reportOtherInc "Writing airport facilities to database"
  // SimConnect reportOtherInc "Loading waypoints, VOR, ILS, NDB and airways"
  // SimConnect reportOtherInc "Writing waypoints and airways to database"
  // SimConnect reportOtherInc "Writing VOR and ILS to database"
  // SimConnect reportOtherInc "Writing NDB to database"
  return 16; // With disconnected enabled otherwise 14
#else
  return 0;
#endif
}

bool SimConnectLoader::isAborted() const
{
#if !defined(SIMCONNECT_BUILD_WIN32)
  return p->aborted;
#else
  return false;
#endif
}

void SimConnectLoader::setBatchSize(int value)
{
#if !defined(SIMCONNECT_BUILD_WIN32)
  p->batchSize = value;
#else
  Q_UNUSED(value)
#endif
}

const QStringList& SimConnectLoader::getErrors() const
{
  const static QStringList EMPTY;
#if !defined(SIMCONNECT_BUILD_WIN32)
  return p->errors;
#else
  return EMPTY;
#endif
}

void SimConnectLoader::setAirportIdents(const QList<QRegExp>& airportIcaoFiltersInc)
{
#if !defined(SIMCONNECT_BUILD_WIN32)
  p->airportIcaoFiltersInc = airportIcaoFiltersInc;
#else
  Q_UNUSED(airportIcaoFiltersInc)
#endif
}

void SimConnectLoader::setProgressCallback(const SimConnectLoaderProgressCallback& callback)
{
#if !defined(SIMCONNECT_BUILD_WIN32)
  p->progressCallback = callback;
  p->writer->setProgressCallback([ = ](const QString& message, bool incProgress)->bool {
            p->aborted = p->callProgress(message, incProgress);
            return p->aborted;
          });
#else
  Q_UNUSED(callback)
#endif
}

void SimConnectLoader::setAirportFetchDelay(int value)
{
#if !defined(SIMCONNECT_BUILD_WIN32)
  p->airportFetchDelay = value;
#else
  Q_UNUSED(value)
#endif
}

void SimConnectLoader::setNavaidFetchDelay(int value)
{
#if !defined(SIMCONNECT_BUILD_WIN32)
  p->navaidFetchDelay = value;
#else
  Q_UNUSED(value)
#endif
}

} // namespace db
} // namespace sc
} // namespace fs
} // namespace atools
