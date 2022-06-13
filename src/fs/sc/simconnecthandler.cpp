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

#include "fs/sc/simconnecthandler.h"

#include "fs/sc/simconnectapi.h"
#include "fs/sc/weatherrequest.h"
#include "fs/sc/simconnectdata.h"
#include "geo/calculations.h"
#include "win/activationcontext.h"
#include "fs/util/fsutil.h"

#include <QDate>
#include <QTime>
#include <QDateTime>
#include <QThread>
#include <QSet>
#include <QCache>
#include <QLatin1String>
#include <QCoreApplication>
#include <QDir>

#pragma GCC diagnostic ignored "-Wold-style-cast"

using atools::fs::weather::MetarResult;

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
  DATA_REQUEST_ID_AI_BOAT = 4,
  DATA_REQUEST_ID_WEATHER_INTERPOLATED = 5,
  DATA_REQUEST_ID_WEATHER_NEAREST_STATION = 6,
  DATA_REQUEST_ID_WEATHER_STATION = 7
};

enum DataDefinitionId
{
  DATA_DEFINITION_USER_AIRCRAFT = 10,
  DATA_DEFINITION_AI_AIRCRAFT = 20,
  DATA_DEFINITION_AI_HELICOPTER = 30,
  DATA_DEFINITION_AI_BOAT = 40
};

/* Struct that will be filled with raw data from the simconnect interface. */
struct SimDataAircraft
{
  char aircraftTitle[256];
  char aircraftAtcType[256];
  char aircraftAtcModel[256];
  char aircraftAtcId[256];
  char aircraftAtcAirline[256];
  char aircraftAtcFlightNumber[256];
  char category[256]; // "Airplane", "Helicopter", "Boat", "GroundVehicle", "ControlTower", "SimpleObject", "Viewer"

  qint32 userSim;
  qint32 modelRadius;
  qint32 wingSpan;

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

  qint32 transponderCode;

  qint32 numEngines;
  qint32 engineType; // 0 = Piston 1 = Jet 2 = None 3 = Helo(Bell) turbine 4 = Unsupported 5 = Turboprop

  // Not used for MSFS
  char aiFrom[32];
  char aiTo[32];
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

  qint32 autopilotAvailable;
  float altitudeAutopilotFt;

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

  atools::fs::sc::SimConnectApi api;
  atools::win::ActivationContext context;

  /* Callback receiving the data. */
  void dispatchProcedure(SIMCONNECT_RECV *pData, DWORD cbData);

  /* Static method will pass call to object which is passed in pContext. */
  static void CALLBACK dispatchCallback(SIMCONNECT_RECV *pData, DWORD cbData, void *pContext);

  /* Defines the data to fetch. Called after receiving open event */
  void fillDataDefinition();
  void fillDataDefinitionAicraft(DataDefinitionId definitionId);

  void copyToSimData(const SimDataAircraft& simDataUserAircraft,
                     atools::fs::sc::SimConnectAircraft& aircraft);

  bool checkCall(HRESULT hr, const QString& message);
  bool callDispatch(bool& dataFetched, const QString& message);

  SimData simData;
  unsigned long simDataObjectId;

  QVector<SimDataAircraft> simDataAircraft;
  QVector<unsigned long> simDataAircraftObjectIds;

  sc::State state = sc::STATEOK;
  bool dataDefined = false; // fillDataDefinition called

  atools::fs::sc::WeatherRequest weatherRequest;
  QVector<QString> fetchedMetars;
  SIMCONNECT_EXCEPTION simconnectException;
  SIMCONNECT_RECV_OPEN openData;

  bool simRunning = true, simPaused = false, verbose = false, simConnectLoaded = false,
       userDataFetched = false, aiDataFetched = false, weatherDataFetched = false, msfs = false;
};

