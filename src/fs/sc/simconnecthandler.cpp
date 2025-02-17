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

#include "fs/sc/simconnecthandler.h"

#include "fs/sc/simconnectapi.h"
#include "fs/sc/weatherrequest.h"
#include "fs/sc/simconnectdata.h"
#include "win/activationcontext.h"
#include "fs/util/fsutil.h"
#include "atools.h"

#include <QDate>
#include <QTime>
#include <QDateTime>
#include <QThread>
#include <QSet>
#include <QCache>
#include <QLatin1String>
#include <QCoreApplication>
#include <QStringBuilder>
#include <QDir>

#pragma GCC diagnostic ignored "-Wold-style-cast"

namespace atools {
namespace fs {
namespace sc {

enum EventIds
{
  EVENT_SIM_STATE,
  EVENT_SIM_PAUSE,
  EVENT_AIRCRAFT_LOADED
};

enum DataRequestId
{
  DATA_REQUEST_ID_USER_AIRCRAFT = 1,
  DATA_REQUEST_ID_AI_AIRCRAFT = 2,
  DATA_REQUEST_ID_AI_HELICOPTER = 3,
  DATA_REQUEST_ID_AI_BOAT = 4,

  // No weather requests in 64-bit version which uses only MSFS
#if defined(SIMCONNECT_BUILD_WIN32)
  DATA_REQUEST_ID_WEATHER_INTERPOLATED = 5,
  DATA_REQUEST_ID_WEATHER_NEAREST_STATION = 6,
  DATA_REQUEST_ID_WEATHER_STATION = 7
#endif
};

enum DataDefinitionId
{
  DATA_DEFINITION_USER_AIRCRAFT = 10,
  DATA_DEFINITION_AI_AIRCRAFT = 20,
  DATA_DEFINITION_AI_HELICOPTER = 30,
  DATA_DEFINITION_AI_BOAT = 40
};

#pragma pack(push, 1)
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

  float altitudeFt; // Actual altitude
  double latitudeDeg;
  double longitudeDeg;

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

  float localTimeSeconds;
  qint32 localYear;
  qint32 localMonth;
  qint32 localDay;
  float zuluTimeSeconds;
  qint32 zuluYear;
  qint32 zuluMonth;
  qint32 zuluDay;
  qint32 timeZoneOffsetSeconds;
};

#pragma pack(pop)

class SimConnectHandlerPrivate
{
public:
  SimConnectHandlerPrivate(bool verboseLogging)
    : verbose(verboseLogging)
  {
    api = new SimConnectApi;
  }

  ~SimConnectHandlerPrivate()
  {
    ATOOLS_DELETE_LOG(api);
  }

  atools::fs::sc::SimConnectApi *api = nullptr;
  atools::win::ActivationContext *activationContext = nullptr;
  QString libraryName;

  /* Callback receiving the data. */
  void dispatchProcedure(SIMCONNECT_RECV *pData, DWORD cbData);

  /* Static method will pass call to object which is passed in pContext. */
  static void CALLBACK dispatchFunction(SIMCONNECT_RECV *pData, DWORD cbData, void *pContext);

  /* Defines the data to fetch. Called after receiving open event */
  void fillDataDefinition();
  void fillDataDefinitionAicraft(DataDefinitionId definitionId);

  void copyToSimConnectAircraft(const SimDataAircraft& simDataAircraft, atools::fs::sc::SimConnectAircraft& aircraft);

  bool checkCall(HRESULT hr, const QString& message);
  bool callDispatch(bool& dataFetched, const QString& message);

  SimData simData;
  unsigned long simDataObjectId;

  // All aircraft fetched by dispatch method
  QHash<unsigned long, SimDataAircraft> simDataAircraftMap;

  sc::State state = sc::STATEOK;
  bool dataDefined = false; // fillDataDefinition called

  atools::fs::sc::WeatherRequest weatherRequest;
  QVector<QString> fetchedMetars;
  SIMCONNECT_EXCEPTION simconnectException;
  SIMCONNECT_RECV_OPEN openData;

  bool simRunning = true, simPaused = false, verbose = false, simConnectLoaded = false,
       userDataFetched = false, aiDataFetched = false, weatherDataFetched = false, paused = false;

