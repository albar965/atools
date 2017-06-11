/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_FS_SIMCONNECTPROCS_H
#define ATOOLS_FS_SIMCONNECTPROCS_H

#include <QString>

#if defined(Q_OS_WIN32)
// Use real SimConnect
extern "C" {
#include <windows.h>
#include <strsafe.h>
#include "SimConnect.h"
}
#else
#include "fs/sc/simconnectdummy.h"
#endif

namespace atools {
namespace win {
class ActivationContext;
}
}

namespace atools {
namespace fs {
namespace sc {

/*
 * Low level SimConnect wrapper that binds all functions dynamically.
 */
class SimConnectApi
{
public:
  SimConnectApi();
  ~SimConnectApi();

  bool bindFunctions(atools::win::ActivationContext& context);

  HRESULT Open(LPCSTR szName, HWND hWnd, DWORD UserEventWin32, HANDLE hEventHandle, DWORD ConfigIndex);
  HRESULT Close();

  HRESULT MapClientEventToSimEvent(SIMCONNECT_CLIENT_EVENT_ID EventID, const char *EventName = "");
  HRESULT TransmitClientEvent(SIMCONNECT_OBJECT_ID ObjectID, SIMCONNECT_CLIENT_EVENT_ID EventID, DWORD dwData,
                              SIMCONNECT_NOTIFICATION_GROUP_ID GroupID,
                              SIMCONNECT_EVENT_FLAG Flags);
  HRESULT SetSystemEventState(SIMCONNECT_CLIENT_EVENT_ID EventID,
                              SIMCONNECT_STATE dwState);
  HRESULT AddClientEventToNotificationGroup(SIMCONNECT_NOTIFICATION_GROUP_ID GroupID,
                                            SIMCONNECT_CLIENT_EVENT_ID EventID,
                                            BOOL bMaskable = FALSE);
  HRESULT RemoveClientEvent(SIMCONNECT_NOTIFICATION_GROUP_ID GroupID,
                            SIMCONNECT_CLIENT_EVENT_ID EventID);
  HRESULT SetNotificationGroupPriority(SIMCONNECT_NOTIFICATION_GROUP_ID GroupID,
                                       DWORD uPriority);
  HRESULT ClearNotificationGroup(SIMCONNECT_NOTIFICATION_GROUP_ID GroupID);
  HRESULT RequestNotificationGroup(SIMCONNECT_NOTIFICATION_GROUP_ID GroupID,
                                   DWORD dwReserved = 0,
                                   DWORD Flags = 0);
  HRESULT AddToDataDefinition(SIMCONNECT_DATA_DEFINITION_ID DefineID,
                              const char *DatumName, const char *UnitsName,
                              SIMCONNECT_DATATYPE DatumType = SIMCONNECT_DATATYPE_FLOAT64,
                              float fEpsilon = 0,
                              DWORD DatumID = SIMCONNECT_UNUSED);
  HRESULT ClearDataDefinition(SIMCONNECT_DATA_DEFINITION_ID DefineID);
  HRESULT RequestDataOnSimObject(SIMCONNECT_DATA_REQUEST_ID RequestID,
                                 SIMCONNECT_DATA_DEFINITION_ID DefineID,
                                 SIMCONNECT_OBJECT_ID ObjectID, SIMCONNECT_PERIOD Period,
                                 SIMCONNECT_DATA_REQUEST_FLAG Flags = 0, DWORD origin = 0,
                                 DWORD interval = 0,
                                 DWORD limit = 0);
  HRESULT RequestDataOnSimObjectType(SIMCONNECT_DATA_REQUEST_ID RequestID,
                                     SIMCONNECT_DATA_DEFINITION_ID DefineID,
                                     DWORD dwRadiusMeters,
                                     SIMCONNECT_SIMOBJECT_TYPE type);
  HRESULT SetDataOnSimObject(SIMCONNECT_DATA_DEFINITION_ID DefineID,
                             SIMCONNECT_OBJECT_ID ObjectID, SIMCONNECT_DATA_SET_FLAG Flags,
                             DWORD ArrayCount, DWORD cbUnitSize,
                             void *pDataSet);
  HRESULT MapInputEventToClientEvent(SIMCONNECT_INPUT_GROUP_ID GroupID, const char *szInputDefinition,
                                     SIMCONNECT_CLIENT_EVENT_ID DownEventID, DWORD DownValue =
                                       0, SIMCONNECT_CLIENT_EVENT_ID UpEventID =
                                       (SIMCONNECT_CLIENT_EVENT_ID)SIMCONNECT_UNUSED,
                                     DWORD UpValue = 0, BOOL bMaskable = FALSE);
  HRESULT SetInputGroupPriority(SIMCONNECT_INPUT_GROUP_ID GroupID,
                                DWORD uPriority);
  HRESULT RemoveInputEvent(SIMCONNECT_INPUT_GROUP_ID GroupID,
                           const char *szInputDefinition);
  HRESULT ClearInputGroup(SIMCONNECT_INPUT_GROUP_ID GroupID);
  HRESULT SetInputGroupState(SIMCONNECT_INPUT_GROUP_ID GroupID,
                             DWORD dwState);
  HRESULT RequestReservedKey(SIMCONNECT_CLIENT_EVENT_ID EventID,
                             const char *szKeyChoice1 = "", const char *szKeyChoice2 = "",
                             const char *szKeyChoice3 = "");
  HRESULT SubscribeToSystemEvent(SIMCONNECT_CLIENT_EVENT_ID EventID,
                                 const char *SystemEventName);
  HRESULT UnsubscribeFromSystemEvent(SIMCONNECT_CLIENT_EVENT_ID EventID);
  HRESULT WeatherRequestInterpolatedObservation(SIMCONNECT_DATA_REQUEST_ID RequestID,
                                                float lat, float lon,
                                                float alt);
  HRESULT WeatherRequestObservationAtStation(SIMCONNECT_DATA_REQUEST_ID RequestID,
                                             const char *szICAO);
  HRESULT WeatherRequestObservationAtNearestStation(SIMCONNECT_DATA_REQUEST_ID RequestID,
                                                    float lat,
                                                    float lon);
  HRESULT WeatherCreateStation(SIMCONNECT_DATA_REQUEST_ID RequestID,
                               const char *szICAO, const char *szName, float lat, float lon,
                               float alt);
  HRESULT WeatherRemoveStation(SIMCONNECT_DATA_REQUEST_ID RequestID,
                               const char *szICAO);
  HRESULT WeatherSetObservation(DWORD Seconds, const char *szMETAR);
  HRESULT WeatherSetModeServer(DWORD dwPort, DWORD dwSeconds);
  HRESULT WeatherSetModeTheme(const char *szThemeName);
  HRESULT WeatherSetModeGlobal();
  HRESULT WeatherSetModeCustom();
  HRESULT WeatherSetDynamicUpdateRate(DWORD dwRate);
  HRESULT WeatherRequestCloudState(SIMCONNECT_DATA_REQUEST_ID RequestID,
                                   float minLat, float minLon, float minAlt, float maxLat,
                                   float maxLon, float maxAlt,
                                   DWORD dwFlags = 0);
  HRESULT WeatherCreateThermal(SIMCONNECT_DATA_REQUEST_ID RequestID,
                               float lat, float lon, float alt, float radius, float height,
                               float coreRate = 3.0f, float coreTurbulence =
                                 0.05f, float sinkRate = 3.0f, float sinkTurbulence = 0.2f,
                               float coreSize = 0.4f, float coreTransitionSize =
                                 0.1f, float sinkLayerSize = 0.4f,
                               float sinkTransitionSize = 0.1f);
  HRESULT WeatherRemoveThermal(SIMCONNECT_OBJECT_ID ObjectID);
  HRESULT AICreateParkedATCAircraft(const char *szContainerTitle,
                                    const char *szTailNumber, const char *szAirportID,
                                    SIMCONNECT_DATA_REQUEST_ID RequestID);
  HRESULT AICreateEnrouteATCAircraft(const char *szContainerTitle,
                                     const char *szTailNumber, int iFlightNumber,
                                     const char *szFlightPlanPath, double dFlightPlanPosition,
                                     BOOL bTouchAndGo,
                                     SIMCONNECT_DATA_REQUEST_ID RequestID);
  HRESULT AICreateNonATCAircraft(const char *szContainerTitle,
                                 const char *szTailNumber,
                                 SIMCONNECT_DATA_INITPOSITION InitPos,
                                 SIMCONNECT_DATA_REQUEST_ID RequestID);
  HRESULT AICreateSimulatedObject(const char *szContainerTitle,
                                  SIMCONNECT_DATA_INITPOSITION InitPos,
                                  SIMCONNECT_DATA_REQUEST_ID RequestID);
  HRESULT AIReleaseControl(SIMCONNECT_OBJECT_ID ObjectID,
                           SIMCONNECT_DATA_REQUEST_ID RequestID);
  HRESULT AIRemoveObject(SIMCONNECT_OBJECT_ID ObjectID,
                         SIMCONNECT_DATA_REQUEST_ID RequestID);
  HRESULT AISetAircraftFlightPlan(SIMCONNECT_OBJECT_ID ObjectID,
                                  const char *szFlightPlanPath,
                                  SIMCONNECT_DATA_REQUEST_ID RequestID);
  HRESULT ExecuteMissionAction(const GUID guidInstanceId);
  HRESULT CompleteCustomMissionAction(const GUID guidInstanceId);
  HRESULT RetrieveString(SIMCONNECT_RECV *pData, DWORD cbData, void *pStringV, char **pszString, DWORD *pcbString);
  HRESULT GetLastSentPacketID(DWORD *pdwError);
  HRESULT CallDispatch(DispatchProc pfcnDispatch, void *pContext);
  HRESULT GetNextDispatch(SIMCONNECT_RECV **ppData, DWORD *pcbData);
  HRESULT RequestResponseTimes(DWORD nCount, float *fElapsedSeconds);
  HRESULT InsertString(char *pDest, DWORD cbDest, void **ppEnd, DWORD *pcbStringV, const char *pSource);
  HRESULT CameraSetRelative6DOF(float fDeltaX, float fDeltaY,
                                float fDeltaZ, float fPitchDeg, float fBankDeg,
                                float fHeadingDeg);
  HRESULT MenuAddItem(const char *szMenuItem, SIMCONNECT_CLIENT_EVENT_ID MenuEventID, DWORD dwData);
  HRESULT MenuDeleteItem(SIMCONNECT_CLIENT_EVENT_ID MenuEventID);
  HRESULT MenuAddSubItem(SIMCONNECT_CLIENT_EVENT_ID MenuEventID,
                         const char *szMenuItem, SIMCONNECT_CLIENT_EVENT_ID SubMenuEventID,
                         DWORD dwData);
  HRESULT MenuDeleteSubItem(SIMCONNECT_CLIENT_EVENT_ID MenuEventID, const SIMCONNECT_CLIENT_EVENT_ID SubMenuEventID);
  HRESULT RequestSystemState(SIMCONNECT_DATA_REQUEST_ID RequestID, const char *szState);
  HRESULT SetSystemState(const char *szState, DWORD dwInteger, float fFloat, const char *szString);
  HRESULT MapClientDataNameToID(const char *szClientDataName, SIMCONNECT_CLIENT_DATA_ID ClientDataID);
  HRESULT CreateClientData(SIMCONNECT_CLIENT_DATA_ID ClientDataID, DWORD dwSize,
                           SIMCONNECT_CREATE_CLIENT_DATA_FLAG Flags);
  HRESULT AddToClientDataDefinition(SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID,
                                    DWORD dwOffset, DWORD dwSizeOrType, float fEpsilon = 0,
                                    DWORD DatumID = SIMCONNECT_UNUSED);
  HRESULT ClearClientDataDefinition(SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID);
  HRESULT RequestClientData(SIMCONNECT_CLIENT_DATA_ID ClientDataID,
                            SIMCONNECT_DATA_REQUEST_ID RequestID,
                            SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID,
                            SIMCONNECT_CLIENT_DATA_PERIOD Period =
                              SIMCONNECT_CLIENT_DATA_PERIOD_ONCE,
                            SIMCONNECT_CLIENT_DATA_REQUEST_FLAG Flags = 0, DWORD origin = 0,
                            DWORD interval = 0,
                            DWORD limit = 0);
  HRESULT SetClientData(SIMCONNECT_CLIENT_DATA_ID ClientDataID,
                        SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID,
                        SIMCONNECT_CLIENT_DATA_SET_FLAG Flags, DWORD dwReserved,
                        DWORD cbUnitSize,
                        void *pDataSet);
  HRESULT FlightLoad(const char *szFileName);
  HRESULT FlightSave(const char *szFileName, const char *szTitle,
                     const char *szDescription,
                     DWORD Flags);
  HRESULT FlightPlanLoad(const char *szFileName);
  HRESULT Text(SIMCONNECT_TEXT_TYPE type, float fTimeSeconds,
               SIMCONNECT_CLIENT_EVENT_ID EventID, DWORD cbUnitSize,
               void *pDataSet);
  HRESULT SubscribeToFacilities(SIMCONNECT_FACILITY_LIST_TYPE type,
                                SIMCONNECT_DATA_REQUEST_ID RequestID);
  HRESULT UnsubscribeToFacilities(SIMCONNECT_FACILITY_LIST_TYPE type);
  HRESULT RequestFacilitiesList(SIMCONNECT_FACILITY_LIST_TYPE type,
                                SIMCONNECT_DATA_REQUEST_ID RequestID);

private:
  // ==============================================================================================
  // ==============================================================================================
  // Function pointers

