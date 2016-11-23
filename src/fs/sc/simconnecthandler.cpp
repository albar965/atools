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
#include "fs/sc/weatherrequest.h"
#include "fs/sc/simconnectdata.h"
#include "geo/calculations.h"

#include <QDate>
#include <QTime>
#include <QDateTime>
#include <QThread>
#include <QSet>
#include <QCache>

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
  DATA_REQUEST_ID_USER_AIRCRAFT = 1,
  DATA_REQUEST_ID_AI_AIRCRAFT = 2,
  DATA_REQUEST_ID_AI_HELICOPTER = 3,
  DATA_REQUEST_ID_WEATHER_INTERPOLATED = 4,
  DATA_REQUEST_ID_WEATHER_NEAREST_STATION = 5,
  DATA_REQUEST_ID_WEATHER_STATION = 6
};

enum DataDefinitionId
{
  DATA_DEFINITION_USER_AIRCRAFT = 10,
  DATA_DEFINITION_AI_AIRCRAFT = 20,
  DATA_DEFINITION_AI_HELICOPTER = 30
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
  qint32 userSim;
  qint32 modelRadius;
  qint32 wingSpan;

  char aiFrom[32];
  char aiTo[32];

  float altitudeFt;
  float latitudeDeg;
  float longitudeDeg;

  float groundVelocityKts;
  float indicatedAltitudeFt;

  float planeHeadingMagneticDeg;
  float planeHeadingTrueDeg;
  qint32 isSimOnGround;

  float airspeedTrueKts;
  float airspeedIndicatedKts;
  float airspeedMach;
  float verticalSpeedFps;

  qint32 numEngines;
  qint32 engineType; // 0 = Piston 1 = Jet 2 = None 3 = Helo(Bell) turbine 4 = Unsupported 5 = Turboprop
};

/* Struct that will be filled with raw data from the simconnect interface. */
struct SimData
{
  SimDataAircraft aircraft;

  float magVarDeg;
  float planeTrackMagneticDeg;
  float planeTrackTrueDeg;

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

class SimConnectHandlerPrivate
{
public:
  SimConnectHandlerPrivate(bool verboseLogging)
    : verbose(verboseLogging)
  {

  }

  /* Callback receiving the data. */
  void dispatchProcedure(SIMCONNECT_RECV *pData, DWORD cbData);

  /* Static method will pass call to object which is passed in pContext. */
  static void CALLBACK dispatchCallback(SIMCONNECT_RECV *pData, DWORD cbData, void *pContext);

  void fillDataDefinitionAicraft(DataDefinitionId definitionId);
  void copyToSimData(const SimDataAircraft& simDataUserAircraft,
                     atools::fs::sc::SimConnectAircraft& airplane);

  HANDLE hSimConnect = NULL;

  SimData simData;
  unsigned long simDataObjectId;

  QVector<SimDataAircraft> simDataAircraft;
  QVector<unsigned long> simDataAircraftObjectIds;

  bool simRunning = true, simPaused = false, verbose = false,
       userDataFetched = false, aiDataFetched = false, weatherDataFetched = false;

  sc::State state = sc::STATEOK;

  atools::fs::sc::WeatherRequest weatherRequest;
  QVector<QString> fetchedMetars;
  SIMCONNECT_EXCEPTION simconnectException;

};

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
        state = sc::EXCEPTION;
        simconnectException = static_cast<SIMCONNECT_EXCEPTION>(except->dwException);
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