void SimConnectHandlerPrivate::dispatchProcedure(SIMCONNECT_RECV *pData, DWORD cbData)
{
  Q_UNUSED(cbData)

  if(verbose)
    qDebug() << "DispatchProcedure entered";

  switch(pData->dwID)
  {
    case SIMCONNECT_RECV_ID_OPEN:
      // enter code to handle SimConnect version information received in a SIMCONNECT_RECV_OPEN structure.
      openData = *static_cast<SIMCONNECT_RECV_OPEN *>(pData);

      // FSX ==========
      // App Name Microsoft Flight Simulator X App Version 10.0 App Build  62615.0 Version  10.0 Build  62615.0

      // MSFS ==========
      // SimConnectHandler App Name KittyHawk App Version 11.0 App Build 282174.999 Version 11.0 Build 62651.3

      // Print some useful simconnect interface data to log ====================
      qInfo() << Q_FUNC_INFO
              << "App Name" << openData.szApplicationName
              << "App Version" << openData.dwApplicationVersionMajor
              << "." << openData.dwApplicationVersionMinor
              << "App Build" << openData.dwApplicationBuildMajor
              << "." << openData.dwApplicationBuildMinor
              << "Version" << openData.dwSimConnectVersionMajor
              << "." << openData.dwSimConnectVersionMinor
              << "Build" << openData.dwSimConnectBuildMajor
              << "." << openData.dwSimConnectBuildMinor;

      msfs = strcmp(openData.szApplicationName, "KittyHawk") == 0 ||
             (openData.dwSimConnectVersionMajor == 11 && openData.dwSimConnectBuildMajor == 62651);

      qInfo() << Q_FUNC_INFO << "MSFS detected" << msfs;

      // Now define other data to fetch
      fillDataDefinition();

      break;

    case SIMCONNECT_RECV_ID_EXCEPTION:
      {
        // enter code to handle errors received in a SIMCONNECT_RECV_EXCEPTION structure.
        SIMCONNECT_RECV_EXCEPTION *except = static_cast<SIMCONNECT_RECV_EXCEPTION *>(pData);
        simconnectException = static_cast<SIMCONNECT_EXCEPTION>(except->dwException);

        if(simconnectException != SIMCONNECT_EXCEPTION_WEATHER_UNABLE_TO_GET_OBSERVATION || verbose)
          qWarning() << "SimConnect exception" << except->dwException
                     << "send ID" << except->dwSendID << "index" << except->dwIndex;

        state = sc::EXCEPTION;
        break;
      }

    case SIMCONNECT_RECV_ID_EVENT:
      {
        SIMCONNECT_RECV_EVENT *evt = static_cast<SIMCONNECT_RECV_EVENT *>(pData);

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
        SIMCONNECT_RECV_SIMOBJECT_DATA_BYTYPE *pObjData = static_cast<SIMCONNECT_RECV_SIMOBJECT_DATA_BYTYPE *>(pData);

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
                pObjData->dwRequestID == DATA_REQUEST_ID_AI_HELICOPTER ||
                pObjData->dwRequestID == DATA_REQUEST_ID_AI_BOAT)
        {
          if(verbose)
            qDebug() << "DATA_REQUEST_ID_AI_AIRCRAFT/HELICOPTER/BOAT"
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
        SIMCONNECT_RECV_WEATHER_OBSERVATION *pObjData = static_cast<SIMCONNECT_RECV_WEATHER_OBSERVATION *>(pData);

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

void SimConnectHandlerPrivate::copyToSimData(const SimDataAircraft& simDataUserAircraft, SimConnectAircraft& aircraft)
{
  aircraft.flags = atools::fs::sc::SIM_FSX_P3D;
  aircraft.airplaneTitle = simDataUserAircraft.aircraftTitle;
  aircraft.airplaneModel = simDataUserAircraft.aircraftAtcModel;
  aircraft.airplaneReg = simDataUserAircraft.aircraftAtcId;
  aircraft.airplaneType = simDataUserAircraft.aircraftAtcType;
  aircraft.airplaneAirline = simDataUserAircraft.aircraftAtcAirline;
  aircraft.airplaneFlightnumber = simDataUserAircraft.aircraftAtcFlightNumber;
  aircraft.fromIdent = simDataUserAircraft.aiFrom;
  aircraft.toIdent = simDataUserAircraft.aiTo;

  QString cat = QString(simDataUserAircraft.category).toLower().trimmed();
  if(cat == "airplane")
    aircraft.category = AIRPLANE;
  else if(cat == "helicopter")
    aircraft.category = HELICOPTER;
  else if(cat == "boat")
    aircraft.category = BOAT;
  else if(cat == "groundvehicle")
    aircraft.category = GROUNDVEHICLE;
  else if(cat == "controltower")
    aircraft.category = CONTROLTOWER;
  else if(cat == "simpleobject")
    aircraft.category = SIMPLEOBJECT;
  else if(cat == "viewer")
    aircraft.category = VIEWER;

  aircraft.wingSpanFt = static_cast<quint16>(simDataUserAircraft.wingSpan);
  aircraft.modelRadiusFt = static_cast<quint16>(simDataUserAircraft.modelRadius);

  aircraft.numberOfEngines = static_cast<quint8>(simDataUserAircraft.numEngines);
  aircraft.engineType = static_cast<EngineType>(simDataUserAircraft.engineType);

  aircraft.position.setLonX(simDataUserAircraft.longitudeDeg);
  aircraft.position.setLatY(simDataUserAircraft.latitudeDeg);
  aircraft.position.setAltitude(simDataUserAircraft.altitudeFt);

  aircraft.groundSpeedKts = simDataUserAircraft.groundVelocityKts;
  aircraft.indicatedAltitudeFt = simDataUserAircraft.indicatedAltitudeFt;
  aircraft.headingMagDeg = simDataUserAircraft.planeHeadingMagneticDeg;
  aircraft.headingTrueDeg = simDataUserAircraft.planeHeadingTrueDeg;

  aircraft.trueAirspeedKts = simDataUserAircraft.airspeedTrueKts;
  aircraft.indicatedSpeedKts = simDataUserAircraft.airspeedIndicatedKts;
  aircraft.machSpeed = simDataUserAircraft.airspeedMach;
  aircraft.verticalSpeedFeetPerMin = simDataUserAircraft.verticalSpeedFps * 60.f;

  aircraft.transponderCode = atools::fs::util::decodeTransponderCode(simDataUserAircraft.transponderCode);

  if(simDataUserAircraft.isSimOnGround > 0)
    aircraft.flags |= atools::fs::sc::ON_GROUND;
  if(simDataUserAircraft.userSim > 0)
    aircraft.flags |= atools::fs::sc::IS_USER;

  if(simPaused > 0)
    aircraft.flags |= atools::fs::sc::SIM_PAUSED;
}

bool SimConnectHandlerPrivate::checkCall(HRESULT hr, const QString& message)
{
  if(verbose)
    qDebug() << "check call" << message;

  if(hr != S_OK)
  {
    qWarning() << "Error during" << message;
    state = sc::FETCH_ERROR;
    return false;
  }
  return true;
}

bool SimConnectHandlerPrivate::callDispatch(bool& dataFetched, const QString& message)
{
  if(verbose)
    qDebug() << "call dispatch enter" << message;

  simconnectException = SIMCONNECT_EXCEPTION_NONE;

  int dispatchCycles = 0;
  dataFetched = false;
  do
  {
    HRESULT hr = api.CallDispatch(dispatchCallback, this);

    if(hr != S_OK)
    {
      // Ignore the station exception
      if(simconnectException != SIMCONNECT_EXCEPTION_WEATHER_UNABLE_TO_GET_OBSERVATION)
      {
        qWarning() << "SimConnect_CallDispatch during " << message << ": Exception" << simconnectException;
        state = sc::FETCH_ERROR;
        return false;
      }
      else if(verbose)
        qDebug() << "SimConnect_CallDispatch during " << message << ": Exception" << simconnectException;
    }

    QThread::msleep(5);
    dispatchCycles++;
  } while(!dataFetched && dispatchCycles < 50 && simconnectException == SIMCONNECT_EXCEPTION_NONE);

  if(verbose)
    qDebug() << "call dispatch leave" << message << "cycles" << dispatchCycles;

  return true;
}

void SimConnectHandlerPrivate::fillDataDefinition()
{
  fillDataDefinitionAicraft(DATA_DEFINITION_AI_AIRCRAFT);
  fillDataDefinitionAicraft(DATA_DEFINITION_AI_HELICOPTER);
  fillDataDefinitionAicraft(DATA_DEFINITION_AI_BOAT);
  fillDataDefinitionAicraft(DATA_DEFINITION_USER_AIRCRAFT);

  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Magvar",
                          "degrees", SIMCONNECT_DATATYPE_FLOAT32);

  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "GPS Ground Magnetic Track",
                          "degrees", SIMCONNECT_DATATYPE_FLOAT32);
  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "GPS Ground True Track",
                          "degrees",
                          SIMCONNECT_DATATYPE_FLOAT32);

  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Plane Alt Above Ground",
                          "feet",
                          SIMCONNECT_DATATYPE_FLOAT32);
  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Ground Altitude", "feet",
                          SIMCONNECT_DATATYPE_FLOAT32);

  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Autopilot Available", "bool",
                          SIMCONNECT_DATATYPE_INT32);
  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Autopilot Altitude Lock Var", "feet",
                          SIMCONNECT_DATATYPE_FLOAT32);

  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Ambient Temperature",
                          "celsius",
                          SIMCONNECT_DATATYPE_FLOAT32);

  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Total Air Temperature",
                          "celsius",
                          SIMCONNECT_DATATYPE_FLOAT32);

  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Ambient Wind Velocity",
                          "knots",
                          SIMCONNECT_DATATYPE_FLOAT32);

  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Ambient Wind Direction",
                          "degrees",
                          SIMCONNECT_DATATYPE_FLOAT32);

  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Ambient Precip State",
                          "mask",
                          SIMCONNECT_DATATYPE_INT32);
  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Ambient In Cloud", "bool",
                          SIMCONNECT_DATATYPE_INT32);

  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Ambient Visibility",
                          "meters",
                          SIMCONNECT_DATATYPE_FLOAT32);

  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Sea Level Pressure",
                          "millibars",
                          SIMCONNECT_DATATYPE_FLOAT32);

  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Pitot Ice Pct", "percent",
                          SIMCONNECT_DATATYPE_FLOAT32);
  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Structural Ice Pct",
                          "percent",
                          SIMCONNECT_DATATYPE_FLOAT32);

  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Total Weight", "pounds",
                          SIMCONNECT_DATATYPE_FLOAT32);
  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Max Gross Weight",
                          "pounds",
                          SIMCONNECT_DATATYPE_FLOAT32);
  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Empty Weight", "pounds",
                          SIMCONNECT_DATATYPE_FLOAT32);

  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Fuel Total Quantity",
                          "gallons",
                          SIMCONNECT_DATATYPE_FLOAT32);

  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT,
                          "Fuel Total Quantity Weight",
                          "pounds",
                          SIMCONNECT_DATATYPE_FLOAT32);

  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Eng Fuel Flow PPH:1",
                          "Pounds per hour", SIMCONNECT_DATATYPE_FLOAT32);
  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Eng Fuel Flow PPH:2",
                          "Pounds per hour", SIMCONNECT_DATATYPE_FLOAT32);
  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Eng Fuel Flow PPH:3",
                          "Pounds per hour", SIMCONNECT_DATATYPE_FLOAT32);
  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Eng Fuel Flow PPH:4",
                          "Pounds per hour", SIMCONNECT_DATATYPE_FLOAT32);

  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Eng Fuel Flow GPH:1",
                          "Gallons per hour", SIMCONNECT_DATATYPE_FLOAT32);
  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Eng Fuel Flow GPH:2",
                          "Gallons per hour", SIMCONNECT_DATATYPE_FLOAT32);
  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Eng Fuel Flow GPH:3",
                          "Gallons per hour", SIMCONNECT_DATATYPE_FLOAT32);
  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Eng Fuel Flow GPH:4",
                          "Gallons per hour", SIMCONNECT_DATATYPE_FLOAT32);

  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Local Time",
                          "seconds", SIMCONNECT_DATATYPE_INT32);
  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Local Year",
                          "number", SIMCONNECT_DATATYPE_INT32);
  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Local Month of Year",
                          "number", SIMCONNECT_DATATYPE_INT32);
  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Local Day of Month",
                          "number", SIMCONNECT_DATATYPE_INT32);

  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Zulu Time",
                          "seconds", SIMCONNECT_DATATYPE_INT32);
  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Zulu Year",
                          "number", SIMCONNECT_DATATYPE_INT32);
  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Zulu Month of Year",
                          "number", SIMCONNECT_DATATYPE_INT32);
  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Zulu Day of Month",
                          "number", SIMCONNECT_DATATYPE_INT32);

  // Measured in seconds, positive west of GMT.
  api.AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Time Zone Offset",
                          "seconds", SIMCONNECT_DATATYPE_INT32);

  // Request an event when the simulation starts or pauses
  api.SubscribeToSystemEvent(EVENT_SIM_STATE, "Sim");
  api.SubscribeToSystemEvent(EVENT_SIM_PAUSE, "Pause");

  state = sc::STATEOK;
  dataDefined = true;
}