  // Log new aircraft categories only once
  QSet<QString> categoriesLogged;

#if defined(SIMCONNECT_BUILD_WIN64)
  QDateTime lastSystemRequestTime; // Do not request for every fetch
  QString aircraftFilePath; // Clean path to aircraft.cfg file in MSFS
#endif
};

void CALLBACK SimConnectHandlerPrivate::dispatchFunction(SIMCONNECT_RECV *pData, DWORD cbData, void *pContext)
{
  SimConnectHandlerPrivate *handlerClass = static_cast<SimConnectHandlerPrivate *>(pContext);
  handlerClass->dispatchProcedure(pData, cbData);
}

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

      // MSFS 2020 ==========
      // SimConnectHandler App Name KittyHawk App Version 11.0 App Build 282174.999 Version 11.0 Build 62651.3

      // MSFS 2024 ==========
      // SimConnectHandler App Name SunRise App Version 12.1 App Build 282174.999 Version 12.1 Build 0.0

      // Print some useful simconnect interface data to log ====================
      qInfo().nospace() << Q_FUNC_INFO
                        << " App Name " << openData.szApplicationName
                        << " App Version " << openData.dwApplicationVersionMajor << "." << openData.dwApplicationVersionMinor
                        << " App Build " << openData.dwApplicationBuildMajor << "." << openData.dwApplicationBuildMinor
                        << " Version " << openData.dwSimConnectVersionMajor << "." << openData.dwSimConnectVersionMinor
                        << " Build " << openData.dwSimConnectBuildMajor << "." << openData.dwSimConnectBuildMinor;
      break;

    case SIMCONNECT_RECV_ID_EXCEPTION:
      {
        // enter code to handle errors received in a SIMCONNECT_RECV_EXCEPTION structure.
        SIMCONNECT_RECV_EXCEPTION *except = static_cast<SIMCONNECT_RECV_EXCEPTION *>(pData);
        simconnectException = static_cast<SIMCONNECT_EXCEPTION>(except->dwException);

        if(simconnectException != SIMCONNECT_EXCEPTION_WEATHER_UNABLE_TO_GET_OBSERVATION)
        {
          qWarning() << "SimConnect exception" << except->dwException
                     << "send ID" << except->dwSendID << "index" << except->dwIndex;
          state = sc::EXCEPTION;
        }
        else
          simconnectException = SIMCONNECT_EXCEPTION_NONE;
        break;
      }

#if defined(SIMCONNECT_BUILD_WIN64)
    case SIMCONNECT_RECV_ID_SYSTEM_STATE:
      {
        // Aircraft file changed - reload type
        SIMCONNECT_RECV_SYSTEM_STATE *evt = static_cast<SIMCONNECT_RECV_SYSTEM_STATE *>(pData);
        if(verbose)
          qDebug() << "SIMCONNECT_RECV_ID_SYSTEM_STATE" << evt->szString;
        // "SimObjects/Airplanes/Asobo_A320_NEO/aircraft.cfg" on MSFS
        aircraftFilePath = QString::fromLocal8Bit(evt->szString);
        break;
      }
#endif

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
          DWORD objectID = pObjData->dwObjectID;
          SimData *simDataPtr = reinterpret_cast<SimData *>(&pObjData->dwData);
          simData = *simDataPtr;
          simDataObjectId = objectID;
          userDataFetched = true;
        }
        else if(pObjData->dwRequestID == DATA_REQUEST_ID_AI_AIRCRAFT ||
                pObjData->dwRequestID == DATA_REQUEST_ID_AI_HELICOPTER ||
                pObjData->dwRequestID == DATA_REQUEST_ID_AI_BOAT)
        {
          if(pObjData->dwObjectID > 0)
          {
            DWORD objectID = pObjData->dwObjectID;
            SimDataAircraft *simDataAircraftPtr = reinterpret_cast<SimDataAircraft *>(&pObjData->dwData);

            if(simDataAircraftPtr->userSim == 0 && // Do not add user aircraft to list
               SUCCEEDED(StringCbLengthA(&simDataAircraftPtr->aircraftTitle[0],
                                         sizeof(simDataAircraftPtr->aircraftTitle),
                                         NULL))) // security check
            {
              simDataAircraftMap.insert(objectID, *simDataAircraftPtr);
              aiDataFetched = true;
            }
          }
        }

        break;
      }
#if defined(SIMCONNECT_BUILD_WIN32)
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
#endif

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