        if(pObjData->dwRequestID == DATA_REQUEST_ID_USER_AIRCRAFT)
        {
          if(verbose)
            qDebug() << "DATA_REQUEST_ID_USER_AIRCRAFT"
                     << "pObjData->dwDefineCount" << pObjData->dwDefineCount
                     << "pObjData->dwDefineID" << pObjData->dwDefineID
                     << "pObjData->dwID" << pObjData->dwID
                     << "pObjData->dwObjectID" << pObjData->dwObjectID
                     << "pObjData->dwRequestID" << pObjData->dwRequestID
                     << "pObjData->dwentrynumber" << pObjData->dwentrynumber
                     << "pObjData->dwoutof" << pObjData->dwoutof;

          DWORD objectID = pObjData->dwObjectID;
          SimData *simDataPtr = reinterpret_cast<SimData *>(&pObjData->dwData);

          if(verbose)
            qDebug() << "ObjectID" << objectID
                     << "Title" << simDataPtr->aircraft.aircraftTitle
                     << "atcType" << simDataPtr->aircraft.aircraftAtcType
                     << "atcModel" << simDataPtr->aircraft.aircraftAtcModel
                     << "atcId" << simDataPtr->aircraft.aircraftAtcId
                     << "atcAirline" << simDataPtr->aircraft.aircraftAtcAirline
                     << "atcFlightNumber" << simDataPtr->aircraft.aircraftAtcFlightNumber
                     << "category" << simDataPtr->aircraft.category
                     << "userSim" << simDataPtr->aircraft.userSim
                     << "modelRadius" << simDataPtr->aircraft.modelRadius
                     << "wingSpan" << simDataPtr->aircraft.wingSpan
                     << "aiFrom" << simDataPtr->aircraft.aiFrom
                     << "aiTo" << simDataPtr->aircraft.aiTo
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
                     << "track " << simDataPtr->planeTrackMagneticDeg
                     << "M" << simDataPtr->planeTrackTrueDeg << "T"
                     << "wind" << simDataPtr->ambientWindDirectionDegT
                     << "/" << simDataPtr->ambientWindVelocityKts
                     << "magvar" << simDataPtr->magVarDeg
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
          simDataObjectId = objectID;
          userDataFetched = true;
        }
        else if(pObjData->dwRequestID == DATA_REQUEST_ID_AI_AIRCRAFT ||
                pObjData->dwRequestID == DATA_REQUEST_ID_AI_HELICOPTER)
        {
          if(verbose)
            qDebug() << "DATA_REQUEST_ID_AI_AIRCRAFT DATA_REQUEST_ID_AI_HELICOPTER"
                     << "pObjData->dwDefineCount" << pObjData->dwDefineCount
                     << "pObjData->dwDefineID" << pObjData->dwDefineID
                     << "pObjData->dw ID" << pObjData->dwID
                     << "pObjData->dwObjectID" << pObjData->dwObjectID
                     << "pObjData->dwRequestID" << pObjData->dwRequestID
                     << "pObjData->dwentrynumber" << pObjData->dwentrynumber
                     << "pObjData->dwoutof" << pObjData->dwoutof;

          if(pObjData->dwObjectID > 0)
          {
            DWORD objectID = pObjData->dwObjectID;
            SimDataAircraft *simDataAircraftPtr = reinterpret_cast<SimDataAircraft *>(&pObjData->dwData);

            if(simDataAircraftPtr->userSim == 0 && // Do not add user aircraft to list
               SUCCEEDED(StringCbLengthA(&simDataAircraftPtr->aircraftTitle[0],
                                         sizeof(simDataAircraftPtr->aircraftTitle),
                                         NULL))) // security check
            {
              if(verbose)
                qDebug() << "ObjectID" << objectID
                         << "Title" << simDataAircraftPtr->aircraftTitle
                         << "atcType" << simDataAircraftPtr->aircraftAtcType
                         << "atcModel" << simDataAircraftPtr->aircraftAtcModel
                         << "atcId" << simDataAircraftPtr->aircraftAtcId
                         << "atcAirline" << simDataAircraftPtr->aircraftAtcAirline
                         << "atcFlightNumber" << simDataAircraftPtr->aircraftAtcFlightNumber
                         << "category" << simDataAircraftPtr->category
                         << "userSim" << simDataAircraftPtr->userSim
                         << "modelRadius" << simDataAircraftPtr->modelRadius
                         << "wingSpan" << simDataAircraftPtr->wingSpan
                         << "aiFrom" << simDataAircraftPtr->aiFrom
                         << "aiTo" << simDataAircraftPtr->aiTo
                         << "numEngines" << simDataAircraftPtr->numEngines
                         << "engineType" << simDataAircraftPtr->engineType
                         << "Lat" << simDataAircraftPtr->latitudeDeg
                         << "Lon" << simDataAircraftPtr->longitudeDeg
                         << "Alt" << simDataAircraftPtr->altitudeFt
                         << "ias" << simDataAircraftPtr->airspeedIndicatedKts
                         << "gs" << simDataAircraftPtr->groundVelocityKts
                         << "vs" << simDataAircraftPtr->verticalSpeedFps
                         << "course " << simDataAircraftPtr->planeHeadingMagneticDeg << "M"
                         << simDataAircraftPtr->planeHeadingTrueDeg << "T"
                ;

              simDataAircraft.append(*simDataAircraftPtr);
              simDataAircraftObjectIds.append(objectID);
              aiDataFetched = true;
            }
          }
        }

