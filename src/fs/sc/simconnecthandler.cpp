/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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

#include "simconnecthandler.h"
#include "fs/sc/simconnectdata.h"
#include "geo/calculations.h"

#include <QDate>
#include <QTime>
#include <QDateTime>
#include <QThread>

#if defined(SIMCONNECT_DUMMY)
// Manually defined
#include "fs/sc/simconnectdummy.h"
#else
#if defined(SIMCONNECT_REAL)
// Use real SimConnect if using MSCV compilation
extern "C" {
#include <windows.h>
#include <strsafe.h>
#include "SimConnect.h"
}
#endif
// Otherwise use simulation
#endif

namespace atools {
namespace fs {
namespace sc {

enum EventIds
{
  EVENT_SIM_STATE,
  EVENT_SIM_PAUSE
};

enum DataRequestId
{
  DATA_REQUEST_ID_USER,
  DATA_REQUEST_ID_AI
};

/* Struct that will be filled with raw data from the simconnect interface. */
struct SimDataAircraft
{
  char aircraftTitle[256];
  char aircraftAtcType[32];
  char aircraftAtcModel[32];
  char aircraftAtcId[32];
  char aircraftAtcAirline[64];
  char aircraftAtcFlightNumber[32];

  char category[32]; // "Airplane", "Helicopter", "Boat", "GroundVehicle", "ControlTower", "SimpleObject", "Viewer"

  float altitudeFt;
  float latitudeDeg;
  float longitudeDeg;

  float groundVelocityKts;
  float indicatedAltitudeFt;

  float planeHeadingMagneticDeg;
  float planeHeadingTrueDeg;
  float planeTrackMagneticDeg;
  float planeTrackTrueDeg;
  qint32 isSimOnGround;

  float airspeedTrueKts;
  float airspeedIndicatedKts;
  float airspeedMach;
  float verticalSpeedFps;

  float magVarDeg;
  qint32 numEngines;
  qint32 engineType; // 0 = Piston 1 = Jet 2 = None 3 = Helo(Bell) turbine 4 = Unsupported 5 = Turboprop
};

/* Struct that will be filled with raw data from the simconnect interface. */
struct SimData
{
  SimDataAircraft aircraft;

  float planeAboveGroundFt;
  float groundAltitudeFt;

  float ambientTemperatureC;
  float totalAirTemperatureC;
  float ambientWindVelocityKts;
  float ambientWindDirectionDegT;

  qint32 ambientPrecipStateFlags;
  qint32 ambientIsInCloud;
  float ambientVisibilityMeter;
  float seaLevelPressureMbar;
  float pitotIcePercent;
  float structuralIcePercent;

  float airplaneTotalWeightLbs;
  float airplaneMaxGrossWeightLbs;
  float airplaneEmptyWeightLbs;
  float fuelTotalQuantityGallons;
  float fuelTotalWeightLbs;

  float fuelFlowPph1;
  float fuelFlowPph2;
  float fuelFlowPph3;
  float fuelFlowPph4;

  float fuelFlowGph1;
  float fuelFlowGph2;
  float fuelFlowGph3;
  float fuelFlowGph4;
  qint32 localTime;
  qint32 localYear;
  qint32 localMonth;
  qint32 localDay;
  qint32 zuluTimeSeconds;
  qint32 zuluYear;
  qint32 zuluMonth;
  qint32 zuluDay;
  qint32 timeZoneOffsetSeconds;
};

enum DataDefinitionId
{
  DATA_DEFINITION_USER,
  DATA_DEFINITION_AI
};

class SimConnectHandlerPrivate
{
public:
  SimConnectHandlerPrivate(bool verboseLogging)
    : verbose(verboseLogging)
  {

  }

#if defined(SIMCONNECT_REAL) || defined(SIMCONNECT_DUMMY)
  /* Callback receiving the data. */
  void dispatchProcedure(SIMCONNECT_RECV *pData, DWORD cbData);

  /* Static method will pass call to object which is passed in pContext. */
  static void CALLBACK dispatchCallback(SIMCONNECT_RECV *pData, DWORD cbData, void *pContext);

  void fillDataDefinitionAicraft(DataDefinitionId definitionId);
  void copyToSimData(const SimDataAircraft& simDataUserAircraft,
                     atools::fs::sc::SimConnectAircraft& airplane);

  HANDLE hSimConnect = NULL;
#endif