void SimConnectHandlerPrivate::copyToSimConnectAircraft(const SimDataAircraft& simDataAircraft, SimConnectAircraft& aircraft)
{
#if defined(SIMCONNECT_BUILD_WIN32)
  aircraft.flags = atools::fs::sc::SIM_FSX_P3D;
#elif defined(SIMCONNECT_BUILD_WIN64)
  aircraft.flags = atools::fs::sc::SIM_MSFS;
#endif

  aircraft.airplaneTitle = simDataAircraft.aircraftTitle;
  aircraft.airplaneModel = simDataAircraft.aircraftAtcModel;
  aircraft.airplaneReg = simDataAircraft.aircraftAtcId;
  aircraft.airplaneType = simDataAircraft.aircraftAtcType;
  aircraft.airplaneAirline = simDataAircraft.aircraftAtcAirline;
  aircraft.airplaneFlightnumber = simDataAircraft.aircraftAtcFlightNumber;
  aircraft.fromIdent = simDataAircraft.aiFrom;
  aircraft.toIdent = simDataAircraft.aiTo;

  if(verbose)
    qDebug() << Q_FUNC_INFO
             << "airplaneTitle" << aircraft.airplaneTitle
             << "airplaneModel" << aircraft.airplaneModel
             << "airplaneReg" << aircraft.airplaneReg
             << "airplaneType" << aircraft.airplaneType
             << "airplaneAirline" << aircraft.airplaneAirline
             << "airplaneFlightnumber" << aircraft.airplaneFlightnumber;

#if defined(SIMCONNECT_BUILD_WIN64)
  // Add aircraft.cfg location as additional property for MSFS
  if(!aircraftFilePath.isEmpty())
    aircraft.properties.addProp(atools::util::Prop(atools::fs::sc::PROP_AIRCRAFT_CFG, aircraftFilePath));
#endif

  QString categoryStr = QString(simDataAircraft.category).toLower().trimmed();
  if(categoryStr == "airplane")
    aircraft.category = AIRPLANE;
  else if(categoryStr == "helicopter")
    aircraft.category = HELICOPTER;
  else if(categoryStr == "boat")
    aircraft.category = BOAT;
  else if(categoryStr == "groundvehicle")
    aircraft.category = GROUNDVEHICLE;
  else if(categoryStr == "controltower")
    aircraft.category = CONTROLTOWER;
  else if(categoryStr == "simpleobject")
    aircraft.category = SIMPLEOBJECT;
  else if(categoryStr == "viewer")
    aircraft.category = VIEWER;
  else
    aircraft.category = UNKNOWN;

  aircraft.wingSpanFt = static_cast<quint16>(simDataAircraft.wingSpan);
  aircraft.modelRadiusFt = static_cast<quint16>(simDataAircraft.modelRadius);

  aircraft.numberOfEngines = static_cast<quint8>(simDataAircraft.numEngines);
  aircraft.engineType = static_cast<EngineType>(simDataAircraft.engineType);

  aircraft.position.setLonX(static_cast<float>(simDataAircraft.longitudeDeg));
  aircraft.position.setLatY(static_cast<float>(simDataAircraft.latitudeDeg));
  aircraft.position.setAltitude(simDataAircraft.altitudeFt);

#if defined(SIMCONNECT_BUILD_WIN64)
  // Add more accurate coordinates as optional property
  aircraft.properties.addProp(atools::util::Prop(atools::fs::sc::PROP_AIRCRAFT_LONX, simDataAircraft.longitudeDeg));
  aircraft.properties.addProp(atools::util::Prop(atools::fs::sc::PROP_AIRCRAFT_LATY, simDataAircraft.latitudeDeg));
#endif

  aircraft.groundSpeedKts = simDataAircraft.groundVelocityKts;
  aircraft.indicatedAltitudeFt = simDataAircraft.indicatedAltitudeFt;
  aircraft.headingMagDeg = simDataAircraft.planeHeadingMagneticDeg;
  aircraft.headingTrueDeg = simDataAircraft.planeHeadingTrueDeg;

  aircraft.trueAirspeedKts = simDataAircraft.airspeedTrueKts;
  aircraft.indicatedSpeedKts = simDataAircraft.airspeedIndicatedKts;
  aircraft.machSpeed = simDataAircraft.airspeedMach;
  aircraft.verticalSpeedFeetPerMin = simDataAircraft.verticalSpeedFps * 60.f;

  aircraft.transponderCode = atools::fs::util::decodeTransponderCode(simDataAircraft.transponderCode);

  aircraft.flags.setFlag(atools::fs::sc::IS_USER, simDataAircraft.userSim > 0);
  aircraft.flags.setFlag(atools::fs::sc::SIM_PAUSED, simPaused > 0);
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
    if(paused)
      break;

    HRESULT hr = api->CallDispatch(dispatchFunction, this);

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

  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Magvar", "degrees", SIMCONNECT_DATATYPE_FLOAT32);

  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "GPS Ground Magnetic Track", "degrees", SIMCONNECT_DATATYPE_FLOAT32);
  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "GPS Ground True Track", "degrees", SIMCONNECT_DATATYPE_FLOAT32);

  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Plane Alt Above Ground", "feet", SIMCONNECT_DATATYPE_FLOAT32);
  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Ground Altitude", "feet", SIMCONNECT_DATATYPE_FLOAT32);

  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Autopilot Available", "bool", SIMCONNECT_DATATYPE_INT32);
  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Autopilot Altitude Lock Var", "feet", SIMCONNECT_DATATYPE_FLOAT32);

  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Ambient Temperature", "celsius", SIMCONNECT_DATATYPE_FLOAT32);

  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Total Air Temperature", "celsius", SIMCONNECT_DATATYPE_FLOAT32);

  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Ambient Wind Velocity", "knots", SIMCONNECT_DATATYPE_FLOAT32);

  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Ambient Wind Direction", "degrees", SIMCONNECT_DATATYPE_FLOAT32);

  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Ambient Precip State", "mask", SIMCONNECT_DATATYPE_INT32);
  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Ambient In Cloud", "bool", SIMCONNECT_DATATYPE_INT32);

  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Ambient Visibility", "meters", SIMCONNECT_DATATYPE_FLOAT32);

  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Sea Level Pressure", "millibars", SIMCONNECT_DATATYPE_FLOAT32);

  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Pitot Ice Pct", "percent", SIMCONNECT_DATATYPE_FLOAT32);
  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Structural Ice Pct", "percent", SIMCONNECT_DATATYPE_FLOAT32);

  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Total Weight", "pounds", SIMCONNECT_DATATYPE_FLOAT32);
  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Max Gross Weight", "pounds", SIMCONNECT_DATATYPE_FLOAT32);
  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Empty Weight", "pounds", SIMCONNECT_DATATYPE_FLOAT32);

  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Fuel Total Quantity", "gallons", SIMCONNECT_DATATYPE_FLOAT32);

  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Fuel Total Quantity Weight", "pounds", SIMCONNECT_DATATYPE_FLOAT32);

  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Eng Fuel Flow PPH:1", "Pounds per hour", SIMCONNECT_DATATYPE_FLOAT32);
  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Eng Fuel Flow PPH:2", "Pounds per hour", SIMCONNECT_DATATYPE_FLOAT32);
  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Eng Fuel Flow PPH:3", "Pounds per hour", SIMCONNECT_DATATYPE_FLOAT32);
  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Eng Fuel Flow PPH:4", "Pounds per hour", SIMCONNECT_DATATYPE_FLOAT32);

  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Eng Fuel Flow GPH:1", "Gallons per hour", SIMCONNECT_DATATYPE_FLOAT32);
  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Eng Fuel Flow GPH:2", "Gallons per hour", SIMCONNECT_DATATYPE_FLOAT32);
  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Eng Fuel Flow GPH:3", "Gallons per hour", SIMCONNECT_DATATYPE_FLOAT32);
  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Eng Fuel Flow GPH:4", "Gallons per hour", SIMCONNECT_DATATYPE_FLOAT32);

  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Local Time", "seconds", SIMCONNECT_DATATYPE_FLOAT32);
  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Local Year", "number", SIMCONNECT_DATATYPE_INT32);
  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Local Month of Year", "number", SIMCONNECT_DATATYPE_INT32);
  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Local Day of Month", "number", SIMCONNECT_DATATYPE_INT32);

  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Zulu Time", "seconds", SIMCONNECT_DATATYPE_FLOAT32);
  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Zulu Year", "number", SIMCONNECT_DATATYPE_INT32);
  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Zulu Month of Year", "number", SIMCONNECT_DATATYPE_INT32);
  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Zulu Day of Month", "number", SIMCONNECT_DATATYPE_INT32);

  // Measured in seconds, positive west of GMT.
  api->AddToDataDefinition(DATA_DEFINITION_USER_AIRCRAFT, "Time Zone Offset", "seconds", SIMCONNECT_DATATYPE_INT32);

  state = sc::STATEOK;
}