        break;
      }
    case SIMCONNECT_RECV_ID_WEATHER_OBSERVATION:
      {
        SIMCONNECT_RECV_WEATHER_OBSERVATION *pObjData = (SIMCONNECT_RECV_WEATHER_OBSERVATION *)pData;

        const char *pszMETAR = pObjData->szMetar;

        if(pObjData->dwRequestID == DATA_REQUEST_ID_WEATHER_INTERPOLATED ||
           pObjData->dwRequestID == DATA_REQUEST_ID_WEATHER_NEAREST_STATION ||
           pObjData->dwRequestID == DATA_REQUEST_ID_WEATHER_STATION)
        {
          if(verbose)
            qDebug() << "METAR" << pszMETAR;

          fetchedMetars.append(QString(pszMETAR));
          weatherDataFetched = true;
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
  airplane.fromIdent = simDataUserAircraft.aiFrom;
  airplane.toIdent = simDataUserAircraft.aiTo;

  QString cat = QString(simDataUserAircraft.category).toLower().trimmed();
  if(cat == "airplane")
    airplane.category = AIRPLANE;
  else if(cat == "helicopter")
    airplane.category = HELICOPTER;
  else if(cat == "boat")
    airplane.category = BOAT;
  else if(cat == "groundvehicle")
    airplane.category = GROUNDVEHICLE;
  else if(cat == "controltower")
    airplane.category = CONTROLTOWER;
  else if(cat == "simpleobject")
    airplane.category = SIMPLEOBJECT;
  else if(cat == "viewer")
    airplane.category = VIEWER;

  airplane.wingSpan = static_cast<quint16>(simDataUserAircraft.wingSpan);
  airplane.modelRadius = static_cast<quint16>(simDataUserAircraft.modelRadius);

  airplane.numberOfEngines = static_cast<quint8>(simDataUserAircraft.numEngines);
  airplane.engineType = static_cast<EngineType>(simDataUserAircraft.engineType);

  airplane.position.setLonX(simDataUserAircraft.longitudeDeg);
  airplane.position.setLatY(simDataUserAircraft.latitudeDeg);
  airplane.position.setAltitude(simDataUserAircraft.altitudeFt);

  airplane.groundSpeed = simDataUserAircraft.groundVelocityKts;
  airplane.indicatedAltitude = simDataUserAircraft.indicatedAltitudeFt;
  airplane.headingMag = simDataUserAircraft.planeHeadingMagneticDeg;
  airplane.headingTrue = simDataUserAircraft.planeHeadingTrueDeg;

  airplane.trueSpeed = simDataUserAircraft.airspeedTrueKts;
  airplane.indicatedSpeed = simDataUserAircraft.airspeedIndicatedKts;
  airplane.machSpeed = simDataUserAircraft.airspeedMach;
  airplane.verticalSpeed = simDataUserAircraft.verticalSpeedFps * 60.f;

  if(simDataUserAircraft.isSimOnGround > 0)
    airplane.flags |= atools::fs::sc::ON_GROUND;
  if(simDataUserAircraft.userSim > 0)
    airplane.flags |= atools::fs::sc::IS_USER;
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

  SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Is User Sim", "bool",
                                 SIMCONNECT_DATATYPE_INT32);

  SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Visual Model Radius", "feet",
                                 SIMCONNECT_DATATYPE_INT32);

  SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Wing Span", "feet",
                                 SIMCONNECT_DATATYPE_INT32);

  SimConnect_AddToDataDefinition(hSimConnect, definitionId, "AI Traffic Fromairport", NULL,
                                 SIMCONNECT_DATATYPE_STRING32);
  SimConnect_AddToDataDefinition(hSimConnect, definitionId, "AI Traffic Toairport", NULL,
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

  SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Number of Engines", "number",
                                 SIMCONNECT_DATATYPE_INT32);

  SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Engine Type", "number",
                                 SIMCONNECT_DATATYPE_INT32);
}

// ===============================================================================================
// SimConnectHandler
// ===============================================================================================

SimConnectHandler::SimConnectHandler(bool verboseLogging)
  : p(new SimConnectHandlerPrivate(verboseLogging))
{

}