  SimData simData;
  QVector<SimDataAircraft> simDataAircraft;
  bool simRunning = true, simPaused = false, verbose = false, dataFetched = false, userDataFetched = false;
  sc::State state = sc::STATEOK;

};

#if defined(SIMCONNECT_REAL) || defined(SIMCONNECT_DUMMY)

void SimConnectHandlerPrivate::dispatchProcedure(SIMCONNECT_RECV *pData, DWORD cbData)
{
  Q_UNUSED(cbData);

  if(verbose)
    qDebug() << "DispatchProcedure entered";

  switch(pData->dwID)
  {
    case SIMCONNECT_RECV_ID_OPEN:
      {
        // enter code to handle SimConnect version information received in a SIMCONNECT_RECV_OPEN structure.
        SIMCONNECT_RECV_OPEN *openData = (SIMCONNECT_RECV_OPEN *)pData;

        // Print some useful simconnect interface data to log
        qInfo() << "ApplicationName" << openData->szApplicationName;
        qInfo().nospace() << "ApplicationVersion " << openData->dwApplicationVersionMajor
                          << "." << openData->dwApplicationVersionMinor;
        qInfo().nospace() << "ApplicationBuild " << openData->dwApplicationBuildMajor
                          << "." << openData->dwApplicationBuildMinor;
        qInfo().nospace() << "SimConnectVersion " << openData->dwSimConnectVersionMajor
                          << "." << openData->dwSimConnectVersionMinor;
        qInfo().nospace() << "SimConnectBuild " << openData->dwSimConnectBuildMajor
                          << "." << openData->dwSimConnectBuildMinor;
        break;
      }

    case SIMCONNECT_RECV_ID_EXCEPTION:
      {
        // enter code to handle errors received in a SIMCONNECT_RECV_EXCEPTION structure.
        SIMCONNECT_RECV_EXCEPTION *except = (SIMCONNECT_RECV_EXCEPTION *)pData;
        qWarning() << "SimConnect exception" << except->dwException
                   << "send ID" << except->dwSendID << "index" << except->dwIndex;
        state = sc::SIMCONNECT_EXCEPTION;
        break;
      }

    case SIMCONNECT_RECV_ID_EVENT:
      {
        SIMCONNECT_RECV_EVENT *evt = (SIMCONNECT_RECV_EVENT *)pData;

        switch(evt->uEventID)
        {
          case EVENT_SIM_PAUSE:
            if(verbose)
              qDebug() << "EVENT_SIM_PAUSE" << evt->dwData;
            simPaused = evt->dwData == 1;
            break;

          case EVENT_SIM_STATE:
            if(verbose)
              qDebug() << "EVENT_SIM_STATE" << evt->dwData;
            simRunning = evt->dwData == 1;
            break;
        }
        break;
      }

    case SIMCONNECT_RECV_ID_SIMOBJECT_DATA_BYTYPE:
      {
        SIMCONNECT_RECV_SIMOBJECT_DATA_BYTYPE *pObjData = (SIMCONNECT_RECV_SIMOBJECT_DATA_BYTYPE *)pData;

        if(pObjData->dwRequestID == DATA_REQUEST_ID_USER)
        {
          if(verbose)
            qDebug() << "DATA_REQUEST_ID";
          DWORD objectID = pObjData->dwObjectID;
          SimData *simDataPtr = reinterpret_cast<SimData *>(&pObjData->dwData);

          if(verbose)
            qDebug() << "ObjectID" << objectID << "Title" << simDataPtr->aircraft.aircraftTitle
                     << "atcType" << simDataPtr->aircraft.aircraftAtcType
                     << "atcModel" << simDataPtr->aircraft.aircraftAtcModel
                     << "atcId" << simDataPtr->aircraft.aircraftAtcId
                     << "atcAirline" << simDataPtr->aircraft.aircraftAtcAirline
                     << "atcFlightNumber" << simDataPtr->aircraft.aircraftAtcFlightNumber
                     << "category" << simDataPtr->aircraft.category
                     << "numEngines" << simDataPtr->aircraft.numEngines
                     << "engineType" << simDataPtr->aircraft.engineType
                     << "Lat" << simDataPtr->aircraft.latitudeDeg
                     << "Lon" << simDataPtr->aircraft.longitudeDeg
                     << "Alt" << simDataPtr->aircraft.altitudeFt
                     << "ias" << simDataPtr->aircraft.airspeedIndicatedKts
                     << "gs" << simDataPtr->aircraft.groundVelocityKts
                     << "vs" << simDataPtr->aircraft.verticalSpeedFps
                     << "course " << simDataPtr->aircraft.planeHeadingMagneticDeg
                     << "M" << simDataPtr->aircraft.planeHeadingTrueDeg << "T"
                     << "wind" << simDataPtr->ambientWindDirectionDegT
                     << "/" << simDataPtr->ambientWindVelocityKts
                     << "magvar" << simDataPtr->aircraft.magVarDeg
                     << "local time" << simDataPtr->localTime
                     << "local year" << simDataPtr->localYear
                     << "local month" << simDataPtr->localMonth
                     << "local day" << simDataPtr->localDay
                     << "zulu time" << simDataPtr->zuluTimeSeconds
                     << "zulu year" << simDataPtr->zuluYear
                     << "zulu month" << simDataPtr->zuluMonth
                     << "zulu day" << simDataPtr->zuluDay
            ;
          simData = *simDataPtr;
          dataFetched = true;
          userDataFetched = true;
        }
        else if(pObjData->dwRequestID == DATA_REQUEST_ID_AI)
        {
          if(verbose)
            qDebug() << "DATA_REQUEST_AC_ID";
          DWORD objectID = pObjData->dwObjectID;
          SimDataAircraft *simDataAircraftPtr = reinterpret_cast<SimDataAircraft *>(&pObjData->dwData);
          if(SUCCEEDED(StringCbLengthA(&simDataAircraftPtr->aircraftTitle[0],
                                       sizeof(simDataAircraftPtr->aircraftTitle),
                                       NULL))) // security check
          {
            if(verbose)
              qDebug() << "ObjectID" << objectID << "Title" << simDataAircraftPtr->aircraftTitle
                       << "atcType" << simDataAircraftPtr->aircraftAtcType
                       << "atcModel" << simDataAircraftPtr->aircraftAtcModel
                       << "atcId" << simDataAircraftPtr->aircraftAtcId
                       << "atcAirline" << simDataAircraftPtr->aircraftAtcAirline
                       << "atcFlightNumber" << simDataAircraftPtr->aircraftAtcFlightNumber
                       << "category" << simDataAircraftPtr->category
                       << "numEngines" << simDataAircraftPtr->numEngines
                       << "engineType" << simDataAircraftPtr->engineType
                       << "Lat" << simDataAircraftPtr->latitudeDeg
                       << "Lon" << simDataAircraftPtr->longitudeDeg
                       << "Alt" << simDataAircraftPtr->altitudeFt
                       << "ias" << simDataAircraftPtr->airspeedIndicatedKts
                       << "gs" << simDataAircraftPtr->groundVelocityKts
                       << "vs" << simDataAircraftPtr->verticalSpeedFps
                       << "course " << simDataAircraftPtr->planeHeadingMagneticDeg
                       << "M" << simDataAircraftPtr->planeHeadingTrueDeg << "T"
                       << "magvar" << simDataAircraftPtr->magVarDeg
              ;

            simDataAircraft.append(*simDataAircraftPtr);
            dataFetched = true;
          }
        }

        break;
      }

    case SIMCONNECT_RECV_ID_QUIT:
      if(verbose)
        qDebug() << "SIMCONNECT_RECV_ID_QUIT";
      simRunning = false;
      state = sc::DISCONNECTED;
      break;

    default:
      if(verbose)
        qDebug() << "Received" << pData->dwID;
      break;
  }
  if(verbose)
    qDebug() << "DispatchProcedure finished";
}

void CALLBACK SimConnectHandlerPrivate::dispatchCallback(SIMCONNECT_RECV *pData, DWORD cbData, void *pContext)
{
  SimConnectHandlerPrivate *handlerClass = static_cast<SimConnectHandlerPrivate *>(pContext);
  handlerClass->dispatchProcedure(pData, cbData);
}

void SimConnectHandlerPrivate::copyToSimData(const SimDataAircraft& simDataUserAircraft,
                                             SimConnectAircraft& airplane)
{
  airplane.airplaneTitle = simDataUserAircraft.aircraftTitle;
  airplane.airplaneModel = simDataUserAircraft.aircraftAtcModel;
  airplane.airplaneReg = simDataUserAircraft.aircraftAtcId;
  airplane.airplaneType = simDataUserAircraft.aircraftAtcType;
  airplane.airplaneAirline = simDataUserAircraft.aircraftAtcAirline;
  airplane.airplaneFlightnumber = simDataUserAircraft.aircraftAtcFlightNumber;

  QString cat = QString(simDataUserAircraft.category).toLower().trimmed();
  if(cat.compare("airplane"))
    airplane.category = AIRPLANE;
  else if(cat.compare("helicopter"))
    airplane.category = HELICOPTER;
  else if(cat.compare("boat"))
    airplane.category = BOAT;
  else if(cat.compare("groundvehicle"))
    airplane.category = GROUNDVEHICLE;
  else if(cat.compare("controltower"))
    airplane.category = CONTROLTOWER;
  else if(cat.compare("simpleobject"))
    airplane.category = SIMPLEOBJECT;
  else if(cat.compare("viewer"))
    airplane.category = VIEWER;

  airplane.numberOfEngines = static_cast<quint8>(simDataUserAircraft.numEngines);
  airplane.engineType = static_cast<EngineType>(simDataUserAircraft.engineType);

  airplane.position.setLonX(simDataUserAircraft.longitudeDeg);
  airplane.position.setLatY(simDataUserAircraft.latitudeDeg);
  airplane.position.setAltitude(simDataUserAircraft.altitudeFt);

  airplane.groundSpeed = simDataUserAircraft.groundVelocityKts;
  airplane.indicatedAltitude = simDataUserAircraft.indicatedAltitudeFt;
  airplane.headingMag = simDataUserAircraft.planeHeadingMagneticDeg;
  airplane.headingTrue = simDataUserAircraft.planeHeadingTrueDeg;
  airplane.trackMag = simDataUserAircraft.planeTrackMagneticDeg;
  airplane.trackTrue = simDataUserAircraft.planeTrackTrueDeg;

  airplane.trueSpeed = simDataUserAircraft.airspeedTrueKts;
  airplane.indicatedSpeed = simDataUserAircraft.airspeedIndicatedKts;
  airplane.machSpeed = simDataUserAircraft.airspeedMach;
  airplane.verticalSpeed = simDataUserAircraft.verticalSpeedFps * 60.f;

  if(simDataUserAircraft.isSimOnGround > 0)
    airplane.flags |= atools::fs::sc::ON_GROUND;

  airplane.magVar = simDataUserAircraft.magVarDeg;
}

void SimConnectHandlerPrivate::fillDataDefinitionAicraft(DataDefinitionId definitionId)
{
  // Set up the data definition, but do not yet do anything with it
  SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Title", NULL,
                                 SIMCONNECT_DATATYPE_STRING256);