void SimConnectHandlerPrivate::fillDataDefinitionAicraft(DataDefinitionId definitionId)
{
  // Set up the data definition, but do not yet do anything with it
  api.AddToDataDefinition(definitionId, "Title", nullptr, SIMCONNECT_DATATYPE_STRING256);

  api.AddToDataDefinition(definitionId, "ATC Type", nullptr, SIMCONNECT_DATATYPE_STRING256);

  api.AddToDataDefinition(definitionId, "ATC Model", nullptr, SIMCONNECT_DATATYPE_STRING256);

  api.AddToDataDefinition(definitionId, "ATC Id", nullptr, SIMCONNECT_DATATYPE_STRING256);

  api.AddToDataDefinition(definitionId, "ATC Airline", nullptr, SIMCONNECT_DATATYPE_STRING256);

  api.AddToDataDefinition(definitionId, "ATC Flight Number", nullptr, SIMCONNECT_DATATYPE_STRING256);

  api.AddToDataDefinition(definitionId, "Category", nullptr, SIMCONNECT_DATATYPE_STRING256);

  api.AddToDataDefinition(definitionId, "Is User Sim", "bool", SIMCONNECT_DATATYPE_INT32);

  api.AddToDataDefinition(definitionId, "Visual Model Radius", "feet", SIMCONNECT_DATATYPE_INT32);

  api.AddToDataDefinition(definitionId, "Wing Span", "feet", SIMCONNECT_DATATYPE_INT32);

  api.AddToDataDefinition(definitionId, "Plane Altitude", "feet", SIMCONNECT_DATATYPE_FLOAT32);
  api.AddToDataDefinition(definitionId, "Plane Latitude", "degrees", SIMCONNECT_DATATYPE_FLOAT32);
  api.AddToDataDefinition(definitionId, "Plane Longitude", "degrees", SIMCONNECT_DATATYPE_FLOAT32);

  api.AddToDataDefinition(definitionId, "Ground Velocity", "knots", SIMCONNECT_DATATYPE_FLOAT32);
  api.AddToDataDefinition(definitionId, "Indicated Altitude", "feet", SIMCONNECT_DATATYPE_FLOAT32);

  api.AddToDataDefinition(definitionId, "Plane Heading Degrees Magnetic", "degrees", SIMCONNECT_DATATYPE_FLOAT32);

  api.AddToDataDefinition(definitionId, "Plane Heading Degrees True", "degrees", SIMCONNECT_DATATYPE_FLOAT32);

  api.AddToDataDefinition(definitionId, "Sim On Ground", "bool", SIMCONNECT_DATATYPE_INT32);

  api.AddToDataDefinition(definitionId, "Airspeed True", "knots", SIMCONNECT_DATATYPE_FLOAT32);
  api.AddToDataDefinition(definitionId, "Airspeed Indicated", "knots", SIMCONNECT_DATATYPE_FLOAT32);
  api.AddToDataDefinition(definitionId, "Airspeed Mach", "mach", SIMCONNECT_DATATYPE_FLOAT32);
  api.AddToDataDefinition(definitionId, "Vertical Speed", "feet per second", SIMCONNECT_DATATYPE_FLOAT32);

  api.AddToDataDefinition(definitionId, "Transponder Code:1", "number", SIMCONNECT_DATATYPE_INT32);

  api.AddToDataDefinition(definitionId, "Number of Engines", "number", SIMCONNECT_DATATYPE_INT32);

  api.AddToDataDefinition(definitionId, "Engine Type", "number", SIMCONNECT_DATATYPE_INT32);

  api.AddToDataDefinition(definitionId, "AI Traffic Fromairport", nullptr, SIMCONNECT_DATATYPE_STRING32);
  api.AddToDataDefinition(definitionId, "AI Traffic Toairport", nullptr, SIMCONNECT_DATATYPE_STRING32);
}