void SimConnectHandlerPrivate::fillDataDefinitionAicraft(DataDefinitionId definitionId)
{
  // Set up the data definition, but do not yet do anything with it
  api->AddToDataDefinition(definitionId, "Title", nullptr, SIMCONNECT_DATATYPE_STRING256);
  api->AddToDataDefinition(definitionId, "ATC Type", nullptr, SIMCONNECT_DATATYPE_STRING256);
  api->AddToDataDefinition(definitionId, "ATC Model", nullptr, SIMCONNECT_DATATYPE_STRING256);
  api->AddToDataDefinition(definitionId, "ATC Id", nullptr, SIMCONNECT_DATATYPE_STRING256);
  api->AddToDataDefinition(definitionId, "ATC Airline", nullptr, SIMCONNECT_DATATYPE_STRING256);
  api->AddToDataDefinition(definitionId, "ATC Flight Number", nullptr, SIMCONNECT_DATATYPE_STRING256);
  api->AddToDataDefinition(definitionId, "Category", nullptr, SIMCONNECT_DATATYPE_STRING256);

  api->AddToDataDefinition(definitionId, "Is User Sim", "bool", SIMCONNECT_DATATYPE_INT32);
  api->AddToDataDefinition(definitionId, "Visual Model Radius", "feet", SIMCONNECT_DATATYPE_INT32);
  api->AddToDataDefinition(definitionId, "Wing Span", "feet", SIMCONNECT_DATATYPE_INT32);

  api->AddToDataDefinition(definitionId, "Plane Altitude", "feet", SIMCONNECT_DATATYPE_FLOAT32);
  api->AddToDataDefinition(definitionId, "Plane Latitude", "degrees", SIMCONNECT_DATATYPE_FLOAT64);
  api->AddToDataDefinition(definitionId, "Plane Longitude", "degrees", SIMCONNECT_DATATYPE_FLOAT64);

  api->AddToDataDefinition(definitionId, "Ground Velocity", "knots", SIMCONNECT_DATATYPE_FLOAT32);
  api->AddToDataDefinition(definitionId, "Indicated Altitude", "feet", SIMCONNECT_DATATYPE_FLOAT32);

  api->AddToDataDefinition(definitionId, "Plane Heading Degrees Magnetic", "degrees", SIMCONNECT_DATATYPE_FLOAT32);
  api->AddToDataDefinition(definitionId, "Plane Heading Degrees True", "degrees", SIMCONNECT_DATATYPE_FLOAT32);

  api->AddToDataDefinition(definitionId, "Sim On Ground", "bool", SIMCONNECT_DATATYPE_INT32);

  api->AddToDataDefinition(definitionId, "Airspeed True", "knots", SIMCONNECT_DATATYPE_FLOAT32);
  api->AddToDataDefinition(definitionId, "Airspeed Indicated", "knots", SIMCONNECT_DATATYPE_FLOAT32);
  api->AddToDataDefinition(definitionId, "Airspeed Mach", "mach", SIMCONNECT_DATATYPE_FLOAT32);
  api->AddToDataDefinition(definitionId, "Vertical Speed", "feet per second", SIMCONNECT_DATATYPE_FLOAT32);

  api->AddToDataDefinition(definitionId, "Transponder Code:1", "number", SIMCONNECT_DATATYPE_INT32);

  api->AddToDataDefinition(definitionId, "Number of Engines", "number", SIMCONNECT_DATATYPE_INT32);

  api->AddToDataDefinition(definitionId, "Engine Type", "number", SIMCONNECT_DATATYPE_INT32);

  api->AddToDataDefinition(definitionId, "AI Traffic Fromairport", nullptr, SIMCONNECT_DATATYPE_STRING32);
  api->AddToDataDefinition(definitionId, "AI Traffic Toairport", nullptr, SIMCONNECT_DATATYPE_STRING32);
}

