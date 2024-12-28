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

#include "fs/sc/airport/simconnectloader.h"

#include "atools.h"
#include "fs/sc/airport/simconnectfacilities.h"
#include "fs/sc/airport/simconnectwriter.h"
#include "fs/sc/simconnectapi.h"

#include <QString>
#include <QStringBuilder>
#include <QDebug>
#include <QCoreApplication>
#include <QThread>
#include <QRegExp>

namespace atools {
namespace fs {
namespace sc {
namespace airport {

#if !defined(SIMCONNECT_BUILD_WIN32)
const static SIMCONNECT_DATA_DEFINITION_ID FACILITY_DATA_AIRPORT_DEFINITION_ID = 1000;
const static SIMCONNECT_DATA_REQUEST_ID FACILITY_LIST_REQUEST_AIRPORT_ID = 100;
// const static SIMCONNECT_DATA_REQUEST_ID FACILITY_LIST_REQUEST_WAYPOINT_ID = 200;
const static SIMCONNECT_DATA_REQUEST_ID FACILITY_DATA_REQUEST_ID = 1000;

// ====================================================================================================================
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

  // Load all airport details for idents in airportIdentDetail and write to database in callback
  bool loadAirports();

  // Load airpport idents into airportIdents
  bool loadAirportIdents();

  // Add all facility definitions for airports and their children
  // The definitions have to match the structs in fs/sc/airport/simconnectfacilities.h
  void addFacilityDefinitions();

  // Add facility definition using builder pattern
  SimConnectLoaderPrivate& add(const char *name)
  {
    HRESULT hr = api->AddToFacilityDefinition(FACILITY_DATA_AIRPORT_DEFINITION_ID, name);

    if(hr != S_OK)
    {
      QString err = QString("Error adding facility definition \"%1\"").arg(name);
      qWarning() << Q_FUNC_INFO << err;
      errors.append(err);
    }
    return *this;
  }

  // Open facility defition
  SimConnectLoaderPrivate& open(const char *name)
  {
    add(QString("OPEN " % QString(name)).toLatin1().constData());
    return *this;
  }

  // Close facility defition
  SimConnectLoaderPrivate& close(const char *name)
  {
    add(QString("CLOSE " % QString(name)).toLatin1().constData());
    return *this;
  }

  // Add procedure legs definition
  SimConnectLoaderPrivate& addLegs(const char *name);

  // Add arrival or departure transition definition
  SimConnectLoaderPrivate& addArrivalDepartureTrans();

  // Call writer and write all airports from the airportMap to the database
  bool writeAirportBatchToDatabase();

  // SimConnect dispatch procedure. pContext is this
  static void dispatchProcedure(SIMCONNECT_RECV *pData, DWORD cbData, void *pContext);

  QStringList errors, airportIdents /* fetched idents */, airportIdentDetail /* Idents to fetch detail from */;
  QVector<SIMCONNECT_DATA_FACILITY_WAYPOINT> waypointIdents; // Not used

  // Used to select airport ident to load if not empty
  QList<QRegExp> airportIcaoFiltersInc;

  // DLL bindings
  atools::fs::sc::SimConnectApi *api;

  // Airport map filled when fetching details - emptied after writing batch
  // Key is pFacilityData->UserRequestId
  QMap<unsigned long, atools::fs::sc::airport::Airport> airportMap;

  SIMCONNECT_FACILITY_DATA_TYPE legsParentType = SIMCONNECT_FACILITY_DATA_AIRPORT;
  SIMCONNECT_FACILITY_DATA_TYPE legsParentType2 = SIMCONNECT_FACILITY_DATA_AIRPORT;

  // Database writer
  atools::fs::sc::airport::SimConnectWriter *writer = nullptr;

  SimConnectLoaderProgressCallback progressCallback = nullptr;

  bool verbose = false, quit = false, facilitiesFetched = false, aborted = false;

  int airportsFetchedBatch = 0, airportsFetchedTotal = 0, batchSize = 2000, fileId = 0, totalWritten = 0;