// ===============================================================================================
// SimConnectHandler
// ===============================================================================================

SimConnectHandler::SimConnectHandler(bool verboseLogging)
  : p(new SimConnectHandlerPrivate(verboseLogging)), appName(QCoreApplication::applicationName().toLatin1())
{

}

SimConnectHandler::~SimConnectHandler()
{
  HRESULT hr = p->api.Close();
  if(hr != S_OK)
    qWarning() << "Error closing SimConnect";

  p->context.freeLibrary("SimConnect.dll");
  p->context.deactivate();
  p->context.release();
  delete p;
}

bool SimConnectHandler::loadSimConnect(const QString& manifestPath)
{
  p->simConnectLoaded = false;

  // Try local copy first
  QString simconnectDll = QCoreApplication::applicationDirPath() + QDir::separator() + "SimConnect.dll";
  bool activated = false;
  if(!QFile::exists(simconnectDll))
  {
    // No local copy - load from WinSxS
    if(!p->context.create(manifestPath))
      return false;

    if(!p->context.activate())
      return false;

    simconnectDll = "SimConnect.dll";
    activated = true;
  }

  if(!p->context.loadLibrary(simconnectDll))
    return false;

  if(activated)
  {
    if(!p->context.deactivate())
      return false;
  }

  if(!p->api.bindFunctions(p->context))
    return false;

  p->simConnectLoaded = true;
  return true;
}