SimConnectHandler::~SimConnectHandler()
{
  if(p->hSimConnect != NULL)
  {
    HRESULT hr = SimConnect_Close(p->hSimConnect);
    if(hr != S_OK)
      qWarning() << "Error closing SimConnect";
  }
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
  HRESULT hr;

  if(p->verbose)
    qDebug() << "Before open";

  hr = SimConnect_Open(&p->hSimConnect, "Little Navconnect", NULL, 0, 0, 0);
  if(hr == S_OK)
  {
    if(p->verbose)
      qDebug() << "Connected to Flight Simulator";

    p->fillDataDefinitionAicraft(DATA_DEFINITION_AI_AIRCRAFT);
    p->fillDataDefinitionAicraft(DATA_DEFINITION_AI_HELICOPTER);
    p->fillDataDefinitionAicraft(DATA_DEFINITION_USER_AIRCRAFT);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Magvar",
                                   "degrees", SIMCONNECT_DATATYPE_FLOAT32);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "GPS Ground Magnetic Track",
                                   "degrees", SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "GPS Ground True Track",
                                   "degrees",
                                   SIMCONNECT_DATATYPE_FLOAT32);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Plane Alt Above Ground",
                                   "feet",
                                   SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Ground Altitude", "feet",
                                   SIMCONNECT_DATATYPE_FLOAT32);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Ambient Temperature",
                                   "celsius",
                                   SIMCONNECT_DATATYPE_FLOAT32);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Total Air Temperature",
                                   "celsius",
                                   SIMCONNECT_DATATYPE_FLOAT32);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Ambient Wind Velocity",
                                   "knots",
                                   SIMCONNECT_DATATYPE_FLOAT32);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Ambient Wind Direction",
                                   "degrees",
                                   SIMCONNECT_DATATYPE_FLOAT32);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Ambient Precip State",
                                   "mask",
                                   SIMCONNECT_DATATYPE_INT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Ambient In Cloud", "bool",
                                   SIMCONNECT_DATATYPE_INT32);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Ambient Visibility",
                                   "meters",
                                   SIMCONNECT_DATATYPE_FLOAT32);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Sea Level Pressure",
                                   "millibars",
                                   SIMCONNECT_DATATYPE_FLOAT32);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Pitot Ice Pct", "percent",
                                   SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Structural Ice Pct",
                                   "percent",
                                   SIMCONNECT_DATATYPE_FLOAT32);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Total Weight", "pounds",
                                   SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Max Gross Weight",
                                   "pounds",
                                   SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Empty Weight", "pounds",
                                   SIMCONNECT_DATATYPE_FLOAT32);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Fuel Total Quantity",
                                   "gallons",
                                   SIMCONNECT_DATATYPE_FLOAT32);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT,
                                   "Fuel Total Quantity Weight",
                                   "pounds",
                                   SIMCONNECT_DATATYPE_FLOAT32);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Eng Fuel Flow PPH:1",
                                   "Pounds per hour", SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Eng Fuel Flow PPH:2",
                                   "Pounds per hour", SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Eng Fuel Flow PPH:3",
                                   "Pounds per hour", SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Eng Fuel Flow PPH:4",
                                   "Pounds per hour", SIMCONNECT_DATATYPE_FLOAT32);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Eng Fuel Flow GPH:1",
                                   "Gallons per hour", SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Eng Fuel Flow GPH:2",
                                   "Gallons per hour", SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Eng Fuel Flow GPH:3",
                                   "Gallons per hour", SIMCONNECT_DATATYPE_FLOAT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Eng Fuel Flow GPH:4",
                                   "Gallons per hour", SIMCONNECT_DATATYPE_FLOAT32);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Local Time",
                                   "seconds", SIMCONNECT_DATATYPE_INT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Local Year",
                                   "number", SIMCONNECT_DATATYPE_INT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Local Month of Year",
                                   "number", SIMCONNECT_DATATYPE_INT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Local Day of Month",
                                   "number", SIMCONNECT_DATATYPE_INT32);

    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Zulu Time",
                                   "seconds", SIMCONNECT_DATATYPE_INT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Zulu Year",
                                   "number", SIMCONNECT_DATATYPE_INT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Zulu Month of Year",
                                   "number", SIMCONNECT_DATATYPE_INT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Zulu Day of Month",
                                   "number", SIMCONNECT_DATATYPE_INT32);
    SimConnect_AddToDataDefinition(p->hSimConnect, DATA_DEFINITION_USER_AIRCRAFT, "Time Zone Offset",
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

  return true;
}