  SimConnect_AddToDataDefinition(hSimConnect, definitionId, "ATC Type", NULL,
                                 SIMCONNECT_DATATYPE_STRING32);

  SimConnect_AddToDataDefinition(hSimConnect, definitionId, "ATC Model", NULL,
                                 SIMCONNECT_DATATYPE_STRING32);

  SimConnect_AddToDataDefinition(hSimConnect, definitionId, "ATC Id", NULL,
                                 SIMCONNECT_DATATYPE_STRING32);

  SimConnect_AddToDataDefinition(hSimConnect, definitionId, "ATC Airline", NULL,
                                 SIMCONNECT_DATATYPE_STRING64);

  SimConnect_AddToDataDefinition(hSimConnect, definitionId, "ATC Flight Number", NULL,
                                 SIMCONNECT_DATATYPE_STRING32);

  SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Category", NULL,
                                 SIMCONNECT_DATATYPE_STRING32);

  SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Plane Altitude", "feet",
                                 SIMCONNECT_DATATYPE_FLOAT32);
  SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Plane Latitude", "degrees",
                                 SIMCONNECT_DATATYPE_FLOAT32);
  SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Plane Longitude", "degrees",
                                 SIMCONNECT_DATATYPE_FLOAT32);

  SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Ground Velocity", "knots",
                                 SIMCONNECT_DATATYPE_FLOAT32);
  SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Indicated Altitude", "feet",
                                 SIMCONNECT_DATATYPE_FLOAT32);

  SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Plane Heading Degrees Magnetic",
                                 "degrees", SIMCONNECT_DATATYPE_FLOAT32);

  SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Plane Heading Degrees True", "degrees",
                                 SIMCONNECT_DATATYPE_FLOAT32);
  SimConnect_AddToDataDefinition(hSimConnect, definitionId, "GPS Ground Magnetic Track",
                                 "degrees", SIMCONNECT_DATATYPE_FLOAT32);
  SimConnect_AddToDataDefinition(hSimConnect, definitionId, "GPS Ground True Track", "degrees",
                                 SIMCONNECT_DATATYPE_FLOAT32);

  SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Sim On Ground", "bool",
                                 SIMCONNECT_DATATYPE_INT32);

  SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Airspeed True", "knots",
                                 SIMCONNECT_DATATYPE_FLOAT32);
  SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Airspeed Indicated", "knots",
                                 SIMCONNECT_DATATYPE_FLOAT32);
  SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Airspeed Mach", "mach",
                                 SIMCONNECT_DATATYPE_FLOAT32);
  SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Vertical Speed", "feet per second",
                                 SIMCONNECT_DATATYPE_FLOAT32);

  SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Magvar",
                                 "degrees", SIMCONNECT_DATATYPE_FLOAT32);

  SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Number of Engines", "number",
                                 SIMCONNECT_DATATYPE_INT32);

  SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Engine Type", "number",
                                 SIMCONNECT_DATATYPE_INT32);
}