  const int MAX_ERRORS = 100;
};

SimConnectLoaderPrivate& SimConnectLoaderPrivate::addArrivalDepartureTrans()
{
  addLegs("APPROACH_LEG");
  open("RUNWAY_TRANSITION").add("RUNWAY_NUMBER").add("RUNWAY_DESIGNATOR").addLegs("APPROACH_LEG").close("RUNWAY_TRANSITION");
  open("ENROUTE_TRANSITION").add("NAME").addLegs("APPROACH_LEG").close("ENROUTE_TRANSITION");
  return *this;
}

SimConnectLoaderPrivate& SimConnectLoaderPrivate::addLegs(const char *name)
{
  open(name);
  add("TYPE");
  add("FIX_ICAO").add("FIX_REGION").add("FIX_TYPE").add("FIX_LATITUDE").add("FIX_LONGITUDE").add("FIX_ALTITUDE");
  add("FLY_OVER").add("DISTANCE_MINUTE").add("TRUE_DEGREE").add("TURN_DIRECTION");
  add("ORIGIN_ICAO").add("ORIGIN_REGION").add("ORIGIN_TYPE").add("ORIGIN_LATITUDE").add("ORIGIN_LONGITUDE").add("ORIGIN_ALTITUDE");
  add("THETA").add("RHO").add("COURSE").add("ROUTE_DISTANCE").add("APPROACH_ALT_DESC").add("ALTITUDE1").add("ALTITUDE2");
  add("SPEED_LIMIT").add("VERTICAL_ANGLE");
  add("ARC_CENTER_FIX_ICAO").add("ARC_CENTER_FIX_REGION").add("ARC_CENTER_FIX_TYPE").add("ARC_CENTER_FIX_LATITUDE");
  add("ARC_CENTER_FIX_LONGITUDE").add("ARC_CENTER_FIX_ALTITUDE");
  add("RADIUS").add("IS_IAF").add("IS_IF").add("IS_FAF").add("IS_MAP").add("REQUIRED_NAVIGATION_PERFORMANCE");
  close(name);
  return *this;
}

void SimConnectLoaderPrivate::addFacilityDefinitions()
{
  // =============================================================================
  // Airport - top level =======================================================
  open("AIRPORT");

  add("LATITUDE").add("LONGITUDE").add("ALTITUDE");
  add("MAGVAR").add("NAME64").add("ICAO").add("REGION");
  add("TOWER_LATITUDE").add("TOWER_LONGITUDE").add("TOWER_ALTITUDE");
  add("TRANSITION_ALTITUDE").add("TRANSITION_LEVEL");

  // =============================================================================
  // Runway =======================================================
  open("RUNWAY");
  add("LATITUDE").add("LONGITUDE").add("ALTITUDE");
  add("HEADING").add("LENGTH").add("WIDTH");
  add("PATTERN_ALTITUDE");
  add("SURFACE");

  // Primary =======================
  add("PRIMARY_NUMBER").add("PRIMARY_DESIGNATOR");
  add("PRIMARY_ILS_ICAO").add("PRIMARY_ILS_REGION").add("PRIMARY_ILS_TYPE");
  open("PRIMARY_THRESHOLD").add("LENGTH").close("PRIMARY_THRESHOLD");
  open("PRIMARY_BLASTPAD").add("LENGTH").close("PRIMARY_BLASTPAD");
  open("PRIMARY_OVERRUN").add("LENGTH").close("PRIMARY_OVERRUN");
  open("PRIMARY_APPROACH_LIGHTS").add("SYSTEM").add("HAS_END_LIGHTS").add("HAS_REIL_LIGHTS").
  add("HAS_TOUCHDOWN_LIGHTS").add("ENABLE").close("PRIMARY_APPROACH_LIGHTS");
  open("PRIMARY_LEFT_VASI").add("TYPE").add("ANGLE").close("PRIMARY_LEFT_VASI");
  open("PRIMARY_RIGHT_VASI").add("TYPE").add("ANGLE").close("PRIMARY_RIGHT_VASI");

  // Secondary =======================
  add("SECONDARY_NUMBER").add("SECONDARY_DESIGNATOR");
  add("SECONDARY_ILS_ICAO").add("SECONDARY_ILS_REGION").add("SECONDARY_ILS_TYPE");
  open("SECONDARY_THRESHOLD").add("LENGTH").close("SECONDARY_THRESHOLD");
  open("SECONDARY_BLASTPAD").add("LENGTH").close("SECONDARY_BLASTPAD");
  open("SECONDARY_OVERRUN").add("LENGTH").close("SECONDARY_OVERRUN");
  open("SECONDARY_APPROACH_LIGHTS").add("SYSTEM").add("HAS_END_LIGHTS").add("HAS_REIL_LIGHTS").
  add("HAS_TOUCHDOWN_LIGHTS").add("ENABLE").close("SECONDARY_APPROACH_LIGHTS");
  open("SECONDARY_LEFT_VASI").add("TYPE").add("ANGLE").close("SECONDARY_LEFT_VASI");
  open("SECONDARY_RIGHT_VASI").add("TYPE").add("ANGLE").close("SECONDARY_RIGHT_VASI");
  close("RUNWAY");

  // =============================================================================
  // Start =======================================================
  open("START").add("LATITUDE").add("LONGITUDE").add("ALTITUDE").add("HEADING").add("NUMBER").add("DESIGNATOR").add("TYPE").close("START");

  // =============================================================================
  // Frequency =======================================================
  open("FREQUENCY").add("TYPE").add("FREQUENCY").add("NAME").close("FREQUENCY");

  // =============================================================================
  // Helipad =======================================================
  open("HELIPAD").add("LATITUDE").add("LONGITUDE").add("ALTITUDE").add("HEADING").add("LENGTH").add("WIDTH").add("SURFACE").add("TYPE").
  close("HELIPAD");

  // =============================================================================
  // Procedures =======================================================
  // Approach ================================
  open("APPROACH").add("TYPE").add("SUFFIX").add("RUNWAY_NUMBER").add("RUNWAY_DESIGNATOR").add("FAF_ICAO").add("FAF_REGION").
  add("FAF_ALTITUDE").add("FAF_TYPE").add("MISSED_ALTITUDE").add("IS_RNPAR");

  // Approach Legs =============
  addLegs("FINAL_APPROACH_LEG");

  // Missed Approach Legs =============
  addLegs("MISSED_APPROACH_LEG");

  // Approach Transition ===========
  open("APPROACH_TRANSITION").add("TYPE").add("IAF_ICAO").add("IAF_REGION").add("IAF_TYPE").add("IAF_ALTITUDE").add("DME_ARC_ICAO").
  add("DME_ARC_REGION").add("DME_ARC_TYPE").add("DME_ARC_RADIAL").add("DME_ARC_DISTANCE").add("NAME");

  // Approach Transition Legs =============
  addLegs("APPROACH_LEG").close("APPROACH_TRANSITION");
  close("APPROACH");

  // SID / Departure ================================
  open("DEPARTURE").add("NAME").addArrivalDepartureTrans().close("DEPARTURE");

  // STAR / Arrival ================================
  open("ARRIVAL").add("NAME").addArrivalDepartureTrans().close("ARRIVAL");

  // =============================================================================
  // Taxiways =======================================================
  // Taxi Parking ================================
  open("TAXI_PARKING").add("TYPE").add("TAXI_POINT_TYPE").add("NAME").add("SUFFIX").add("NUMBER").add("ORIENTATION").add("HEADING").
  add("RADIUS").add("BIAS_X").add("BIAS_Z").close("TAXI_PARKING");

  // Taxi Point ================================
  open("TAXI_POINT").add("TYPE").add("ORIENTATION").add("BIAS_X").add("BIAS_Z").close("TAXI_POINT");

  // Taxi Path ================================
  open("TAXI_PATH").add("TYPE").add("WIDTH").add("RUNWAY_NUMBER").
  add("RUNWAY_DESIGNATOR").add("LEFT_EDGE_LIGHTED").add("RIGHT_EDGE_LIGHTED").add("CENTER_LINE_LIGHTED").
  add("START").add("END").add("NAME_INDEX").close("TAXI_PATH");

  // Taxi Name ================================
  open("TAXI_NAME").add("NAME").close("TAXI_NAME");

  // =============================================================================
  // Jetways =======================================================
  open("JETWAY").add("PARKING_GATE").add("PARKING_SUFFIX").add("PARKING_SPOT").close("JETWAY");

  close("AIRPORT");
}

bool SimConnectLoaderPrivate::loadAirportIdents()
{
  airportIdents.clear();
  facilitiesFetched = false;

  HRESULT hr = api->Open(QCoreApplication::applicationName().toLatin1().constData(), nullptr, 0, nullptr, 0);
  if(hr == S_OK)
  {
    // Request a list of airport idents (SIMCONNECT_RECV_ID_AIRPORT_LIST)
    hr = api->RequestFacilitiesList(SIMCONNECT_FACILITY_LIST_TYPE_AIRPORT, FACILITY_LIST_REQUEST_AIRPORT_ID);

    // Repeat until requested number of idents was fetched in dispatch
    while(!facilitiesFetched)
    {
      if(verbose)
        qDebug() << Q_FUNC_INFO << "SimConnect_CallDispatch";
      api->CallDispatch(dispatchProcedure, this);
      if(hr != S_OK)
      {
        errors.append("Error in CallDispatch");
        return false;
      }
    }

    // Not used - limited by reality bubble in SimConnect
    // hr = api->RequestFacilitiesList(SIMCONNECT_FACILITY_LIST_TYPE_WAYPOINT, FACILITY_LIST_REQUEST_WAYPOINT_ID);
    // facilitiesFetched = false;
    // while(!facilitiesFetched)
    // {
    // if(verbose)
    // qDebug() << Q_FUNC_INFO << "SimConnect_CallDispatch";
    // api->CallDispatch(dispatchProcedure, this);
    // }

    // Filter airport idents if requested =============================
    if(!airportIcaoFiltersInc.isEmpty())
    {
      airportIdents.erase(std::remove_if(airportIdents.begin(), airportIdents.end(), [ = ](const QString& icao) -> bool {
                bool foundMatch = false;
                for(const QRegExp& regexp : qAsConst(airportIcaoFiltersInc))
                {
                  if(regexp.exactMatch(icao))
                  {
                    foundMatch = true;
                    break;
                  }
                }
                return !foundMatch;
              }), airportIdents.end());
    }

    airportIdents.sort();

    qDebug() << Q_FUNC_INFO << "Fetched" << airportIdents.size() << "airports";

    hr = api->Close();
    if(hr != S_OK)
    {
      errors.append("Error closing");
      return false;
    }
  }
  else
    errors.append("Error opening SimConnect.");

  if(hr != S_OK)
    return false;
  return true;
}

bool SimConnectLoaderPrivate::loadAirports()
{
  // return true;

  HRESULT hr = api->Open(QCoreApplication::applicationName().toLatin1().constData(), nullptr, 0, nullptr, 0);
  aborted = false;
  if(hr == S_OK)
  {
    writer->initQueries();
    addFacilityDefinitions();
    if(!errors.isEmpty())
      return false;

    int requested = 0;
    int size = airportIdentDetail.size();
    for(int i = 0; i < size && !aborted; i++)
    {
      // Request a list of airport details (SIMCONNECT_RECV_ID_FACILITY_DATA -> SIMCONNECT_FACILITY_DATA_AIRPORT ...)
      hr = api->RequestFacilityData(FACILITY_DATA_AIRPORT_DEFINITION_ID, FACILITY_DATA_REQUEST_ID + static_cast<unsigned int>(i),
                                    airportIdentDetail.at(i).toLatin1().constData());
      if(hr != S_OK)
      {
        errors.append("Error in RequestFacilityData");
        return false;
      }

      requested++;

      // Keep calling RequestFacilityData facility data until batch size matches or last call is done
      if(((i % batchSize) == 0 && i > 0) || i == size - 1)
      {
        // Fetch airport details now
        airportsFetchedBatch = 0;

        while(airportsFetchedBatch < requested && !aborted)
        {
          api->CallDispatch(dispatchProcedure, this);
          if(hr != S_OK)
          {
            errors.append("Error in CallDispatch");
            return false;
          }

          if(errors.size() > MAX_ERRORS)
          {
            errors.append(SimConnectLoader::tr("Too many errors reading airport data. Stopping."));
            return false;
          }

          // QThread::msleep(10);
        }
        requested = 0;

        // Write all structures and children to database
        if(!aborted)
          aborted = writeAirportBatchToDatabase();
      }
    }

    writer->deInitQueries();
    qDebug() << Q_FUNC_INFO << "Fetched" << airportsFetchedTotal << "of" << size;

    hr = api->Close();
  }
  else
    errors.append("Error opening SimConnect.");

  if(hr != S_OK)
    return false;
  return true;
}

void CALLBACK SimConnectLoaderPrivate::dispatchProcedure(SIMCONNECT_RECV *pData, DWORD cbData, void *pContext)
{
  Q_UNUSED(cbData)

  SimConnectLoaderPrivate *p = static_cast<SimConnectLoaderPrivate *>(pContext);

  if(p->verbose)
    qDebug() << Q_FUNC_INFO << "pData->dwID" << pData->dwID << "pData->dwSize" << pData->dwSize;
  switch(pData->dwID)
  {
    // List of airport idents ==============================================================
    case SIMCONNECT_RECV_ID_AIRPORT_LIST:
      {
        const SIMCONNECT_RECV_AIRPORT_LIST *pAirportList = static_cast<const SIMCONNECT_RECV_AIRPORT_LIST *>(pData);

        if(p->verbose)
          qDebug() << Q_FUNC_INFO << "pAirportList->dwArraySize" << pAirportList->dwArraySize;

        for(unsigned int i = 0; i < pAirportList->dwArraySize; i++)
          p->airportIdents.append(pAirportList->rgData[i].Ident);

        if(p->verbose)
          qDebug() << Q_FUNC_INFO << "dwOutOf" << pAirportList->dwOutOf << "dwEntryNumber" << pAirportList->dwEntryNumber
                   << pAirportList->rgData[0].Ident << "..." << pAirportList->rgData[pAirportList->dwArraySize - 1].Ident;

        if(pAirportList->dwEntryNumber >= pAirportList->dwOutOf - 1)
          p->facilitiesFetched = true;
      }
      break;

    // List of waypoint idents ==============================================================
    // Not used due to reality bubble limitation
    case SIMCONNECT_RECV_ID_WAYPOINT_LIST:
      {
        const SIMCONNECT_RECV_WAYPOINT_LIST *pWaypointList = static_cast<const SIMCONNECT_RECV_WAYPOINT_LIST *>(pData);

        if(p->verbose)
          qDebug() << Q_FUNC_INFO << "pWaypointList->dwArraySize" << pWaypointList->dwArraySize;

        for(unsigned int i = 0; i < pWaypointList->dwArraySize; i++)
          p->waypointIdents.append(pWaypointList->rgData[i]);

        if(p->verbose)
          qDebug() << Q_FUNC_INFO << "dwOutOf" << pWaypointList->dwOutOf << "dwEntryNumber" << pWaypointList->dwEntryNumber
                   << pWaypointList->rgData[0].Ident << "..." << pWaypointList->rgData[pWaypointList->dwArraySize - 1].Ident;

        if(pWaypointList->dwEntryNumber >= pWaypointList->dwOutOf - 1)
          p->facilitiesFetched = true;
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
        const SIMCONNECT_RECV_FACILITY_DATA *pFacilityData = static_cast<const SIMCONNECT_RECV_FACILITY_DATA *>(pData);
        const SIMCONNECT_FACILITY_DATA_TYPE facilityType = static_cast<SIMCONNECT_FACILITY_DATA_TYPE>(pFacilityData->Type);

        if(p->verbose)
          qDebug() << Q_FUNC_INFO << "pFacilityData->UserRequestId" << pFacilityData->UserRequestId
                   << "pFacilityData->UniqueRequestId" << pFacilityData->UniqueRequestId
                   << "pFacilityData->ParentUniqueRequestId" << pFacilityData->ParentUniqueRequestId
                   << "pFacilityData->Type" << pFacilityData->Type
                   << "pFacilityData->IsListItem" << pFacilityData->IsListItem
                   << "pFacilityData->ItemIndex" << pFacilityData->ItemIndex
                   << "pFacilityData->ListSize" << pFacilityData->ListSize;

        if(facilityType == SIMCONNECT_FACILITY_DATA_AIRPORT)
        {
          // Airport top level==============================================================
          const AirportFacility *airportFacility = reinterpret_cast<const AirportFacility *>(&pFacilityData->Data);
          p->airportMap.insert(pFacilityData->UserRequestId, Airport(*airportFacility));
          p->legsParentType = p->legsParentType2 = facilityType;

          if(p->verbose)
            qDebug() << Q_FUNC_INFO << "SIMCONNECT_FACILITY_DATA_AIRPORT" << pFacilityData->UserRequestId << airportFacility->icao
                     << airportFacility->name;
        }
        else
        {
          Airport *airport = nullptr;
          if(p->airportMap.contains(pFacilityData->UserRequestId))
            airport = &p->airportMap[pFacilityData->UserRequestId];

          if(airport != nullptr)
          {
            switch(facilityType)
            {
              // Runway ================================================================================
              case SIMCONNECT_FACILITY_DATA_RUNWAY:
                {
                  const RunwayFacility *runway = reinterpret_cast<const RunwayFacility *>(&pFacilityData->Data);
                  airport->runways.append(Runway(*runway));

                  if(p->verbose)
                    qDebug() << Q_FUNC_INFO << "SIMCONNECT_FACILITY_DATA_RUNWAY"
                             << runway->heading << runway->length << runway->width
                             << runway->primaryNumber << runway->primaryDesignator
                             << runway->secondaryNumber << runway->secondaryDesignator;
                }
                break;

              // Primary and secondary Threshold, blastpad or overrun ==============================================================
              case SIMCONNECT_FACILITY_DATA_PAVEMENT:
                {
                  const PavementFacility *pavement = reinterpret_cast<const PavementFacility *>(&pFacilityData->Data);

                  if(!airport->runways.isEmpty())
                    airport->runways.last().pavements.append(*pavement);
                  else
                    qWarning() << Q_FUNC_INFO << "Runways for pavement empty" << pFacilityData->UserRequestId;

                  if(p->verbose)
                    qDebug() << Q_FUNC_INFO << "SIMCONNECT_FACILITY_DATA_PAVEMENT" << pavement->length;
                }
                break;

              // Primary and secondary approach lights ================================================================================
              case SIMCONNECT_FACILITY_DATA_APPROACH_LIGHTS:
                {
                  const ApproachLightFacility *lights = reinterpret_cast<const ApproachLightFacility *>(&pFacilityData->Data);

                  if(!airport->runways.isEmpty())
                    airport->runways.last().approachLights.append(*lights);
                  else
                    qWarning() << Q_FUNC_INFO << "Runways for approach lights empty" << pFacilityData->UserRequestId;

                  if(p->verbose)
                    qDebug() << Q_FUNC_INFO << "SIMCONNECT_FACILITY_DATA_APPROACH_LIGHTS" << lights->system;
                }
                break;

              // Left/right primary and secondary VASI ==============================================================================
              case SIMCONNECT_FACILITY_DATA_VASI:
                {
                  const VasiFacility *vasi = reinterpret_cast<const VasiFacility *>(&pFacilityData->Data);

                  if(!airport->runways.isEmpty())
                    airport->runways.last().vasi.append(*vasi);
                  else
                    qWarning() << Q_FUNC_INFO << "Runways for VASI empty" << pFacilityData->UserRequestId;

                  if(p->verbose)
                    qDebug() << Q_FUNC_INFO << "SIMCONNECT_FACILITY_DATA_VASI" << vasi->type << vasi->angle;
                }
                break;

              // Start positions ================================================================================
              case SIMCONNECT_FACILITY_DATA_START:
                {
                  const StartFacility *start = reinterpret_cast<const StartFacility *>(&pFacilityData->Data);
                  airport->starts.append(*start);

                  if(p->verbose)
                    qDebug() << Q_FUNC_INFO << "SIMCONNECT_FACILITY_DATA_START" << start->number << start->designator << start->type;
                }
                break;

              // COM ================================================================================
              case SIMCONNECT_FACILITY_DATA_FREQUENCY:
                {
                  const FrequencyFacility *frequency = reinterpret_cast<const FrequencyFacility *>(&pFacilityData->Data);
                  airport->frequencies.append(*frequency);

                  if(p->verbose)
                    qDebug() << Q_FUNC_INFO << "SIMCONNECT_FACILITY_DATA_FREQUENCY" << frequency->name;
                }
                break;

              // Helipad ================================================================================
              case SIMCONNECT_FACILITY_DATA_HELIPAD:
                {
                  const HelipadFacility *helipad = reinterpret_cast<const HelipadFacility *>(&pFacilityData->Data);
                  airport->helipads.append(*helipad);

                  if(p->verbose)
                    qDebug() << Q_FUNC_INFO << "SIMCONNECT_FACILITY_DATA_HELIPAD" << helipad->heading << helipad->length
                             << helipad->width;
                }
                break;

              // Approach ================================================================================
              case SIMCONNECT_FACILITY_DATA_APPROACH:
                {
                  const ApproachFacility *approach = reinterpret_cast<const ApproachFacility *>(&pFacilityData->Data);
                  airport->approaches.append(Approach(*approach));

                  if(p->verbose)
                    qDebug() << Q_FUNC_INFO << "SIMCONNECT_FACILITY_DATA_APPROACH" << approach->type;
                }
                break;

              // ================================================================================
              case SIMCONNECT_FACILITY_DATA_FINAL_APPROACH_LEG:
                {
                  const LegFacility *leg = reinterpret_cast<const LegFacility *>(&pFacilityData->Data);

                  if(!airport->approaches.isEmpty())
                    airport->approaches.last().finalApproachLegs.append(*leg);
                  else
                    qWarning() << Q_FUNC_INFO << "Approaches for final legs empty" << pFacilityData->UserRequestId;

                  if(p->verbose)
                    qDebug() << Q_FUNC_INFO << "SIMCONNECT_FACILITY_DATA_FINAL_APPROACH_LEG" << leg->type << leg->fixIcao;
                }
                break;

              // ================================================================================
              case SIMCONNECT_FACILITY_DATA_MISSED_APPROACH_LEG:
                {
                  const LegFacility *leg = reinterpret_cast<const LegFacility *>(&pFacilityData->Data);

                  if(!airport->approaches.isEmpty())
                    airport->approaches.last().missedApproachLegs.append(*leg);
                  else
                    qWarning() << Q_FUNC_INFO << "Approaches for missed legs empty" << pFacilityData->UserRequestId;

                  if(p->verbose)
                    qDebug() << Q_FUNC_INFO << "SIMCONNECT_FACILITY_DATA_MISSED_APPROACH_LEG" << leg->type << leg->fixIcao;
                }
                break;

              // ================================================================================
              case SIMCONNECT_FACILITY_DATA_APPROACH_TRANSITION:
                {
                  p->legsParentType = facilityType;
                  const ApproachTransitionFacility *transition = reinterpret_cast<const ApproachTransitionFacility *>(&pFacilityData->Data);
                  if(!airport->approaches.isEmpty())
                    airport->approaches.last().approachTransitions.append(ApproachTransition(*transition));
                  else
                    qWarning() << Q_FUNC_INFO << "Approaches for transitions empty" << pFacilityData->UserRequestId;

                  if(p->verbose)
                    qDebug() << Q_FUNC_INFO << "SIMCONNECT_FACILITY_DATA_APPROACH_TRANSITION" << transition->name;
                }
                break;

              // Procedure legs ================================================================================
              case SIMCONNECT_FACILITY_DATA_APPROACH_LEG:
                {
                  const LegFacility *leg = reinterpret_cast<const LegFacility *>(&pFacilityData->Data);

                  if(p->legsParentType == SIMCONNECT_FACILITY_DATA_APPROACH_TRANSITION)
                  {
                    if(!airport->approaches.isEmpty() && !airport->approaches.last().approachTransitions.isEmpty())
                      airport->approaches.last().approachTransitions.last().transitionLegs.append(*leg);
                    else
                      qWarning() << Q_FUNC_INFO << "Transitions for legs empty" << pFacilityData->UserRequestId;
                  }
                  else if(p->legsParentType == SIMCONNECT_FACILITY_DATA_ARRIVAL)
                  {
                    // STAR legs =======================================================================
                    if(!airport->arrivals.isEmpty())
                    {
                      if(p->legsParentType2 == SIMCONNECT_FACILITY_DATA_RUNWAY_TRANSITION)
                      {
                        if(!airport->arrivals.last().runwayTransitions.isEmpty())
                          airport->arrivals.last().runwayTransitions.last().legs.append(*leg);
                        else
                          qWarning() << Q_FUNC_INFO << "Arrival for runway trans legs empty" << pFacilityData->UserRequestId;
                      }
                      else if(p->legsParentType2 == SIMCONNECT_FACILITY_DATA_ENROUTE_TRANSITION)
                      {
                        if(!airport->arrivals.last().enrouteTransitions.isEmpty())
                          airport->arrivals.last().enrouteTransitions.last().legs.append(*leg);
                        else
                          qWarning() << Q_FUNC_INFO << "Arrival for enroute trans legs empty" << pFacilityData->UserRequestId;
                      }
                      else
                        airport->arrivals.last().approachLegs.append(*leg);
                    }
                    else
                      qWarning() << Q_FUNC_INFO << "Arrivals empty" << pFacilityData->UserRequestId;
                  }
                  else if(p->legsParentType == SIMCONNECT_FACILITY_DATA_DEPARTURE)
                  {
                    // SID legs =======================================================================
                    if(!airport->departures.isEmpty())
                    {
                      if(p->legsParentType2 == SIMCONNECT_FACILITY_DATA_RUNWAY_TRANSITION)
                      {
                        if(!airport->departures.last().runwayTransitions.isEmpty())
                          airport->departures.last().runwayTransitions.last().legs.append(*leg);
                        else
                          qWarning() << Q_FUNC_INFO << "Departures for runway trans legs empty" << pFacilityData->UserRequestId;
                      }
                      else if(p->legsParentType2 == SIMCONNECT_FACILITY_DATA_ENROUTE_TRANSITION)
                      {
                        if(!airport->departures.last().enrouteTransitions.isEmpty())
                          airport->departures.last().enrouteTransitions.last().legs.append(*leg);
                        else
                          qWarning() << Q_FUNC_INFO << "Departures for enroute trans legs empty" << pFacilityData->UserRequestId;
                      }
                      else
                        airport->departures.last().approachLegs.append(*leg);
                    }
                    else
                      qWarning() << Q_FUNC_INFO << "Departures empty" << pFacilityData->UserRequestId;
                  }

                  if(p->verbose)
                    qDebug() << Q_FUNC_INFO << "SIMCONNECT_FACILITY_DATA_APPROACH_LEG" << leg->type << leg->fixIcao;
                }
                break;

              // SID ================================================================================
              case SIMCONNECT_FACILITY_DATA_DEPARTURE:
                {
                  const DepartureFacility *departure = reinterpret_cast<const DepartureFacility *>(&pFacilityData->Data);
                  airport->departures.append(Departure(*departure));
                  p->legsParentType = facilityType;
                  p->legsParentType2 = SIMCONNECT_FACILITY_DATA_AIRPORT;

                  if(p->verbose)
                    qDebug() << Q_FUNC_INFO << "SIMCONNECT_FACILITY_DATA_DEPARTURE" << departure->name;
                }
                break;

              // STAR ================================================================================
              case SIMCONNECT_FACILITY_DATA_ARRIVAL:
                {
                  const ArrivalFacility *arrival = reinterpret_cast<const ArrivalFacility *>(&pFacilityData->Data);
                  airport->arrivals.append(Arrival(*arrival));
                  p->legsParentType = facilityType;
                  p->legsParentType2 = SIMCONNECT_FACILITY_DATA_AIRPORT;

                  if(p->verbose)
                    qDebug() << Q_FUNC_INFO << "SIMCONNECT_FACILITY_DATA_ARRIVAL" << arrival->name;
                }
                break;

              // SID/STAR runway transitions ================================================================================
              case SIMCONNECT_FACILITY_DATA_RUNWAY_TRANSITION:
                {
                  const RunwayTransitionFacility *transition = reinterpret_cast<const RunwayTransitionFacility *>(&pFacilityData->Data);

                  if(p->legsParentType == SIMCONNECT_FACILITY_DATA_ARRIVAL)
                  {
                    if(!airport->arrivals.isEmpty())
                      airport->arrivals.last().runwayTransitions.append(*transition);
                  }
                  else if(p->legsParentType == SIMCONNECT_FACILITY_DATA_DEPARTURE)
                  {
                    if(!airport->departures.isEmpty())
                      airport->departures.last().runwayTransitions.append(*transition);
                  }
                  else
                    qWarning() << Q_FUNC_INFO << "Wrong parent type for runway trans" << pFacilityData->UserRequestId;

                  p->legsParentType2 = facilityType;

                  if(p->verbose)
                    qDebug() << Q_FUNC_INFO << "SIMCONNECT_FACILITY_DATA_RUNWAY_TRANSITION" << transition->runwayNumber
                             << transition->runwayDesignator;
                }
                break;

              // SID/STAR transitions ================================================================================
              case SIMCONNECT_FACILITY_DATA_ENROUTE_TRANSITION:
                {
                  const EnrouteTransitionFacility *transition = reinterpret_cast<const EnrouteTransitionFacility *>(&pFacilityData->Data);

                  if(p->legsParentType == SIMCONNECT_FACILITY_DATA_ARRIVAL)
                  {
                    if(!airport->arrivals.isEmpty())
                      airport->arrivals.last().enrouteTransitions.append(*transition);
                    else
                      qWarning() << Q_FUNC_INFO << "Arrivals for enroute trans empty" << pFacilityData->UserRequestId;
                  }
                  else if(p->legsParentType == SIMCONNECT_FACILITY_DATA_DEPARTURE)
                  {
                    if(!airport->departures.isEmpty())
                      airport->departures.last().enrouteTransitions.append(*transition);
                    else
                      qWarning() << Q_FUNC_INFO << "Departures for enroute trans empty" << pFacilityData->UserRequestId;
                  }
                  else
                    qWarning() << Q_FUNC_INFO << "Wrong parent type for enroute trans" << pFacilityData->UserRequestId;

                  p->legsParentType2 = facilityType;
                  if(p->verbose)
                    qDebug() << Q_FUNC_INFO << "SIMCONNECT_FACILITY_DATA_ENROUTE_TRANSITION" << transition->name;
                }
                break;

              // ================================================================================
              // Parking spots ==================================================================
              case SIMCONNECT_FACILITY_DATA_TAXI_PARKING:
                {
                  const TaxiParkingFacility *taxiParking = reinterpret_cast<const TaxiParkingFacility *>(&pFacilityData->Data);
                  airport->taxiParkings.append(*taxiParking);

                  if(p->verbose)
                    qDebug() << Q_FUNC_INFO << "SIMCONNECT_FACILITY_DATA_TAXI_PARKING" << taxiParking->type;
                }
                break;

              // Taxi intersections ===================================================================
              case SIMCONNECT_FACILITY_DATA_TAXI_POINT:
                {
                  const TaxiPointFacility *taxiPoint = reinterpret_cast<const TaxiPointFacility *>(&pFacilityData->Data);
                  airport->taxiPoints.append(*taxiPoint);

                  if(p->verbose)
                    qDebug() << Q_FUNC_INFO << "SIMCONNECT_FACILITY_DATA_TAXI_POINT" << taxiPoint->type;
                }
                break;

              // Paths ================================================================================
              case SIMCONNECT_FACILITY_DATA_TAXI_PATH:
                {
                  const TaxiPathFacility *taxiPath = reinterpret_cast<const TaxiPathFacility *>(&pFacilityData->Data);
                  airport->taxiPaths.append(*taxiPath);

                  if(p->verbose)
                    qDebug() << Q_FUNC_INFO << "SIMCONNECT_FACILITY_DATA_TAXI_PATH" << taxiPath->type << taxiPath->start << taxiPath->end
                             << taxiPath->nameIndex;
                }
                break;

              // Names ================================================================================
              case SIMCONNECT_FACILITY_DATA_TAXI_NAME:
                {
                  const TaxiNameFacility *taxiName = reinterpret_cast<const TaxiNameFacility *>(&pFacilityData->Data);
                  airport->taxiNames.append(*taxiName);

                  if(p->verbose)
                    qDebug() << Q_FUNC_INFO << "SIMCONNECT_FACILITY_DATA_TAXI_NAME" << taxiName->name;
                }
                break;

              // Jetways ================================================================================
              case SIMCONNECT_FACILITY_DATA_JETWAY:
                {
                  const JetwayFacility *jetway = reinterpret_cast<const JetwayFacility *>(&pFacilityData->Data);
                  airport->jetways.append(*jetway);

                  if(p->verbose)
                    qDebug() << Q_FUNC_INFO << "JetwayFacility" << jetway->parkingGate
                             << jetway->parkingSuffix << jetway->parkingSpot;
                }
                break;

              case SIMCONNECT_FACILITY_DATA_AIRPORT:
              case SIMCONNECT_FACILITY_DATA_VOR:
              case SIMCONNECT_FACILITY_DATA_NDB:
              case SIMCONNECT_FACILITY_DATA_WAYPOINT:
              case SIMCONNECT_FACILITY_DATA_ROUTE:
              case SIMCONNECT_FACILITY_DATA_VDGS:
              case SIMCONNECT_FACILITY_DATA_HOLDING_PATTERN:
                break;
            } // switch(facilityType)
          } // if(airport != nullptr)
          else
            qWarning() << Q_FUNC_INFO << "Airport for" << pFacilityData->UserRequestId << "not found";
        } // if(facilityType == SIMCONNECT_FACILITY_DATA_AIRPORT) ... else
      }
      break;

    // End of facility data =================================================================================
    case SIMCONNECT_RECV_ID_FACILITY_DATA_END:
      {
        const SIMCONNECT_RECV_FACILITY_DATA_END *pFacilityData = reinterpret_cast<const SIMCONNECT_RECV_FACILITY_DATA_END *>(pData);
        if(p->verbose)
          qDebug() << Q_FUNC_INFO << "SIMCONNECT_RECV_ID_FACILITY_DATA_END" << pFacilityData->dwID << pFacilityData->RequestId;
        p->airportsFetchedBatch++;
        p->airportsFetchedTotal++;
      }
      break;

    case SIMCONNECT_RECV_ID_EXCEPTION:
      {
        const SIMCONNECT_RECV_EXCEPTION *pException = reinterpret_cast<const SIMCONNECT_RECV_EXCEPTION *>(pData);
        qWarning() << Q_FUNC_INFO << "dwException " << pException->dwException << "dwSendID" << pException->dwSendID << "dwIndex"
                   << pException->dwIndex;
        p->errors.append(QString("Simconnect exception %1").arg(pException->dwException));
      }
      break;
  } // switch(pData->dwID)
}

bool SimConnectLoaderPrivate::writeAirportBatchToDatabase()
{
  qDebug() << Q_FUNC_INFO << "Before clean" << airportMap.size();

  // Cannot use erase on map - collect keys first and then delete
  QVector<unsigned long> dummyAirportIds;
  for(auto it = airportMap.constBegin(); it != airportMap.constEnd(); ++it)
  {
    if(it.value().isPoiDummy())
      dummyAirportIds.append(it.key());
  }

  for(unsigned long id : dummyAirportIds)
    airportMap.remove(id);

  qDebug() << Q_FUNC_INFO << "After clean" << airportMap.size();

  if(progressCallback)
    aborted = progressCallback(airportMap.first().getAirportFacility().icao, airportMap.last().getAirportFacility().icao, airportMap.size(),
                               totalWritten);

  if(!aborted)
  {
    // Write to database and commit batch ===================================================
    writer->writeAirportsToDatabase(airportMap, fileId);
    errors.append(writer->getErrors());

    totalWritten += airportMap.size();
  }

  airportMap.clear();
  return aborted;
}

#endif

// ==================================================================================================================
// ==================================================================================================================
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

bool SimConnectLoader::loadAirports(const QStringList& idents, int fileId)
{
#if !defined(SIMCONNECT_BUILD_WIN32)
  p->fileId = fileId;
  p->airportIdentDetail = idents;
  return p->loadAirports();
#else
  Q_UNUSED(idents)
  Q_UNUSED(fileId)
  return false;
#endif
}

QStringList SimConnectLoader::loadAirportIdents() const
{
#if !defined(SIMCONNECT_BUILD_WIN32)
  p->loadAirportIdents();
  return p->airportIdents;
#else
  return QStringList();
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
#else
  Q_UNUSED(callback)
#endif
}

} // namespace airport
} // namespace sc
} // namespace fs
} // namespace atools