  HRESULT (_stdcall *SC_MapClientEventToSimEvent)(HANDLE hSimConnect, SIMCONNECT_CLIENT_EVENT_ID EventID,
                                                  const char *EventName) = nullptr;
  HRESULT (_stdcall *SC_TransmitClientEvent)(HANDLE hSimConnect, SIMCONNECT_OBJECT_ID ObjectID,
                                             SIMCONNECT_CLIENT_EVENT_ID EventID, DWORD dwData,
                                             SIMCONNECT_NOTIFICATION_GROUP_ID GroupID,
                                             SIMCONNECT_EVENT_FLAG Flags) = nullptr;
  HRESULT (_stdcall *SC_SetSystemEventState)(HANDLE hSimConnect, SIMCONNECT_CLIENT_EVENT_ID EventID,
                                             SIMCONNECT_STATE dwState) = nullptr;
  HRESULT (_stdcall *SC_AddClientEventToNotificationGroup)(HANDLE hSimConnect,
                                                           SIMCONNECT_NOTIFICATION_GROUP_ID GroupID,
                                                           SIMCONNECT_CLIENT_EVENT_ID EventID,
                                                           BOOL bMaskable) = nullptr;
  HRESULT (_stdcall *SC_RemoveClientEvent)(HANDLE hSimConnect, SIMCONNECT_NOTIFICATION_GROUP_ID GroupID,
                                           SIMCONNECT_CLIENT_EVENT_ID EventID) = nullptr;
  HRESULT (_stdcall *SC_SetNotificationGroupPriority)(HANDLE hSimConnect,
                                                      SIMCONNECT_NOTIFICATION_GROUP_ID GroupID,
                                                      DWORD uPriority) = nullptr;
  HRESULT (_stdcall *SC_ClearNotificationGroup)(HANDLE hSimConnect, SIMCONNECT_NOTIFICATION_GROUP_ID GroupID) = nullptr;
  HRESULT (_stdcall *SC_RequestNotificationGroup)(HANDLE hSimConnect,
                                                  SIMCONNECT_NOTIFICATION_GROUP_ID GroupID,
                                                  DWORD dwReserved,
                                                  DWORD Flags) = nullptr;
  HRESULT (_stdcall *SC_AddToDataDefinition)(HANDLE hSimConnect, SIMCONNECT_DATA_DEFINITION_ID DefineID,
                                             const char *DatumName, const char *UnitsName,
                                             SIMCONNECT_DATATYPE DatumType,
                                             float fEpsilon,
                                             DWORD DatumID) = nullptr;
  HRESULT (_stdcall *SC_ClearDataDefinition)(HANDLE hSimConnect, SIMCONNECT_DATA_DEFINITION_ID DefineID) = nullptr;
  HRESULT (_stdcall *SC_RequestDataOnSimObject)(HANDLE hSimConnect, SIMCONNECT_DATA_REQUEST_ID RequestID,
                                                SIMCONNECT_DATA_DEFINITION_ID DefineID,
                                                SIMCONNECT_OBJECT_ID ObjectID, SIMCONNECT_PERIOD Period,
                                                SIMCONNECT_DATA_REQUEST_FLAG Flags, DWORD origin,
                                                DWORD interval,
                                                DWORD limit) = nullptr;
  HRESULT (_stdcall *SC_RequestDataOnSimObjectType)(HANDLE hSimConnect, SIMCONNECT_DATA_REQUEST_ID RequestID,
                                                    SIMCONNECT_DATA_DEFINITION_ID DefineID,
                                                    DWORD dwRadiusMeters,
                                                    SIMCONNECT_SIMOBJECT_TYPE type) = nullptr;
  HRESULT (_stdcall *SC_SetDataOnSimObject)(HANDLE hSimConnect, SIMCONNECT_DATA_DEFINITION_ID DefineID,
                                            SIMCONNECT_OBJECT_ID ObjectID, SIMCONNECT_DATA_SET_FLAG Flags,
                                            DWORD ArrayCount, DWORD cbUnitSize,
                                            void *pDataSet) = nullptr;
  HRESULT (_stdcall *SC_MapInputEventToClientEvent)(HANDLE hSimConnect, SIMCONNECT_INPUT_GROUP_ID GroupID,
                                                    const char *szInputDefinition,
                                                    SIMCONNECT_CLIENT_EVENT_ID DownEventID, DWORD DownValue,
                                                    SIMCONNECT_CLIENT_EVENT_ID UpEventID,
                                                    DWORD UpValue, BOOL bMaskable) = nullptr;
  HRESULT (_stdcall *SC_SetInputGroupPriority)(HANDLE hSimConnect, SIMCONNECT_INPUT_GROUP_ID GroupID,
                                               DWORD uPriority) = nullptr;
  HRESULT (_stdcall *SC_RemoveInputEvent)(HANDLE hSimConnect, SIMCONNECT_INPUT_GROUP_ID GroupID,
                                          const char *szInputDefinition) = nullptr;
  HRESULT (_stdcall *SC_ClearInputGroup)(HANDLE hSimConnect, SIMCONNECT_INPUT_GROUP_ID GroupID) = nullptr;
  HRESULT (_stdcall *SC_SetInputGroupState)(HANDLE hSimConnect, SIMCONNECT_INPUT_GROUP_ID GroupID,
                                            DWORD dwState) = nullptr;
  HRESULT (_stdcall *SC_RequestReservedKey)(HANDLE hSimConnect, SIMCONNECT_CLIENT_EVENT_ID EventID,
                                            const char *szKeyChoice1, const char *szKeyChoice2,
                                            const char *szKeyChoice3) = nullptr;
  HRESULT (_stdcall *SC_SubscribeToSystemEvent)(HANDLE hSimConnect, SIMCONNECT_CLIENT_EVENT_ID EventID,
                                                const char *SystemEventName) = nullptr;
  HRESULT (_stdcall *SC_UnsubscribeFromSystemEvent)(HANDLE hSimConnect, SIMCONNECT_CLIENT_EVENT_ID EventID) = nullptr;
  HRESULT (_stdcall *SC_WeatherRequestInterpolatedObservation)(HANDLE hSimConnect,
                                                               SIMCONNECT_DATA_REQUEST_ID RequestID,
                                                               float lat, float lon,
                                                               float alt) = nullptr;
  HRESULT (_stdcall *SC_WeatherRequestObservationAtStation)(HANDLE hSimConnect,
                                                            SIMCONNECT_DATA_REQUEST_ID RequestID,
                                                            const char *szICAO) = nullptr;
  HRESULT (_stdcall *SC_WeatherRequestObservationAtNearestStation)(HANDLE hSimConnect,
                                                                   SIMCONNECT_DATA_REQUEST_ID RequestID,
                                                                   float lat,
                                                                   float lon) = nullptr;
  HRESULT (_stdcall *SC_WeatherCreateStation)(HANDLE hSimConnect, SIMCONNECT_DATA_REQUEST_ID RequestID,
                                              const char *szICAO, const char *szName, float lat, float lon,
                                              float alt) = nullptr;
  HRESULT (_stdcall *SC_WeatherRemoveStation)(HANDLE hSimConnect, SIMCONNECT_DATA_REQUEST_ID RequestID,
                                              const char *szICAO) = nullptr;
  HRESULT (_stdcall *SC_WeatherSetObservation)(HANDLE hSimConnect, DWORD Seconds, const char *szMETAR) = nullptr;
  HRESULT (_stdcall *SC_WeatherSetModeServer)(HANDLE hSimConnect, DWORD dwPort, DWORD dwSeconds) = nullptr;
  HRESULT (_stdcall *SC_WeatherSetModeTheme)(HANDLE hSimConnect, const char *szThemeName) = nullptr;
  HRESULT (_stdcall *SC_WeatherSetModeGlobal)(HANDLE hSimConnect) = nullptr;
  HRESULT (_stdcall *SC_WeatherSetModeCustom)(HANDLE hSimConnect) = nullptr;
  HRESULT (_stdcall *SC_WeatherSetDynamicUpdateRate)(HANDLE hSimConnect, DWORD dwRate) = nullptr;
  HRESULT (_stdcall *SC_WeatherRequestCloudState)(HANDLE hSimConnect, SIMCONNECT_DATA_REQUEST_ID RequestID,
                                                  float minLat, float minLon, float minAlt, float maxLat,
                                                  float maxLon, float maxAlt,
                                                  DWORD dwFlags) = nullptr;
  HRESULT (_stdcall *SC_WeatherCreateThermal)(HANDLE hSimConnect, SIMCONNECT_DATA_REQUEST_ID RequestID,
                                              float lat, float lon, float alt, float radius, float height,
                                              float coreRate, float coreTurbulence, float sinkRate,
                                              float sinkTurbulence,
                                              float coreSize, float coreTransitionSize, float sinkLayerSize,
                                              float sinkTransitionSize) = nullptr;
  HRESULT (_stdcall *SC_WeatherRemoveThermal)(HANDLE hSimConnect, SIMCONNECT_OBJECT_ID ObjectID) = nullptr;
  HRESULT (_stdcall *SC_AICreateParkedATCAircraft)(HANDLE hSimConnect, const char *szContainerTitle,
                                                   const char *szTailNumber, const char *szAirportID,
                                                   SIMCONNECT_DATA_REQUEST_ID RequestID) = nullptr;
  HRESULT (_stdcall *SC_AICreateEnrouteATCAircraft)(HANDLE hSimConnect, const char *szContainerTitle,
                                                    const char *szTailNumber, int iFlightNumber,
                                                    const char *szFlightPlanPath, double dFlightPlanPosition,
                                                    BOOL bTouchAndGo,
                                                    SIMCONNECT_DATA_REQUEST_ID RequestID) = nullptr;
  HRESULT (_stdcall *SC_AICreateNonATCAircraft)(HANDLE hSimConnect, const char *szContainerTitle,
                                                const char *szTailNumber,
                                                SIMCONNECT_DATA_INITPOSITION InitPos,
                                                SIMCONNECT_DATA_REQUEST_ID RequestID) = nullptr;
  HRESULT (_stdcall *SC_AICreateSimulatedObject)(HANDLE hSimConnect, const char *szContainerTitle,
                                                 SIMCONNECT_DATA_INITPOSITION InitPos,
                                                 SIMCONNECT_DATA_REQUEST_ID RequestID) = nullptr;
  HRESULT (_stdcall *SC_AIReleaseControl)(HANDLE hSimConnect, SIMCONNECT_OBJECT_ID ObjectID,
                                          SIMCONNECT_DATA_REQUEST_ID RequestID) = nullptr;
  HRESULT (_stdcall *SC_AIRemoveObject)(HANDLE hSimConnect, SIMCONNECT_OBJECT_ID ObjectID,
                                        SIMCONNECT_DATA_REQUEST_ID RequestID) = nullptr;
  HRESULT (_stdcall *SC_AISetAircraftFlightPlan)(HANDLE hSimConnect, SIMCONNECT_OBJECT_ID ObjectID,
                                                 const char *szFlightPlanPath,
                                                 SIMCONNECT_DATA_REQUEST_ID RequestID) = nullptr;
  HRESULT (_stdcall *SC_ExecuteMissionAction)(HANDLE hSimConnect, const GUID guidInstanceId) = nullptr;
  HRESULT (_stdcall *SC_CompleteCustomMissionAction)(HANDLE hSimConnect, const GUID guidInstanceId) = nullptr;
  HRESULT (_stdcall *SC_Close)(HANDLE hSimConnect) = nullptr;
  HRESULT (_stdcall *SC_RetrieveString)(SIMCONNECT_RECV *pData, DWORD cbData, void *pStringV,
                                        char **pszString,
                                        DWORD *pcbString) = nullptr;
  HRESULT (_stdcall *SC_GetLastSentPacketID)(HANDLE hSimConnect, DWORD *pdwError) = nullptr;
  HRESULT (_stdcall *SC_Open)(HANDLE *phSimConnect, LPCSTR szName, HWND hWnd, DWORD UserEventWin32,
                              HANDLE hEventHandle,
                              DWORD ConfigIndex) = nullptr;
  HRESULT (_stdcall *SC_CallDispatch)(HANDLE hSimConnect, DispatchProc pfcnDispatch, void *pContext) = nullptr;
  HRESULT (_stdcall *SC_GetNextDispatch)(HANDLE hSimConnect, SIMCONNECT_RECV **ppData, DWORD *pcbData) = nullptr;
  HRESULT (_stdcall *SC_RequestResponseTimes)(HANDLE hSimConnect, DWORD nCount, float *fElapsedSeconds) = nullptr;
  HRESULT (_stdcall *SC_InsertString)(char *pDest, DWORD cbDest, void **ppEnd, DWORD *pcbStringV,
                                      const char *pSource) = nullptr;
  HRESULT (_stdcall *SC_CameraSetRelative6DOF)(HANDLE hSimConnect, float fDeltaX, float fDeltaY,
                                               float fDeltaZ, float fPitchDeg, float fBankDeg,
                                               float fHeadingDeg) = nullptr;
  HRESULT (_stdcall *SC_MenuAddItem)(HANDLE hSimConnect, const char *szMenuItem,
                                     SIMCONNECT_CLIENT_EVENT_ID MenuEventID,
                                     DWORD dwData) = nullptr;
  HRESULT (_stdcall *SC_MenuDeleteItem)(HANDLE hSimConnect, SIMCONNECT_CLIENT_EVENT_ID MenuEventID) = nullptr;
  HRESULT (_stdcall *SC_MenuAddSubItem)(HANDLE hSimConnect, SIMCONNECT_CLIENT_EVENT_ID MenuEventID,
                                        const char *szMenuItem, SIMCONNECT_CLIENT_EVENT_ID SubMenuEventID,
                                        DWORD dwData) = nullptr;
  HRESULT (_stdcall *SC_MenuDeleteSubItem)(HANDLE hSimConnect, SIMCONNECT_CLIENT_EVENT_ID MenuEventID,
                                           const SIMCONNECT_CLIENT_EVENT_ID SubMenuEventID) = nullptr;
  HRESULT (_stdcall *SC_RequestSystemState)(HANDLE hSimConnect, SIMCONNECT_DATA_REQUEST_ID RequestID,
                                            const char *szState) = nullptr;
  HRESULT (_stdcall *SC_SetSystemState)(HANDLE hSimConnect, const char *szState, DWORD dwInteger,
                                        float fFloat,
                                        const char *szString) = nullptr;
  HRESULT (_stdcall *SC_MapClientDataNameToID)(HANDLE hSimConnect, const char *szClientDataName,
                                               SIMCONNECT_CLIENT_DATA_ID ClientDataID) = nullptr;
  HRESULT (_stdcall *SC_CreateClientData)(HANDLE hSimConnect, SIMCONNECT_CLIENT_DATA_ID ClientDataID,
                                          DWORD dwSize,
                                          SIMCONNECT_CREATE_CLIENT_DATA_FLAG Flags) = nullptr;
  HRESULT (_stdcall *SC_AddToClientDataDefinition)(HANDLE hSimConnect,
                                                   SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID,
                                                   DWORD dwOffset, DWORD dwSizeOrType, float fEpsilon,
                                                   DWORD DatumID) = nullptr;
  HRESULT (_stdcall *SC_ClearClientDataDefinition)(HANDLE hSimConnect,
                                                   SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID) = nullptr;
  HRESULT (_stdcall *SC_RequestClientData)(HANDLE hSimConnect, SIMCONNECT_CLIENT_DATA_ID ClientDataID,
                                           SIMCONNECT_DATA_REQUEST_ID RequestID,
                                           SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID,
                                           SIMCONNECT_CLIENT_DATA_PERIOD Period,
                                           SIMCONNECT_CLIENT_DATA_REQUEST_FLAG Flags, DWORD origin,
                                           DWORD interval, DWORD limit) = nullptr;
  HRESULT (_stdcall *SC_SetClientData)(HANDLE hSimConnect, SIMCONNECT_CLIENT_DATA_ID ClientDataID,
                                       SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID,
                                       SIMCONNECT_CLIENT_DATA_SET_FLAG Flags, DWORD dwReserved,
                                       DWORD cbUnitSize,
                                       void *pDataSet) = nullptr;
  HRESULT (_stdcall *SC_FlightLoad)(HANDLE hSimConnect, const char *szFileName) = nullptr;
  HRESULT (_stdcall *SC_FlightSave)(HANDLE hSimConnect, const char *szFileName, const char *szTitle,
                                    const char *szDescription,
                                    DWORD Flags) = nullptr;
  HRESULT (_stdcall *SC_FlightPlanLoad)(HANDLE hSimConnect, const char *szFileName) = nullptr;
  HRESULT (_stdcall *SC_Text)(HANDLE hSimConnect, SIMCONNECT_TEXT_TYPE type, float fTimeSeconds,
                              SIMCONNECT_CLIENT_EVENT_ID EventID, DWORD cbUnitSize,
                              void *pDataSet) = nullptr;
  HRESULT (_stdcall *SC_SubscribeToFacilities)(HANDLE hSimConnect, SIMCONNECT_FACILITY_LIST_TYPE type,
                                               SIMCONNECT_DATA_REQUEST_ID RequestID) = nullptr;
  HRESULT (_stdcall *SC_UnsubscribeToFacilities)(HANDLE hSimConnect, SIMCONNECT_FACILITY_LIST_TYPE type) = nullptr;
  HRESULT (_stdcall *SC_RequestFacilitiesList)(HANDLE hSimConnect, SIMCONNECT_FACILITY_LIST_TYPE type,
                                               SIMCONNECT_DATA_REQUEST_ID RequestID) = nullptr;

  HANDLE hSimConnect = NULL;
};

} // namespace sc
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_SIMCONNECTPROCS_H