#endif

// ===============================================================================================
// SimConnectHandler
// ===============================================================================================

SimConnectHandler::SimConnectHandler(bool verboseLogging)
  : p(new SimConnectHandlerPrivate(verboseLogging))
{

}

SimConnectHandler::~SimConnectHandler()
{
#if defined(SIMCONNECT_REAL) || defined(SIMCONNECT_DUMMY)
  if(p->hSimConnect != NULL)
  {
    HRESULT hr = SimConnect_Close(p->hSimConnect);
    if(hr != S_OK)
      qWarning() << "Error closing SimConnect";
  }
#endif
  delete p;
}

bool SimConnectHandler::isSimRunning() const
{
  return p->simRunning;
}

bool SimConnectHandler::isSimPaused() const
{
  return p->simPaused;
}

State atools::fs::sc::SimConnectHandler::getState() const
{
  return p->state;
}

bool SimConnectHandler::connect()
{
#if defined(SIMCONNECT_REAL) || defined(SIMCONNECT_DUMMY)
  HRESULT hr;

  if(p->verbose)
    qDebug() << "Before open";

  hr = SimConnect_Open(&p->hSimConnect, "Little Navconnect", NULL, 0, 0, 0);
  if(hr == S_OK)
  {
    if(p->verbose)
      qDebug() << "Connected to Flight Simulator";

    p->fillDataDefinitionAicraft(DATA_DEFINITION_AI);

    p->fillDataDefinitionAicraft(DATA_DEFINITION_USER);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Plane Alt Above Ground", "feet",
                                   SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Ground Altitude", "feet",
                                   SIMCONNECT_DATATYPE_FLOAT32);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Ambient Temperature", "celsius",
                                   SIMCONNECT_DATATYPE_FLOAT32);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Total Air Temperature", "celsius",
                                   SIMCONNECT_DATATYPE_FLOAT32);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Ambient Wind Velocity", "knots",
                                   SIMCONNECT_DATATYPE_FLOAT32);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Ambient Wind Direction", "degrees",
                                   SIMCONNECT_DATATYPE_FLOAT32);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Ambient Precip State", "mask",
                                   SIMCONNECT_DATATYPE_INT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Ambient In Cloud", "bool",
                                   SIMCONNECT_DATATYPE_INT32);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Ambient Visibility", "meters",
                                   SIMCONNECT_DATATYPE_FLOAT32);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Sea Level Pressure", "millibars",
                                   SIMCONNECT_DATATYPE_FLOAT32);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Pitot Ice Pct", "percent",
                                   SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Structural Ice Pct", "percent",
                                   SIMCONNECT_DATATYPE_FLOAT32);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Total Weight", "pounds",
                                   SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Max Gross Weight", "pounds",
                                   SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Empty Weight", "pounds",
                                   SIMCONNECT_DATATYPE_FLOAT32);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Fuel Total Quantity", "gallons",
                                   SIMCONNECT_DATATYPE_FLOAT32);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Fuel Total Quantity Weight",
                                   "pounds", SIMCONNECT_DATATYPE_FLOAT32);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Eng Fuel Flow PPH:1",
                                   "Pounds per hour", SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Eng Fuel Flow PPH:2",
                                   "Pounds per hour", SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Eng Fuel Flow PPH:3",
                                   "Pounds per hour", SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Eng Fuel Flow PPH:4",
                                   "Pounds per hour", SIMCONNECT_DATATYPE_FLOAT32);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Eng Fuel Flow GPH:1",
                                   "Gallons per hour", SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Eng Fuel Flow GPH:2",
                                   "Gallons per hour", SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Eng Fuel Flow GPH:3",
                                   "Gallons per hour", SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Eng Fuel Flow GPH:4",
                                   "Gallons per hour", SIMCONNECT_DATATYPE_FLOAT32);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Local Time",
                                   "seconds", SIMCONNECT_DATATYPE_INT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Local Year",
                                   "number", SIMCONNECT_DATATYPE_INT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Local Month of Year",
                                   "number", SIMCONNECT_DATATYPE_INT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Local Day of Month",
                                   "number", SIMCONNECT_DATATYPE_INT32);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Zulu Time",
                                   "seconds", SIMCONNECT_DATATYPE_INT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Zulu Year",
                                   "number", SIMCONNECT_DATATYPE_INT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Zulu Month of Year",
                                   "number", SIMCONNECT_DATATYPE_INT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Zulu Day of Month",
                                   "number", SIMCONNECT_DATATYPE_INT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER, "Time Zone Offset",
                                   "seconds", SIMCONNECT_DATATYPE_INT32);

    // Request an event when the simulation starts or pauses
    SimConnect_SubscribeToSystemEvent(p->hSimConnect, EVENT_SIM_STATE, "Sim");
    SimConnect_SubscribeToSystemEvent(p->hSimConnect, EVENT_SIM_PAUSE, "Pause");

    p->state = sc::STATEOK;

    return true;
  }
  else
  {
    qWarning() << "SimConnect_Open: Error";
    p->state = sc::OPEN_ERROR;
    p->hSimConnect = NULL;
    return false;
  }