// ===============================================================================================
// SimConnectHandler
// ===============================================================================================

SimConnectHandler::SimConnectHandler(bool verboseLogging)
  : appName(QCoreApplication::applicationName().toLatin1())
{
  p = new SimConnectHandlerPrivate(verboseLogging);
}

SimConnectHandler::~SimConnectHandler()
{
  close();
  ATOOLS_DELETE_LOG(p);
}

void SimConnectHandler::close()
{
  HRESULT hr = p->api->Close();
  if(hr != S_OK)
    qWarning() << "Error closing SimConnect";
}

void SimConnectHandler::pauseSimConnect()
{
  qDebug() << Q_FUNC_INFO;
  p->paused = true;
  close();
}

void SimConnectHandler::resumeSimConnect()
{
  qDebug() << Q_FUNC_INFO;
  p->paused = false;
  connect();
}

void SimConnectHandler::releaseSimConnect()
{
  p->simConnectLoaded = false;
  p->activationContext->freeLibrary(p->libraryName);
  p->activationContext->deactivate();
  p->activationContext->release();
}

bool SimConnectHandler::loadSimConnect(atools::win::ActivationContext *activationContext, const QString& libraryName)
{
  p->simConnectLoaded = false;
  p->libraryName = libraryName;
  p->activationContext = activationContext;

  // Try local copy first
  QString simconnectDll = QCoreApplication::applicationDirPath() % QDir::separator() % p->libraryName;
  bool activated = false;
  if(!QFile::exists(simconnectDll))
  {
    // No local copy - load default from WinSxS
    if(!activationContext->create(QCoreApplication::applicationDirPath() % atools::SEP % "simconnect" % atools::SEP %
                                  "simconnect.manifest"))
      return false;

    if(!activationContext->activate())
      return false;

    p->libraryName = simconnectDll = "SimConnect.dll";
    activated = true;
  }

  if(!activationContext->loadLibrary(simconnectDll))
    return false;

  if(activated)
  {
    if(!activationContext->deactivate())
      return false;
  }

  if(!p->api->bindFunctions(activationContext, libraryName))
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
#if defined(SIMCONNECT_BUILD_WIN32)
  return true;

#else
  return false;

#endif
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
    qDebug() << "Before open";

  hr = p->api->Open(appName.constData(), nullptr, 0, nullptr, 0);
  if(hr == S_OK)
  {
    if(p->verbose)
      qDebug() << "Connected to Flight Simulator";

    p->fillDataDefinition();

    // Request an event when the simulation starts or pauses
    p->api->SubscribeToSystemEvent(EVENT_SIM_STATE, "Sim");
    p->api->SubscribeToSystemEvent(EVENT_SIM_PAUSE, "Pause");
    p->api->RequestSystemState(EVENT_AIRCRAFT_LOADED, "AircraftLoaded");

    p->state = sc::STATEOK;
    p->categoriesLogged.clear();

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
  if(p->verbose)
    qDebug() << "fetchData entered ================================================================";

  if(p->paused)
    return false;

  // === Get AI aircraft =======================================================
  p->simDataAircraftMap.clear();
  p->simDataObjectId = 0;

#if defined(SIMCONNECT_BUILD_WIN64)
  if(!p->lastSystemRequestTime.isValid() || p->lastSystemRequestTime.msecsTo(QDateTime::currentDateTime()) > 1000)
  {
    p->api->RequestSystemState(EVENT_AIRCRAFT_LOADED, "AircraftLoaded");
    p->lastSystemRequestTime = QDateTime::currentDateTime();
  }
#endif

  HRESULT hr = 0;

  if(options.testFlag(FETCH_AI_AIRCRAFT))
  {
    hr = p->api->RequestDataOnSimObjectType(DATA_REQUEST_ID_AI_AIRCRAFT, DATA_DEFINITION_AI_AIRCRAFT,
                                            static_cast<DWORD>(radiusKm) * 1000, SIMCONNECT_SIMOBJECT_TYPE_AIRCRAFT);
    if(!p->checkCall(hr, "DATA_REQUEST_ID_AI_AIRCRAFT"))
      return false;

    hr = p->api->RequestDataOnSimObjectType(DATA_REQUEST_ID_AI_HELICOPTER, DATA_DEFINITION_AI_HELICOPTER,
                                            static_cast<DWORD>(radiusKm) * 1000, SIMCONNECT_SIMOBJECT_TYPE_HELICOPTER);
    if(!p->checkCall(hr, "DATA_REQUEST_ID_AI_HELICOPTER"))
      return false;
  }

  if(options.testFlag(FETCH_AI_BOAT))
  {
    hr = p->api->RequestDataOnSimObjectType(DATA_REQUEST_ID_AI_BOAT, DATA_DEFINITION_AI_BOAT,
                                            static_cast<DWORD>(radiusKm) * 1000, SIMCONNECT_SIMOBJECT_TYPE_BOAT);
    if(!p->checkCall(hr, "DATA_REQUEST_ID_AI_BOAT"))
      return false;
  }

  p->callDispatch(p->aiDataFetched, "DATA_REQUEST_ID_AI_HELICOPTER, DATA_REQUEST_ID_AI_BOAT and DATA_REQUEST_ID_AI_AIRCRAFT");

  if(p->state == sc::STATEOK)
  {
    // === Get user aircraft =======================================================
    hr = p->api->RequestDataOnSimObjectType(DATA_REQUEST_ID_USER_AIRCRAFT, DATA_DEFINITION_USER_AIRCRAFT, 0,
                                            SIMCONNECT_SIMOBJECT_TYPE_USER);
    if(!p->checkCall(hr, "DATA_REQUEST_ID_USER_AIRCRAFT"))
      return false;

    p->callDispatch(p->userDataFetched, "DATA_REQUEST_ID_USER_AIRCRAFT");

    p->state = sc::STATEOK;

    // Get AI aircraft from hash =======================================================================
    for(auto it = p->simDataAircraftMap.constBegin(); it != p->simDataAircraftMap.constEnd(); ++it)
    {
      const SimDataAircraft& simDataAircraft = it.value();

      atools::fs::sc::SimConnectAircraft aiAircraft;
      p->copyToSimConnectAircraft(simDataAircraft, aiAircraft);

#if defined(SIMCONNECT_BUILD_WIN64)
      // MSFS ground flag is is unreliable for AI - try to detect by speed at least, AGL is not available for this
      aiAircraft.flags.setFlag(atools::fs::sc::ON_GROUND, simDataAircraft.isSimOnGround > 0 ||
                               (aiAircraft.verticalSpeedFeetPerMin < 0.01f && simDataAircraft.groundVelocityKts < 30.f));
#else
      // FSX and P3D
      aiAircraft.flags.setFlag(atools::fs::sc::ON_GROUND, simDataAircraft.isSimOnGround > 0);
#endif
      aiAircraft.objectId = static_cast<unsigned int>(it.key());

      // Add only aircraft, helicopters and ships in MSFS 2020 and 2024 to
      // avoid ground traffic which is wrongly delivered by the sim despite using the requests
      // SIMCONNECT_SIMOBJECT_TYPE_AIRCRAFT, SIMCONNECT_SIMOBJECT_TYPE_BOAT and SIMCONNECT_SIMOBJECT_TYPE_HELICOPTER
      // if(p->openData.dwApplicationVersionMajor != 12 || aiAircraft.isAnyFlying() || aiAircraft.isAnyBoat())
      // if(aiAircraft.isAnyFlying() || aiAircraft.isAnyBoat())
      data.aiAircraft.append(aiAircraft);

      if(!p->categoriesLogged.contains(simDataAircraft.category))
      {
        qInfo() << Q_FUNC_INFO << "Found new AI vehicle category" << QString(simDataAircraft.category)
                << "category" << aiAircraft.getCategory()
                << "isOnGround" << aiAircraft.isOnGround()
                << "airplaneTitle" << aiAircraft.airplaneTitle
                << "airplaneModel" << aiAircraft.airplaneModel
                << "airplaneReg" << aiAircraft.airplaneReg
                << "airplaneType" << aiAircraft.airplaneType
                << "airplaneAirline" << aiAircraft.airplaneAirline
                << "airplaneFlightnumber" << aiAircraft.airplaneFlightnumber;

        p->categoriesLogged.insert(simDataAircraft.category);
      }

    } // for(auto it = p->simDataAircraftMap.constBegin(); it != p->simDataAircraftMap.constEnd(); ++it)

    // Get user aircraft =======================================================================
    if(p->userDataFetched)
    {
      // Copy base data
      p->copyToSimConnectAircraft(p->simData.aircraft, data.userAircraft);

      data.userAircraft.flags.setFlag(atools::fs::sc::ON_GROUND, p->simData.aircraft.isSimOnGround > 0);

      // Copy additional user aircraft data
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
      data.userAircraft.fuelFlowPPH = p->simData.fuelFlowPph1 + p->simData.fuelFlowPph2 + p->simData.fuelFlowPph3 + p->simData.fuelFlowPph4;

      data.userAircraft.fuelFlowGPH = p->simData.fuelFlowGph1 + p->simData.fuelFlowGph2 + p->simData.fuelFlowGph3 + p->simData.fuelFlowGph4;

      data.userAircraft.windDirectionDegT = p->simData.ambientWindDirectionDegT;
      data.userAircraft.windSpeedKts = p->simData.ambientWindVelocityKts;

      // Build local time and use timezone offset from simulator
      QDate localDate(p->simData.localYear, p->simData.localMonth, p->simData.localDay);
      QTime localTime = QTime::fromMSecsSinceStartOfDay(atools::roundToInt(p->simData.localTimeSeconds * 1000.f));

      // Offset from FS: Measured in seconds, positive west of GMT.
      QDateTime localDateTime(localDate, localTime, Qt::OffsetFromUTC, -p->simData.timeZoneOffsetSeconds);
      data.userAircraft.localDateTime = localDateTime;

      QDate zuluDate(p->simData.zuluYear, p->simData.zuluMonth, p->simData.zuluDay);
      QTime zuluTime = QTime::fromMSecsSinceStartOfDay(atools::roundToInt(p->simData.zuluTimeSeconds * 1000.f));
      QDateTime zuluDateTime(zuluDate, zuluTime, Qt::UTC);
      data.userAircraft.zuluDateTime = zuluDateTime;
    } // if(p->userDataFetched)
    else
      data.userAircraft.position = atools::geo::Pos();
  } // if(p->state == sc::STATEOK)

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
#if defined(SIMCONNECT_BUILD_WIN32)
  if(p->paused)
    return false;

  if(p->weatherRequest.isValid())
  {
    p->fetchedMetars.clear();

    HRESULT hr;
    weather::Metar metar(p->weatherRequest.getStation(), p->weatherRequest.getPosition());

    if(!metar.getRequestIdent().isEmpty())
    {
      // == weather for station ========================================================
      hr = p->api->WeatherRequestObservationAtStation(DATA_REQUEST_ID_WEATHER_STATION, metar.getRequestIdent().toUtf8().constData());
      if(!p->checkCall(hr, "DATA_REQUEST_ID_WEATHER_STATION" + metar.getRequestIdent()))
        return false;

      p->fetchedMetars.clear();
      p->callDispatch(p->weatherDataFetched, "DATA_REQUEST_ID_WEATHER_STATION" + metar.getRequestIdent());

      if(p->fetchedMetars.size() > 1)
        qWarning() << "Got more than one metar for station" << metar.getRequestIdent() << ":" << p->fetchedMetars.size();

      if(!p->fetchedMetars.isEmpty())
        metar.setMetarForStation(p->fetchedMetars.constFirst());
    }

    if(p->fetchedMetars.isEmpty())
    {
      // Nothing found for station or no station given

      // == weather for nearest station ========================================================
      if(metar.getRequestPos().isValid())
      {
        hr = p->api->WeatherRequestObservationAtNearestStation(DATA_REQUEST_ID_WEATHER_NEAREST_STATION,
                                                               metar.getRequestPos().getLatY(), metar.getRequestPos().getLonX());
        if(!p->checkCall(hr, "DATA_REQUEST_ID_WEATHER_NEAREST_STATION" + metar.getRequestPos().toString()))
          return false;

        p->fetchedMetars.clear();
        p->callDispatch(p->weatherDataFetched, "DATA_REQUEST_ID_WEATHER_NEAREST_STATION" + metar.getRequestPos().toString());

        if(p->fetchedMetars.size() > 1)
          qWarning() << "Got more than one nearest metar for position" << metar.getRequestPos().toString() << ":"
                     << p->fetchedMetars.size();

        if(!p->fetchedMetars.isEmpty())
          metar.setMetarForNearest(p->fetchedMetars.constFirst());
      }

      // == interpolated weather ========================================================
      if(metar.getRequestPos().isValid())
      {
        hr = p->api->WeatherRequestInterpolatedObservation(DATA_REQUEST_ID_WEATHER_INTERPOLATED,
                                                           metar.getRequestPos().getLatY(), metar.getRequestPos().getLonX(),
                                                           metar.getRequestPos().getAltitude());
        if(!p->checkCall(hr, "DATA_REQUEST_ID_WEATHER_INTERPOLATED"))
          return false;

        p->fetchedMetars.clear();
        p->callDispatch(p->weatherDataFetched, "DATA_REQUEST_ID_WEATHER_INTERPOLATED");

        if(p->fetchedMetars.size() > 1)
          qWarning() << "Got more than one interpolated metar for position"
                     << metar.getRequestPos().toString() << ":" << p->fetchedMetars.size();

        if(!p->fetchedMetars.isEmpty())
          metar.setMetarForInterpolated(p->fetchedMetars.constFirst());
      }
    }
    data.metars.append(metar);
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

#else
  Q_UNUSED(data)
  return false;

#endif

}

void SimConnectHandler::addWeatherRequest(const atools::fs::sc::WeatherRequest& request)
{
  p->weatherRequest = request;
}

const WeatherRequest& SimConnectHandler::getWeatherRequest() const
{
  return p->weatherRequest;
}

QString SimConnectHandler::getName() const
{
  return QLatin1String("SimConnect");
}

bool SimConnectHandler::checkSimConnect() const
{
  HRESULT hr = p->api->Open(QCoreApplication::applicationName().toLatin1().constData(), nullptr, 0, nullptr, 0);
  if(hr == S_OK)
    p->api->Close();

  if(hr != S_OK)
    return false;
  return true;
}

} // namespace sc
} // namespace fs
} // namespace atools