bool SimConnectHandler::fetchData(atools::fs::sc::SimConnectData& data, int radiusKm)
{
  if(p->verbose)
    qDebug() << "fetchData entered ================================================================";

  // ==========================================================
  if(p->verbose)
    qDebug() << "fetchData AI aircraft details";

  HRESULT hr =
    SimConnect_RequestDataOnSimObjectType(p->hSimConnect, DATA_REQUEST_ID_AI_AIRCRAFT,
                                          DATA_DEFINITION_AI_AIRCRAFT,
                                          static_cast<DWORD>(radiusKm) * 1000,
                                          SIMCONNECT_SIMOBJECT_TYPE_AIRCRAFT);

  if(hr != S_OK)
  {
    qWarning() << "SimConnect_RequestDataOnSimObjectType SIMCONNECT_SIMOBJECT_TYPE_AIRCRAFT: Error";
    p->state = sc::FETCH_ERROR;
    return false;
  }

  if(p->verbose)
    qDebug() << "fetchData AI helicopter details";

  hr =
    SimConnect_RequestDataOnSimObjectType(p->hSimConnect, DATA_REQUEST_ID_AI_HELICOPTER,
                                          DATA_DEFINITION_AI_HELICOPTER,
                                          static_cast<DWORD>(radiusKm) * 1000,
                                          SIMCONNECT_SIMOBJECT_TYPE_HELICOPTER);

  if(hr != S_OK)
  {
    qWarning() << "SimConnect_RequestDataOnSimObjectType SIMCONNECT_SIMOBJECT_TYPE_HELICOPTER: Error";
    p->state = sc::FETCH_ERROR;
    return false;
  }

  hr =
    SimConnect_RequestDataOnSimObjectType(p->hSimConnect, DATA_REQUEST_ID_USER_AIRCRAFT,
                                          DATA_DEFINITION_USER_AIRCRAFT,
                                          0,
                                          SIMCONNECT_SIMOBJECT_TYPE_USER);

  if(hr != S_OK)
  {
    qWarning() << "SimConnect_RequestDataOnSimObjectType SIMCONNECT_SIMOBJECT_TYPE_USER: Error";
    p->state = sc::FETCH_ERROR;
    return false;
  }

  p->weatherDataFetched = false;
  p->userDataFetched = false;
  p->aiDataFetched = false;

  p->simDataAircraft.clear();
  p->simDataAircraftObjectIds.clear();
  p->fetchedMetars.clear();

  int dispatchCycles = 0;
  p->simconnectException = SIMCONNECT_EXCEPTION_NONE;
  do
  {
    hr = SimConnect_CallDispatch(p->hSimConnect, p->dispatchCallback, p);
    if(hr != S_OK)
    {
      qWarning() << "SimConnect_CallDispatch: Error";
      p->state = sc::FETCH_ERROR;
      return false;
    }

    QThread::msleep(5);
    dispatchCycles++;
  } while(!p->userDataFetched && dispatchCycles < 50 &&
          p->simconnectException == SIMCONNECT_EXCEPTION_NONE);

  if(p->verbose)
  {
    if(dispatchCycles > 1)
      qDebug() << "dispatchCycles > 1" << dispatchCycles;
    qDebug() << "numDataFetchedAi" << p->simDataAircraft.size();
  }

  if(p->weatherRequest.isValid())
  {
    for(const QString& weatherPos : p->weatherRequest.getWeatherRequestStation())
    {
      hr =
        SimConnect_WeatherRequestObservationAtStation(p->hSimConnect,
                                                      DATA_REQUEST_ID_WEATHER_STATION,
                                                      weatherPos.toUtf8().data());
      if(hr != S_OK)
      {
        qWarning() <<
        "SimConnect_WeatherRequestObservationAtStation DATA_REQUEST_ID_WEATHER_STATION: Error";
        p->state = sc::FETCH_ERROR;
        return false;
      }
    }

    p->simconnectException = SIMCONNECT_EXCEPTION_NONE;
    dispatchCycles = 0;
    do
    {
      hr = SimConnect_CallDispatch(p->hSimConnect, p->dispatchCallback, p);
      if(hr != S_OK)
      {
        qWarning() << "SimConnect_CallDispatch for weather stations: Error";
        p->state = sc::FETCH_ERROR;
        return false;
      }

      QThread::msleep(5);
      dispatchCycles++;
    } while(!p->weatherDataFetched && dispatchCycles < 50 &&
            p->simconnectException == SIMCONNECT_EXCEPTION_NONE);

    if(p->verbose)
    {
      if(dispatchCycles > 1)
        qDebug() << "dispatchCycles for weather station > 1" << dispatchCycles;
    }

    if(p->fetchedMetars.isEmpty())
    {
      for(const atools::geo::Pos& weatherPos : p->weatherRequest.getWeatherRequestNearest())
      {
        hr =
          SimConnect_WeatherRequestObservationAtNearestStation(p->hSimConnect,
                                                               DATA_REQUEST_ID_WEATHER_NEAREST_STATION,
                                                               weatherPos.getLatY(), weatherPos.getLonX());
        if(hr != S_OK)
        {
          qWarning() <<
          "SimConnect_WeatherRequestObservationAtNearestStation DATA_REQUEST_ID_WEATHER_NEAREST_STATION: Error";
          p->state = sc::FETCH_ERROR;
          return false;
        }
      }

      p->simconnectException = SIMCONNECT_EXCEPTION_NONE;
      dispatchCycles = 0;
      do
      {
        hr = SimConnect_CallDispatch(p->hSimConnect, p->dispatchCallback, p);
        if(hr != S_OK)
        {
          qWarning() << "SimConnect_CallDispatch for weather stations: Error";
          p->state = sc::FETCH_ERROR;
          return false;
        }

        QThread::msleep(5);
        dispatchCycles++;
      } while(!p->weatherDataFetched && dispatchCycles < 50 &&
              p->simconnectException == SIMCONNECT_EXCEPTION_NONE);

      if(p->verbose)
      {
        if(dispatchCycles > 1)
          qDebug() << "dispatchCycles for weather station > 1" << dispatchCycles;
      }
    }
    // for(const atools::geo::Pos& weatherPos : p->weatherRequest.getWeatherRequestInterpolated())
    // {
    // hr =
    // SimConnect_WeatherRequestInterpolatedObservation(p->hSimConnect,
    // DATA_REQUEST_ID_WEATHER_INTERPOLATED,
    // weatherPos.getLatY(), weatherPos.getLonX(),
    // weatherPos.getAltitude());
    // if(hr != S_OK)
    // {
    // qWarning() <<
    // "SimConnect_WeatherRequestInterpolatedObservation DATA_REQUEST_ID_WEATHER_INTERPOLATED: Error";
    // p->state = sc::FETCH_ERROR;
    // return false;
    // }
  }

  p->state = sc::STATEOK;

  QSet<unsigned long> objectIds;
  for(int i = 0; i < p->simDataAircraft.size(); i++)
  {
    unsigned long oid = p->simDataAircraftObjectIds.at(i);
    // Avoid duplicates
    if(!objectIds.contains(oid))
    {
      atools::fs::sc::SimConnectAircraft ap;
      p->copyToSimData(p->simDataAircraft.at(i), ap);
      ap.objectId = static_cast<unsigned int>(oid);
      data.aiAircraft.append(ap);
      objectIds.insert(ap.objectId);
    }
  }

  for(const QString& key : p->fetchedMetars)
    data.metars.append(key);
  p->fetchedMetars.clear();

  if(p->userDataFetched)
  {
    data.userAircraft.flags = atools::fs::sc::NONE;

    p->copyToSimData(p->simData.aircraft, data.userAircraft);
    data.userAircraft.objectId = static_cast<unsigned int>(p->simDataObjectId);

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
    data.userAircraft.magVarDeg = p->simData.magVarDeg;

    data.userAircraft.trackMag = p->simData.planeTrackMagneticDeg;
    data.userAircraft.trackTrue = p->simData.planeTrackTrueDeg;

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
  else
    data.userAircraft.position = atools::geo::Pos();

  if(!p->simRunning || p->simPaused || !p->userDataFetched)
  {
    if(p->verbose)
      qDebug() << "Running" << p->simRunning << "paused" << p->simPaused
               << "userDataFetched" << p->userDataFetched
               << "aiDataFetched" << p->aiDataFetched
               << "weatherDataFetched" << p->weatherDataFetched;
    return false;
  }
  return true;
}

void SimConnectHandler::setWeatherRequest(const atools::fs::sc::WeatherRequest& request)
{
  p->weatherRequest = request;
}

const WeatherRequest& SimConnectHandler::getWeatherRequest() const
{
  return p->weatherRequest;
}

} // namespace sc
} // namespace fs
} // namespace atools
