/*****************************************************************************
* Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY{return E_FAIL;} without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#include "fs/sc/simconnectdummy.h"

#if !defined(Q_OS_WIN32)

#include <QDebug>

#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wunused-parameter"

HRESULT StringCbLengthA(const char *, size_t cbMax, size_t *pcb)
{
  static HRESULT hr = E_FAIL;
  return hr;
}

SIMCONNECTAPI SimConnect_MapClientEventToSimEvent(HANDLE hSimConnect, SIMCONNECT_CLIENT_EVENT_ID EventID,
                                                  const char *EventName)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_TransmitClientEvent(HANDLE hSimConnect, SIMCONNECT_OBJECT_ID ObjectID,
                                             SIMCONNECT_CLIENT_EVENT_ID EventID, DWORD dwData,
                                             SIMCONNECT_NOTIFICATION_GROUP_ID GroupID,
                                             SIMCONNECT_EVENT_FLAG Flags)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_SetSystemEventState(HANDLE hSimConnect, SIMCONNECT_CLIENT_EVENT_ID EventID,
                                             SIMCONNECT_STATE dwState)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_AddClientEventToNotificationGroup(HANDLE hSimConnect,
                                                           SIMCONNECT_NOTIFICATION_GROUP_ID GroupID,
                                                           SIMCONNECT_CLIENT_EVENT_ID EventID,
                                                           BOOL bMaskable)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_RemoveClientEvent(HANDLE hSimConnect, SIMCONNECT_NOTIFICATION_GROUP_ID GroupID,
                                           SIMCONNECT_CLIENT_EVENT_ID EventID)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_SetNotificationGroupPriority(HANDLE hSimConnect,
                                                      SIMCONNECT_NOTIFICATION_GROUP_ID GroupID,
                                                      DWORD uPriority)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_ClearNotificationGroup(HANDLE hSimConnect, SIMCONNECT_NOTIFICATION_GROUP_ID GroupID)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_RequestNotificationGroup(HANDLE hSimConnect,
                                                  SIMCONNECT_NOTIFICATION_GROUP_ID GroupID,
                                                  DWORD dwReserved,
                                                  DWORD Flags)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_AddToDataDefinition(HANDLE hSimConnect, SIMCONNECT_DATA_DEFINITION_ID DefineID,
                                             const char *DatumName, const char *UnitsName,
                                             SIMCONNECT_DATATYPE DatumType,
                                             float fEpsilon,
                                             DWORD DatumID)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_ClearDataDefinition(HANDLE hSimConnect, SIMCONNECT_DATA_DEFINITION_ID DefineID)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_RequestDataOnSimObject(HANDLE hSimConnect, SIMCONNECT_DATA_REQUEST_ID RequestID,
                                                SIMCONNECT_DATA_DEFINITION_ID DefineID,
                                                SIMCONNECT_OBJECT_ID ObjectID, SIMCONNECT_PERIOD Period,
                                                SIMCONNECT_DATA_REQUEST_FLAG Flags, DWORD origin,
                                                DWORD interval,
                                                DWORD limit)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_RequestDataOnSimObjectType(HANDLE hSimConnect, SIMCONNECT_DATA_REQUEST_ID RequestID,
                                                    SIMCONNECT_DATA_DEFINITION_ID DefineID,
                                                    DWORD dwRadiusMeters,
                                                    SIMCONNECT_SIMOBJECT_TYPE type)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_SetDataOnSimObject(HANDLE hSimConnect, SIMCONNECT_DATA_DEFINITION_ID DefineID,
                                            SIMCONNECT_OBJECT_ID ObjectID, SIMCONNECT_DATA_SET_FLAG Flags,
                                            DWORD ArrayCount, DWORD cbUnitSize,
                                            void *pDataSet)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_MapInputEventToClientEvent(HANDLE hSimConnect, SIMCONNECT_INPUT_GROUP_ID GroupID,
                                                    const char *szInputDefinition,
                                                    SIMCONNECT_CLIENT_EVENT_ID DownEventID, DWORD DownValue,
                                                    SIMCONNECT_CLIENT_EVENT_ID UpEventID,
                                                    DWORD UpValue,
                                                    BOOL bMaskable)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_SetInputGroupPriority(HANDLE hSimConnect, SIMCONNECT_INPUT_GROUP_ID GroupID,
                                               DWORD uPriority)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_RemoveInputEvent(HANDLE hSimConnect, SIMCONNECT_INPUT_GROUP_ID GroupID,
                                          const char *szInputDefinition)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_ClearInputGroup(HANDLE hSimConnect, SIMCONNECT_INPUT_GROUP_ID GroupID)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_SetInputGroupState(HANDLE hSimConnect, SIMCONNECT_INPUT_GROUP_ID GroupID,
                                            DWORD dwState)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_RequestReservedKey(HANDLE hSimConnect, SIMCONNECT_CLIENT_EVENT_ID EventID,
                                            const char *szKeyChoice1, const char *szKeyChoice2,
                                            const char *szKeyChoice3)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_SubscribeToSystemEvent(HANDLE hSimConnect, SIMCONNECT_CLIENT_EVENT_ID EventID,
                                                const char *SystemEventName)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_UnsubscribeFromSystemEvent(HANDLE hSimConnect, SIMCONNECT_CLIENT_EVENT_ID EventID)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_WeatherRequestInterpolatedObservation(HANDLE hSimConnect,
                                                               SIMCONNECT_DATA_REQUEST_ID RequestID,
                                                               float lat, float lon,
                                                               float alt)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_WeatherRequestObservationAtStation(HANDLE hSimConnect,
                                                            SIMCONNECT_DATA_REQUEST_ID RequestID,
                                                            const char *szICAO)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_WeatherRequestObservationAtNearestStation(HANDLE hSimConnect,
                                                                   SIMCONNECT_DATA_REQUEST_ID RequestID,
                                                                   float lat,
                                                                   float lon)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_WeatherCreateStation(HANDLE hSimConnect, SIMCONNECT_DATA_REQUEST_ID RequestID,
                                              const char *szICAO, const char *szName, float lat, float lon,
                                              float alt)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_WeatherRemoveStation(HANDLE hSimConnect, SIMCONNECT_DATA_REQUEST_ID RequestID,
                                              const char *szICAO)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_WeatherSetObservation(HANDLE hSimConnect, DWORD Seconds, const char *szMETAR)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_WeatherSetModeServer(HANDLE hSimConnect, DWORD dwPort, DWORD dwSeconds)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_WeatherSetModeTheme(HANDLE hSimConnect, const char *szThemeName)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_WeatherSetModeGlobal(HANDLE hSimConnect)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_WeatherSetModeCustom(HANDLE hSimConnect)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_WeatherSetDynamicUpdateRate(HANDLE hSimConnect, DWORD dwRate)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_WeatherRequestCloudState(HANDLE hSimConnect, SIMCONNECT_DATA_REQUEST_ID RequestID,
                                                  float minLat, float minLon, float minAlt, float maxLat,
                                                  float maxLon, float maxAlt,
                                                  DWORD dwFlags)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_WeatherCreateThermal(HANDLE hSimConnect, SIMCONNECT_DATA_REQUEST_ID RequestID,
                                              float lat, float lon, float alt, float radius, float height,
                                              float coreRate, float coreTurbulence, float sinkRate,
                                              float sinkTurbulence,
                                              float coreSize, float coreTransitionSize,
                                              float sinkLayerSize,
                                              float sinkTransitionSize)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_WeatherRemoveThermal(HANDLE hSimConnect, SIMCONNECT_OBJECT_ID ObjectID)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_AICreateParkedATCAircraft(HANDLE hSimConnect, const char *szContainerTitle,
                                                   const char *szTailNumber, const char *szAirportID,
                                                   SIMCONNECT_DATA_REQUEST_ID RequestID)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_AICreateEnrouteATCAircraft(HANDLE hSimConnect, const char *szContainerTitle,
                                                    const char *szTailNumber, int iFlightNumber,
                                                    const char *szFlightPlanPath, double dFlightPlanPosition,
                                                    BOOL bTouchAndGo,
                                                    SIMCONNECT_DATA_REQUEST_ID RequestID)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_AICreateNonATCAircraft(HANDLE hSimConnect, const char *szContainerTitle,
                                                const char *szTailNumber,
                                                SIMCONNECT_DATA_INITPOSITION InitPos,
                                                SIMCONNECT_DATA_REQUEST_ID RequestID)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_AICreateSimulatedObject(HANDLE hSimConnect, const char *szContainerTitle,
                                                 SIMCONNECT_DATA_INITPOSITION InitPos,
                                                 SIMCONNECT_DATA_REQUEST_ID RequestID)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_AIReleaseControl(HANDLE hSimConnect, SIMCONNECT_OBJECT_ID ObjectID,
                                          SIMCONNECT_DATA_REQUEST_ID RequestID)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_AIRemoveObject(HANDLE hSimConnect, SIMCONNECT_OBJECT_ID ObjectID,
                                        SIMCONNECT_DATA_REQUEST_ID RequestID)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_AISetAircraftFlightPlan(HANDLE hSimConnect, SIMCONNECT_OBJECT_ID ObjectID,
                                                 const char *szFlightPlanPath,
                                                 SIMCONNECT_DATA_REQUEST_ID RequestID)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_ExecuteMissionAction(HANDLE hSimConnect, const GUID guidInstanceId)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_CompleteCustomMissionAction(HANDLE hSimConnect, const GUID guidInstanceId)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_Close(HANDLE hSimConnect)
{
  qDebug() << "Dummy: SimConnect_Close";
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_RetrieveString(SIMCONNECT_RECV *pData, DWORD cbData, void *pStringV,
                                        char **pszString,
                                        DWORD *pcbString)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_GetLastSentPacketID(HANDLE hSimConnect, DWORD *pdwError)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_Open(HANDLE *phSimConnect, LPCSTR szName, HWND hWnd, DWORD UserEventWin32,
                              HANDLE hEventHandle,
                              DWORD ConfigIndex)
{
  qDebug() << "Dummy: SimConnect_Open";
  return 1;
}

SIMCONNECTAPI SimConnect_CallDispatch(HANDLE hSimConnect, DispatchProc pfcnDispatch, void *pContext)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_GetNextDispatch(HANDLE hSimConnect, SIMCONNECT_RECV **ppData, DWORD *pcbData)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_RequestResponseTimes(HANDLE hSimConnect, DWORD nCount, float *fElapsedSeconds)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_InsertString(char *pDest, DWORD cbDest, void **ppEnd, DWORD *pcbStringV,
                                      const char *pSource)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_CameraSetRelative6DOF(HANDLE hSimConnect, float fDeltaX, float fDeltaY,
                                               float fDeltaZ, float fPitchDeg, float fBankDeg,
                                               float fHeadingDeg)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_MenuAddItem(HANDLE hSimConnect, const char *szMenuItem,
                                     SIMCONNECT_CLIENT_EVENT_ID MenuEventID,
                                     DWORD dwData)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_MenuDeleteItem(HANDLE hSimConnect, SIMCONNECT_CLIENT_EVENT_ID MenuEventID)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_MenuAddSubItem(HANDLE hSimConnect, SIMCONNECT_CLIENT_EVENT_ID MenuEventID,
                                        const char *szMenuItem, SIMCONNECT_CLIENT_EVENT_ID SubMenuEventID,
                                        DWORD dwData)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_MenuDeleteSubItem(HANDLE hSimConnect, SIMCONNECT_CLIENT_EVENT_ID MenuEventID,
                                           const SIMCONNECT_CLIENT_EVENT_ID SubMenuEventID)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_RequestSystemState(HANDLE hSimConnect, SIMCONNECT_DATA_REQUEST_ID RequestID,
                                            const char *szState)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_SetSystemState(HANDLE hSimConnect, const char *szState, DWORD dwInteger,
                                        float fFloat,
                                        const char *szString)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_MapClientDataNameToID(HANDLE hSimConnect, const char *szClientDataName,
                                               SIMCONNECT_CLIENT_DATA_ID ClientDataID)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_CreateClientData(HANDLE hSimConnect, SIMCONNECT_CLIENT_DATA_ID ClientDataID,
                                          DWORD dwSize,
                                          SIMCONNECT_CREATE_CLIENT_DATA_FLAG Flags)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_AddToClientDataDefinition(HANDLE hSimConnect,
                                                   SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID,
                                                   DWORD dwOffset, DWORD dwSizeOrType, float fEpsilon,
                                                   DWORD DatumID)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_ClearClientDataDefinition(HANDLE hSimConnect,
                                                   SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_RequestClientData(HANDLE hSimConnect, SIMCONNECT_CLIENT_DATA_ID ClientDataID,
                                           SIMCONNECT_DATA_REQUEST_ID RequestID,
                                           SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID,
                                           SIMCONNECT_CLIENT_DATA_PERIOD Period,
                                           SIMCONNECT_CLIENT_DATA_REQUEST_FLAG Flags, DWORD origin,
                                           DWORD interval,
                                           DWORD limit)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_SetClientData(HANDLE hSimConnect, SIMCONNECT_CLIENT_DATA_ID ClientDataID,
                                       SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID,
                                       SIMCONNECT_CLIENT_DATA_SET_FLAG Flags, DWORD dwReserved,
                                       DWORD cbUnitSize,
                                       void *pDataSet)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_FlightLoad(HANDLE hSimConnect, const char *szFileName)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_FlightSave(HANDLE hSimConnect, const char *szFileName, const char *szTitle,
                                    const char *szDescription,
                                    DWORD Flags)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_FlightPlanLoad(HANDLE hSimConnect, const char *szFileName)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_Text(HANDLE hSimConnect, SIMCONNECT_TEXT_TYPE type, float fTimeSeconds,
                              SIMCONNECT_CLIENT_EVENT_ID EventID, DWORD cbUnitSize,
                              void *pDataSet)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_SubscribeToFacilities(HANDLE hSimConnect, SIMCONNECT_FACILITY_LIST_TYPE type,
                                               SIMCONNECT_DATA_REQUEST_ID RequestID)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_UnsubscribeToFacilities(HANDLE hSimConnect, SIMCONNECT_FACILITY_LIST_TYPE type)
{
  return E_FAIL;
}

SIMCONNECTAPI SimConnect_RequestFacilitiesList(HANDLE hSimConnect, SIMCONNECT_FACILITY_LIST_TYPE type,
                                               SIMCONNECT_DATA_REQUEST_ID RequestID)
{
  return E_FAIL;
}

#endif