#endif

  return true;
}

bool SimConnectHandler::fetchData(atools::fs::sc::SimConnectData& data, int radiusKm)
{
#if defined(SIMCONNECT_REAL) || defined(SIMCONNECT_DUMMY)
  if(p->verbose)
    qDebug() << "fetchData entered ================================================================";

  // ==========================================================
  if(p->verbose)
    qDebug() << "fetchData AI aircraft details";

  HRESULT hr = SimConnect_RequestDataOnSimObjectType(p->hSimConnect, DATA_REQUEST_ID_AI, DATA_DEFINITION_AI,
                                                     static_cast<DWORD>(radiusKm) * 1000,
                                                     SIMCONNECT_SIMOBJECT_TYPE_AIRCRAFT);

  if(hr != S_OK)
  {
    qWarning() << "SimConnect_RequestDataOnSimObjectType SIMCONNECT_SIMOBJECT_TYPE_AIRCRAFT: Error";
    p->state = sc::FETCH_ERROR;
    return false;
  }

  hr = SimConnect_RequestDataOnSimObjectType(p->hSimConnect, DATA_REQUEST_ID_USER, DATA_DEFINITION_USER, 0,
                                             SIMCONNECT_SIMOBJECT_TYPE_USER);

  if(hr != S_OK)
  {
    qWarning() << "SimConnect_RequestDataOnSimObjectType SIMCONNECT_SIMOBJECT_TYPE_USER: Error";
    p->state = sc::FETCH_ERROR;
    return false;
  }

  p->userDataFetched = false;
  p->dataFetched = true;
  p->simDataAircraft.clear();
  int dispatchCycles = 0;
  while(p->dataFetched)
  {
    p->dataFetched = false;
    hr = SimConnect_CallDispatch(p->hSimConnect, p->dispatchCallback, p);
    if(hr != S_OK)
    {
      qWarning() << "SimConnect_CallDispatch: Error";
      p->state = sc::FETCH_ERROR;
      return false;
    }

    QThread::msleep(10);
    dispatchCycles++;
  }

  p->state = sc::STATEOK;

  if(p->verbose)
  {
    if(dispatchCycles > 1)
      qDebug() << "dispatchCycles > 1" << dispatchCycles;
    qDebug() << "numDataFetchedAi" << p->simDataAircraft.size();
  }

  for(const SimDataAircraft& simDataAi : p->simDataAircraft)
  {
    atools::fs::sc::SimConnectAircraft ap;
    p->copyToSimData(simDataAi, ap);
    data.aiAircraft.append(ap);
  }

  if(p->userDataFetched)
  {
    p->copyToSimData(p->simData.aircraft, data.userAircraft);
    data.userAircraft.groundAltitude = p->simData.groundAltitudeFt;
    data.userAircraft.altitudeAboveGround = p->simData.planeAboveGroundFt;

    if(p->simData.ambientPrecipStateFlags & 4)
      data.userAircraft.flags |= atools::fs::sc::IN_RAIN;
    if(p->simData.ambientPrecipStateFlags & 8)
      data.userAircraft.flags |= atools::fs::sc::IN_SNOW;

    if(p->simData.ambientIsInCloud > 0)
      data.userAircraft.flags |= atools::fs::sc::IN_CLOUD;

    data.userAircraft.ambientTemperature = p->simData.ambientTemperatureC;
    data.userAircraft.totalAirTemperature = p->simData.totalAirTemperatureC;
    data.userAircraft.ambientVisibility = p->simData.ambientVisibilityMeter;

    data.userAircraft.seaLevelPressure = p->simData.seaLevelPressureMbar;
    data.userAircraft.pitotIce = p->simData.pitotIcePercent;
    data.userAircraft.structuralIce = p->simData.structuralIcePercent;
    data.userAircraft.airplaneTotalWeight = p->simData.airplaneTotalWeightLbs;
    data.userAircraft.airplaneMaxGrossWeight = p->simData.airplaneMaxGrossWeightLbs;
    data.userAircraft.airplaneEmptyWeight = p->simData.airplaneEmptyWeightLbs;
    data.userAircraft.fuelTotalQuantity = p->simData.fuelTotalQuantityGallons;
    data.userAircraft.fuelTotalWeight = p->simData.fuelTotalWeightLbs;

    // Summarize fuel flow for all engines
    data.userAircraft.fuelFlowPPH =
      p->simData.fuelFlowPph1 + p->simData.fuelFlowPph2 + p->simData.fuelFlowPph3 + p->simData.fuelFlowPph4;

    data.userAircraft.fuelFlowGPH =
      p->simData.fuelFlowGph1 + p->simData.fuelFlowGph2 + p->simData.fuelFlowGph3 + p->simData.fuelFlowGph4;

    data.userAircraft.windDirection = p->simData.ambientWindDirectionDegT;
    data.userAircraft.windSpeed = p->simData.ambientWindVelocityKts;

    // Build local time and use timezone offset from simulator
    QDate localDate(p->simData.localYear, p->simData.localMonth, p->simData.localDay);
    QTime localTime = QTime::fromMSecsSinceStartOfDay(p->simData.localTime * 1000);
    QDateTime localDateTime(localDate, localTime, Qt::OffsetFromUTC, p->simData.timeZoneOffsetSeconds);
    data.userAircraft.localDateTime = localDateTime;

    QDate zuluDate(p->simData.zuluYear, p->simData.zuluMonth, p->simData.zuluDay);
    QTime zuluTime = QTime::fromMSecsSinceStartOfDay(p->simData.zuluTimeSeconds * 1000);
    QDateTime zuluDateTime(zuluDate, zuluTime, Qt::UTC);
    data.userAircraft.zuluDateTime = zuluDateTime;
  }

  if(!p->simRunning || p->simPaused || !p->userDataFetched)
  {
    if(p->verbose)
      qDebug() << "Running" << p->simRunning << "paused" << p->simPaused
               << "dataFetched" << p->dataFetched;
    return false;
  }
  return true;

#else
  Q_UNUSED(radiusKm);

  // Simple aircraft simulation ------------------------------------------------
  static qint64 lastUpdate = QDateTime::currentMSecsSinceEpoch();
  int updateRate = static_cast<int>(QDateTime::currentMSecsSinceEpoch() - lastUpdate);
  lastUpdate = QDateTime::currentMSecsSinceEpoch();
  static int dataId = 0;
  static int updatesMs = 0;
  static atools::geo::Pos curPos(8.34239197, 54.9116364);
  // 200 kts: 0.0555 nm per second / 0.0277777 nm per cycle - only for 500 ms updates
  float speed = 200.f;
  float nmPerSec = speed / 3600.f;
  static float course = 45.f;
  static float courseChange = 0.f;
  static float fuelFlow = 100.f;
  static float visibility = 0.1f;

  static float alt = 0.f, altChange = 0.f;

  updatesMs += updateRate;

  if((updatesMs % 40000) == 0)
    courseChange = 0.f;
  else if((updatesMs % 30000) == 0)
  {
    courseChange = updateRate / 1000.f * 2.f; // 2 deg per second
    if(course > 180.f)
      courseChange = -courseChange;
  }
  course += courseChange;
  course = atools::geo::normalizeCourse(course);

  // Simulate takeoff run
  if(updatesMs <= 10000)
  {
    data.userAircraft.flags |= atools::fs::sc::ON_GROUND;
    fuelFlow = 200.f;
  }

  // Simulate takeoff
  if(updatesMs == 10000)
  {
    altChange = updateRate / 1000.f * 16.6f; // 1000 ft per min
    data.userAircraft.flags &= ~atools::fs::sc::ON_GROUND;
    fuelFlow = 150.f;
  }

  if((updatesMs % 120000) == 0)
  {
    altChange = 0.f;
    fuelFlow = 100.f;
  }
  else if((updatesMs % 60000) == 0)
  {
    altChange = updateRate / 1000.f * 16.6f; // 1000 ft per min
    fuelFlow = 150.f;
    if(alt > 8000.f)
    {
      altChange = -altChange / 2.f;
      fuelFlow = 50.f;
    }
  }
  alt += altChange;

  if(updatesMs == 20000)
    data.userAircraft.flags |= atools::fs::sc::IN_SNOW | atools::fs::sc::IN_CLOUD | atools::fs::sc::IN_RAIN;
  else if(updatesMs == 10000)
    data.userAircraft.flags &= ~(atools::fs::sc::IN_SNOW | atools::fs::sc::IN_CLOUD | atools::fs::sc::IN_RAIN);

  atools::geo::Pos next =
    curPos.endpoint(atools::geo::nmToMeter(updateRate / 1000.f * nmPerSec), course).normalize();

  QString dataIdStr = QString::number(dataId);
  data.userAircraft.airplaneTitle = "Airplane Title " + dataIdStr;
  data.userAircraft.airplaneModel = "Duke";
  data.userAircraft.airplaneReg = "D-REGI";
  data.userAircraft.airplaneType = "Beech";
  data.userAircraft.airplaneAirline = "Airline";
  data.userAircraft.airplaneFlightnumber = "965";
  data.userAircraft.fuelFlowPPH = fuelFlow;
  data.userAircraft.fuelFlowGPH = fuelFlow / 6.f;
  data.userAircraft.ambientVisibility = visibility;
  visibility += 1.f;

  data.userAircraft.position = next;
  data.userAircraft.getPosition().setAltitude(alt);
  data.userAircraft.verticalSpeed = altChange * 60.f;

  data.userAircraft.headingMag = course;
  data.userAircraft.headingTrue = course + 1.f;

  data.userAircraft.groundSpeed = 200.f;
  data.userAircraft.indicatedSpeed = 150.f;
  data.userAircraft.trueSpeed = 170.f;
  data.userAircraft.windDirection = 180.f;
  data.userAircraft.windSpeed = 25.f;
  data.userAircraft.seaLevelPressure = 1013.f;

  data.userAircraft.ambientTemperature = 10.f;
  data.userAircraft.totalAirTemperature = 20.f;
  data.userAircraft.fuelTotalQuantity = 1000.f / 6.f;
  data.userAircraft.fuelTotalWeight = 1000.f;

  data.userAircraft.localDateTime = QDateTime::currentDateTime();

  QDate zuluDate(QDate::currentDate().year(), QDate::currentDate().month(), QDate::currentDate().day());
  QTime zuluTime = QTime::fromMSecsSinceStartOfDay(QTime::currentTime().msecsSinceStartOfDay());
  QDateTime zuluDateTime(zuluDate, zuluTime, Qt::UTC);
  data.userAircraft.zuluDateTime = zuluDateTime;

  dataId++;

  static const int num = 10000;
  static bool init = false;
  static float x[num];
  static float y[num];

  if(!init)
  {
    init = true;
    for(int i = 0; i < num; i++)
    {
      x[i] = static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX) / 10.f) - 5.f;
      y[i] = static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX) / 10.f) - 5.f;
    }
  }

  for(int i = 0; i < num; i++)
  {
    SimConnectAircraft ap(data.userAircraft);
    ap.airplaneTitle = "AI" + QString::number(i) + " " + ap.airplaneTitle;
    ap.position.setLonX(ap.position.getLonX() + x[i]);
    ap.position.setLatY(ap.position.getLatY() + y[i]);

    data.aiAircraft.append(ap);
  }

  curPos = next;
  return true;

#endif
}

} // namespace sc
} // namespace fs
} // namespace atools
