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

#ifndef ATOOLS_FS_SIMCONNECTHANDLER_H
#define ATOOLS_FS_SIMCONNECTHANDLER_H

#include <QtGlobal>
#include <QVector>

#if defined(SIMCONNECT_DUMMY)
#include "fs/sc/simconnectdummy.h"
#else

#if defined(Q_CC_MSVC)
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
#include "SimConnect.h"
#else
#define SIMCONNECT_DUMMY
#include "fs/sc/simconnectdummy.h"
#endif

#endif

namespace atools {
namespace fs {
namespace sc {

class SimConnectData;
class SimConnectAircraft;

/* Status of the last operation when fetching data. */
enum State
{
  STATEOK,
  FETCH_ERROR,
  OPEN_ERROR,
  DISCONNECTED,
  SIMCONNECT_EXCEPTION
};

/* Reads data synchronously from Fs simconnect interfaces.
 *  For non windows platforms contains also a simple aircraft simulation. */
class SimConnectHandler
{
public:
  SimConnectHandler(bool verboseLogging = false);
  virtual ~SimConnectHandler();

  /* Connect to fs.. Returns true it successful. */
  bool connect();

  /* Fetch data from simulator. Returns false if no data was retrieved due to paused or not running fs. */
  bool fetchData(atools::fs::sc::SimConnectData& data, int radiusKm);

  /* true if simulator is running and not stuck in open dialogs. */
  bool isSimRunning() const
  {
    return simRunning;
  }

  bool isSimPaused() const
  {
    return simPaused;
  }

  /* Get state of last call. */
  sc::State getState() const
  {
    return state;
  }

private:
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

#if defined(Q_OS_WIN32) || defined(SIMCONNECT_DUMMY)
  /* Callback receiving the data. */
  void dispatchProcedure(SIMCONNECT_RECV *pData, DWORD cbData);

  /* Static method will pass call to object which is passed in pContext. */
  static void CALLBACK dispatchCallback(SIMCONNECT_RECV *pData, DWORD cbData, void *pContext);

  void fillDataDefinitionAicraft(DataDefinitionId definitionId);
  void copyToSimData(const SimDataAircraft& simDataUserAircraft, atools::fs::sc::SimConnectAircraft& airplane);

  HANDLE hSimConnect = NULL;
#endif

  SimData simData;
  QVector<SimDataAircraft> simDataAircraft;

  bool simRunning = true, simPaused = false, verbose = false, dataFetched = false, userDataFetched = false;
  sc::State state = sc::STATEOK;

};

} // namespace sc
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_SIMCONNECTHANDLER_H