bool SimConnectHandler::isSimRunning() const
{
  return p->simRunning;
}

bool SimConnectHandler::isSimPaused() const
{
  return p->simPaused;
}

bool SimConnectHandler::canFetchWeather() const
{
  return !p->msfs;
}

bool SimConnectHandler::isLoaded() const
{
  return p->simConnectLoaded;
}

State atools::fs::sc::SimConnectHandler::getState() const
{
  return p->state;
}

bool SimConnectHandler::connect()
{
  HRESULT hr;

  if(p->verbose)
    qDebug() << Q_FUNC_INFO << "Before Open";

  hr = p->api.Open(appName.constData(), nullptr, 0, nullptr, 0);
  if(hr == S_OK)
  {
    qDebug() << Q_FUNC_INFO << "Opened";

    // Call dispatch function to get the SimConnect version information in SIMCONNECT_RECV_ID_OPEN event
    // This event will also do the data definition
    bool fetched = false;
    p->callDispatch(fetched, "OPEN");

    return true;
  }
  else
  {
    if(p->verbose)
      qWarning() << "SimConnect_Open: Error";
    p->state = sc::OPEN_ERROR;
    return false;
  }
}

bool SimConnectHandler::fetchData(atools::fs::sc::SimConnectData& data, int radiusKm, atools::fs::sc::Options options)
{
  if(!p->dataDefined)
  {
    qWarning() << Q_FUNC_INFO << "Fetch before data definition";
    return false;
  }

  if(p->verbose)
    qDebug() << "fetchData entered ================================================================";

  // === Get AI aircraft =======================================================
  p->simDataAircraft.clear();
  p->simDataAircraftObjectIds.clear();
  p->simDataObjectId = 0;

  HRESULT hr = 0;

  if(options & FETCH_AI_AIRCRAFT)
  {
    hr = p->api.RequestDataOnSimObjectType(
      DATA_REQUEST_ID_AI_AIRCRAFT, DATA_DEFINITION_AI_AIRCRAFT,
      static_cast<DWORD>(radiusKm) * 1000, SIMCONNECT_SIMOBJECT_TYPE_AIRCRAFT);
    if(!p->checkCall(hr, "DATA_REQUEST_ID_AI_AIRCRAFT"))
      return false;

    hr = p->api.RequestDataOnSimObjectType(
      DATA_REQUEST_ID_AI_HELICOPTER, DATA_DEFINITION_AI_HELICOPTER,
      static_cast<DWORD>(radiusKm) * 1000, SIMCONNECT_SIMOBJECT_TYPE_HELICOPTER);
    if(!p->checkCall(hr, "DATA_REQUEST_ID_AI_HELICOPTER"))
      return false;
  }

  if(options & FETCH_AI_BOAT)
  {
    hr = p->api.RequestDataOnSimObjectType(
      DATA_REQUEST_ID_AI_BOAT, DATA_DEFINITION_AI_BOAT,
      static_cast<DWORD>(radiusKm) * 1000, SIMCONNECT_SIMOBJECT_TYPE_BOAT);
    if(!p->checkCall(hr, "DATA_REQUEST_ID_AI_BOAT"))
      return false;
  }

  p->callDispatch(p->aiDataFetched,
                  "DATA_REQUEST_ID_AI_HELICOPTER, DATA_REQUEST_ID_AI_BOAT and DATA_REQUEST_ID_AI_AIRCRAFT");

  // === Get user aircraft =======================================================
  hr = p->api.RequestDataOnSimObjectType(
    DATA_REQUEST_ID_USER_AIRCRAFT, DATA_DEFINITION_USER_AIRCRAFT, 0,
    SIMCONNECT_SIMOBJECT_TYPE_USER);
  if(!p->checkCall(hr, "DATA_REQUEST_ID_USER_AIRCRAFT"))
    return false;

  p->callDispatch(p->userDataFetched, "DATA_REQUEST_ID_USER_AIRCRAFT");

  p->state = sc::STATEOK;

  // Get user aircraft =======================================================================
  QSet<unsigned long> objectIds;
  for(int i = 0; i < p->simDataAircraft.size(); i++)
  {
    unsigned long oid = p->simDataAircraftObjectIds.at(i);
    // Avoid duplicates
    if(!objectIds.contains(oid))
    {
      atools::fs::sc::SimConnectAircraft aircraft;
      p->copyToSimData(p->simDataAircraft.at(i), aircraft);
      aircraft.objectId = static_cast<unsigned int>(oid);
      data.aiAircraft.append(aircraft);
      objectIds.insert(aircraft.objectId);
    }
  }

  // Get user aircraft =======================================================================
  if(p->userDataFetched)
  {
    p->copyToSimData(p->simData.aircraft, data.userAircraft);
    data.userAircraft.objectId = static_cast<unsigned int>(p->simDataObjectId);

    data.userAircraft.groundAltitudeFt = p->simData.groundAltitudeFt;

    if(p->simData.autopilotAvailable > 0)
      data.userAircraft.altitudeAutopilotFt = p->simData.altitudeAutopilotFt;
    else
      data.userAircraft.altitudeAutopilotFt = 0.f;

    data.userAircraft.altitudeAboveGroundFt = p->simData.planeAboveGroundFt;

    if(p->simData.ambientPrecipStateFlags & 4)
      data.userAircraft.flags |= atools::fs::sc::IN_RAIN;
    if(p->simData.ambientPrecipStateFlags & 8)
      data.userAircraft.flags |= atools::fs::sc::IN_SNOW;

    if(p->simData.ambientIsInCloud > 0)
      data.userAircraft.flags |= atools::fs::sc::IN_CLOUD;

    data.userAircraft.ambientTemperatureCelsius = p->simData.ambientTemperatureC;
    data.userAircraft.totalAirTemperatureCelsius = p->simData.totalAirTemperatureC;
    data.userAircraft.ambientVisibilityMeter = p->simData.ambientVisibilityMeter;

    data.userAircraft.seaLevelPressureMbar = p->simData.seaLevelPressureMbar;
    data.userAircraft.pitotIcePercent = static_cast<quint8>(p->simData.pitotIcePercent);
    data.userAircraft.structuralIcePercent = static_cast<quint8>(p->simData.structuralIcePercent);
    data.userAircraft.airplaneTotalWeightLbs = p->simData.airplaneTotalWeightLbs;
    data.userAircraft.airplaneMaxGrossWeightLbs = p->simData.airplaneMaxGrossWeightLbs;
    data.userAircraft.airplaneEmptyWeightLbs = p->simData.airplaneEmptyWeightLbs;
    data.userAircraft.fuelTotalQuantityGallons = p->simData.fuelTotalQuantityGallons;
    data.userAircraft.fuelTotalWeightLbs = p->simData.fuelTotalWeightLbs;
    data.userAircraft.magVarDeg = p->simData.magVarDeg;

    data.userAircraft.trackMagDeg = p->simData.planeTrackMagneticDeg;
    data.userAircraft.trackTrueDeg = p->simData.planeTrackTrueDeg;

    // Summarize fuel flow for all engines
    data.userAircraft.fuelFlowPPH =
      p->simData.fuelFlowPph1 + p->simData.fuelFlowPph2 + p->simData.fuelFlowPph3 + p->simData.fuelFlowPph4;

    data.userAircraft.fuelFlowGPH =
      p->simData.fuelFlowGph1 + p->simData.fuelFlowGph2 + p->simData.fuelFlowGph3 + p->simData.fuelFlowGph4;

    data.userAircraft.windDirectionDegT = p->simData.ambientWindDirectionDegT;
    data.userAircraft.windSpeedKts = p->simData.ambientWindVelocityKts;

    // Build local time and use timezone offset from simulator
    QDate localDate(p->simData.localYear, p->simData.localMonth, p->simData.localDay);
    QTime localTime = QTime::fromMSecsSinceStartOfDay(p->simData.localTime * 1000);

    // Offset from FS: Measured in seconds, positive west of GMT.
    QDateTime localDateTime(localDate, localTime, Qt::OffsetFromUTC, -p->simData.timeZoneOffsetSeconds);
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

bool SimConnectHandler::fetchWeatherData(atools::fs::sc::SimConnectData& data)
{
  if(!p->dataDefined)
  {
    qWarning() << Q_FUNC_INFO << "Fetch before data definition";
    return false;
  }

  if(p->weatherRequest.isValid())
  {
    p->fetchedMetars.clear();

    HRESULT hr;
    MetarResult result;
    result.init(p->weatherRequest.getStation(), p->weatherRequest.getPosition());

    if(!result.requestIdent.isEmpty())
    {
      // == weather for station ========================================================
      hr = p->api.WeatherRequestObservationAtStation(
        DATA_REQUEST_ID_WEATHER_STATION, result.requestIdent.toUtf8().constData());
      if(!p->checkCall(hr, "DATA_REQUEST_ID_WEATHER_STATION" + result.requestIdent))
        return false;

      p->fetchedMetars.clear();
      p->callDispatch(p->weatherDataFetched, "DATA_REQUEST_ID_WEATHER_STATION" + result.requestIdent);

      if(p->fetchedMetars.size() > 1)
        qWarning() << "Got more than one metar for station"
                   << result.requestIdent << ":" << p->fetchedMetars.size();

      if(!p->fetchedMetars.isEmpty())
        result.metarForStation = p->fetchedMetars.first();
    }

    if(p->fetchedMetars.isEmpty())
    {
      // Nothing found for station or no station given

      // == weather for nearest station ========================================================
      if(result.requestPos.isValid())
      {
        hr = p->api.WeatherRequestObservationAtNearestStation(
          DATA_REQUEST_ID_WEATHER_NEAREST_STATION,
          result.requestPos.getLatY(), result.requestPos.getLonX());
        if(!p->checkCall(hr, "DATA_REQUEST_ID_WEATHER_NEAREST_STATION" + result.requestPos.toString()))
          return false;

        p->fetchedMetars.clear();
        p->callDispatch(p->weatherDataFetched,
                        "DATA_REQUEST_ID_WEATHER_NEAREST_STATION" + result.requestPos.toString());

        if(p->fetchedMetars.size() > 1)
          qWarning() << "Got more than one nearest metar for position"
                     << result.requestPos.toString() << ":" << p->fetchedMetars.size();

        if(!p->fetchedMetars.isEmpty())
          result.metarForNearest = p->fetchedMetars.first();
      }

      // == interpolated weather ========================================================
      if(result.requestPos.isValid())
      {
        hr = p->api.WeatherRequestInterpolatedObservation(
          DATA_REQUEST_ID_WEATHER_INTERPOLATED,
          result.requestPos.getLatY(), result.requestPos.getLonX(), result.requestPos.getAltitude());
        if(!p->checkCall(hr, "DATA_REQUEST_ID_WEATHER_INTERPOLATED"))
          return false;

        p->fetchedMetars.clear();
        p->callDispatch(p->weatherDataFetched, "DATA_REQUEST_ID_WEATHER_INTERPOLATED");

        if(p->fetchedMetars.size() > 1)
          qWarning() << "Got more than one interpolated metar for position"
                     << result.requestPos.toString() << ":" << p->fetchedMetars.size();

        if(!p->fetchedMetars.isEmpty())
          result.metarForInterpolated = p->fetchedMetars.first();
      }
    }
    data.metarResults.append(result);
  }

  if(!p->simRunning || p->simPaused || !p->weatherDataFetched)
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

void SimConnectHandler::addWeatherRequest(const atools::fs::sc::WeatherRequest& request)
{
  p->weatherRequest = request;
}

const WeatherRequest& SimConnectHandler::getWeatherRequest() const
{
  return p->weatherRequest;
}

QString atools::fs::sc::SimConnectHandler::getName() const
{
  return QLatin1String("SimConnect");
}

} // namespace sc
} // namespace fs
} // namespace atools
