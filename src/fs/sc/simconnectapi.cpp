/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY{return S_OK;} without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#include "fs/sc/simconnectapi.h"

#include "win/activationcontext.h"

#include <QDebug>

#pragma GCC diagnostic ignored "-Wold-style-cast"

#define BINDSC(a) (error |= \
                     ((*(FARPROC *)&SC_ ## a = \
                         (FARPROC)context.getProcAddress("SimConnect.dll", "SimConnect_" # a)) == NULL))

namespace atools {
namespace fs {
namespace sc {

SimConnectApi::SimConnectApi()
{

}

SimConnectApi::~SimConnectApi()
{

}

bool SimConnectApi::bindFunctions(atools::win::ActivationContext& context)
{
  qDebug() << Q_FUNC_INFO;

  bool error = false;

#if defined(SIMCONNECT_BUILD)
  BINDSC(Open);
  BINDSC(Close);

  BINDSC(MapClientEventToSimEvent);
  BINDSC(TransmitClientEvent);
  BINDSC(SetSystemEventState);
  BINDSC(AddClientEventToNotificationGroup);
  BINDSC(RemoveClientEvent);
  BINDSC(SetNotificationGroupPriority);
  BINDSC(ClearNotificationGroup);
  BINDSC(RequestNotificationGroup);
  BINDSC(AddToDataDefinition);
  BINDSC(ClearDataDefinition);
  BINDSC(RequestDataOnSimObject);
  BINDSC(RequestDataOnSimObjectType);
  BINDSC(SetDataOnSimObject);
  BINDSC(MapInputEventToClientEvent);
  BINDSC(SetInputGroupPriority);
  BINDSC(RemoveInputEvent);
  BINDSC(ClearInputGroup);
  BINDSC(SetInputGroupState);
  BINDSC(RequestReservedKey);
  BINDSC(SubscribeToSystemEvent);
  BINDSC(UnsubscribeFromSystemEvent);
  BINDSC(WeatherRequestInterpolatedObservation);
  BINDSC(WeatherRequestObservationAtStation);
  BINDSC(WeatherRequestObservationAtNearestStation);
  BINDSC(WeatherCreateStation);
  BINDSC(WeatherRemoveStation);
  BINDSC(WeatherSetObservation);
  BINDSC(WeatherSetModeServer);
  BINDSC(WeatherSetModeTheme);
  BINDSC(WeatherSetModeGlobal);
  BINDSC(WeatherSetModeCustom);
  BINDSC(WeatherSetDynamicUpdateRate);
  BINDSC(WeatherRequestCloudState);
  BINDSC(WeatherCreateThermal);
  BINDSC(WeatherRemoveThermal);
  BINDSC(AICreateParkedATCAircraft);
  BINDSC(AICreateEnrouteATCAircraft);
  BINDSC(AICreateNonATCAircraft);
  BINDSC(AICreateSimulatedObject);
  BINDSC(AIReleaseControl);
  BINDSC(AIRemoveObject);
  BINDSC(AISetAircraftFlightPlan);
  BINDSC(ExecuteMissionAction);
  BINDSC(CompleteCustomMissionAction);
  BINDSC(RetrieveString);
  BINDSC(GetLastSentPacketID);
  BINDSC(CallDispatch);
  BINDSC(GetNextDispatch);
  BINDSC(RequestResponseTimes);
  BINDSC(InsertString);
  BINDSC(CameraSetRelative6DOF);
  BINDSC(MenuAddItem);
  BINDSC(MenuDeleteItem);
  BINDSC(MenuAddSubItem);
  BINDSC(MenuDeleteSubItem);
  BINDSC(RequestSystemState);
  BINDSC(SetSystemState);
  BINDSC(MapClientDataNameToID);
  BINDSC(CreateClientData);
  BINDSC(AddToClientDataDefinition);
  BINDSC(ClearClientDataDefinition);
  BINDSC(RequestClientData);
  BINDSC(SetClientData);
  BINDSC(FlightLoad);
  BINDSC(FlightSave);
  BINDSC(FlightPlanLoad);
  BINDSC(Text);
  BINDSC(SubscribeToFacilities);
  BINDSC(UnsubscribeToFacilities);
  BINDSC(RequestFacilitiesList);
#else
  Q_UNUSED(context);
#endif

  qDebug() << Q_FUNC_INFO << "done";

  return !error;
}

HRESULT SimConnectApi::Open(LPCSTR szName, HWND hWnd, DWORD UserEventWin32, HANDLE hEventHandle, DWORD ConfigIndex)
{
  Close();

  if(SC_Open == nullptr)
    return E_FAIL;
  else
    return SC_Open(&hSimConnect, szName, hWnd, UserEventWin32, hEventHandle, ConfigIndex);
}

HRESULT SimConnectApi::Close()
{
  if(hSimConnect != NULL)
  {
    if(SC_Close == nullptr)
      return E_FAIL;
    else
    {
      HRESULT hr = SC_Close(hSimConnect);
      hSimConnect = NULL;
      return hr;
    }
  }
  else
    return S_OK;
}

HRESULT SimConnectApi::MapClientEventToSimEvent(SIMCONNECT_CLIENT_EVENT_ID EventID, const char *EventName)
{
  if(hSimConnect == NULL || SC_MapClientEventToSimEvent == nullptr)
    return E_FAIL;
  else
    return SC_MapClientEventToSimEvent(hSimConnect, EventID, EventName);
}

HRESULT SimConnectApi::TransmitClientEvent(SIMCONNECT_OBJECT_ID ObjectID, SIMCONNECT_CLIENT_EVENT_ID EventID,
                                           DWORD dwData, SIMCONNECT_NOTIFICATION_GROUP_ID GroupID,
                                           SIMCONNECT_EVENT_FLAG Flags)
{
  if(hSimConnect == NULL || SC_TransmitClientEvent == nullptr)
    return E_FAIL;
  else
    return SC_TransmitClientEvent(hSimConnect, ObjectID, EventID, dwData, GroupID, Flags);
}

HRESULT SimConnectApi::SetSystemEventState(SIMCONNECT_CLIENT_EVENT_ID EventID, SIMCONNECT_STATE dwState)
{
  if(hSimConnect == NULL || SC_SetSystemEventState == nullptr)
    return E_FAIL;
  else
    return SC_SetSystemEventState(hSimConnect, EventID, dwState);
}

HRESULT SimConnectApi::AddClientEventToNotificationGroup(SIMCONNECT_NOTIFICATION_GROUP_ID GroupID,
                                                         SIMCONNECT_CLIENT_EVENT_ID EventID,
                                                         WINBOOL bMaskable)
{
  if(hSimConnect == NULL || SC_AddClientEventToNotificationGroup == nullptr)
    return E_FAIL;
  else
    return SC_AddClientEventToNotificationGroup(hSimConnect, GroupID, EventID, bMaskable);
}

HRESULT SimConnectApi::RemoveClientEvent(SIMCONNECT_NOTIFICATION_GROUP_ID GroupID, SIMCONNECT_CLIENT_EVENT_ID EventID)
{
  if(hSimConnect == NULL || SC_RemoveClientEvent == nullptr)
    return E_FAIL;
  else
    return SC_RemoveClientEvent(hSimConnect, GroupID, EventID);
}

HRESULT SimConnectApi::SetNotificationGroupPriority(SIMCONNECT_NOTIFICATION_GROUP_ID GroupID, DWORD uPriority)
{
  if(hSimConnect == NULL || SC_SetNotificationGroupPriority == nullptr)
    return E_FAIL;
  else
    return SC_SetNotificationGroupPriority(hSimConnect, GroupID, uPriority);
}

HRESULT SimConnectApi::ClearNotificationGroup(SIMCONNECT_NOTIFICATION_GROUP_ID GroupID)
{
  if(hSimConnect == NULL || SC_ClearNotificationGroup == nullptr)
    return E_FAIL;
  else
    return SC_ClearNotificationGroup(hSimConnect, GroupID);
}

HRESULT SimConnectApi::RequestNotificationGroup(SIMCONNECT_NOTIFICATION_GROUP_ID GroupID, DWORD dwReserved, DWORD Flags)
{
  if(hSimConnect == NULL || SC_RequestNotificationGroup == nullptr)
    return E_FAIL;
  else
    return SC_RequestNotificationGroup(hSimConnect, GroupID, dwReserved, Flags);
}

HRESULT SimConnectApi::AddToDataDefinition(SIMCONNECT_DATA_DEFINITION_ID DefineID, const char *DatumName,
                                           const char *UnitsName, SIMCONNECT_DATATYPE DatumType, float fEpsilon,
                                           DWORD DatumID)
{
  if(hSimConnect == NULL || SC_AddToDataDefinition == nullptr)
    return E_FAIL;
  else
    return SC_AddToDataDefinition(hSimConnect, DefineID, DatumName, UnitsName, DatumType, fEpsilon, DatumID);
}

HRESULT SimConnectApi::ClearDataDefinition(SIMCONNECT_DATA_DEFINITION_ID DefineID)
{
  if(hSimConnect == NULL || SC_ClearDataDefinition == nullptr)
    return E_FAIL;
  else
    return SC_ClearDataDefinition(hSimConnect, DefineID);
}

HRESULT SimConnectApi::RequestDataOnSimObject(SIMCONNECT_DATA_REQUEST_ID RequestID,
                                              SIMCONNECT_DATA_DEFINITION_ID DefineID, SIMCONNECT_OBJECT_ID ObjectID,
                                              SIMCONNECT_PERIOD Period,
                                              SIMCONNECT_DATA_REQUEST_FLAG Flags, DWORD origin, DWORD interval,
                                              DWORD limit)
{
  if(hSimConnect == NULL || SC_RequestDataOnSimObject == nullptr)
    return E_FAIL;
  else
    return SC_RequestDataOnSimObject(hSimConnect, RequestID, DefineID, ObjectID, Period, Flags, origin, interval,
                                     limit);
}

HRESULT SimConnectApi::RequestDataOnSimObjectType(SIMCONNECT_DATA_REQUEST_ID RequestID,
                                                  SIMCONNECT_DATA_DEFINITION_ID DefineID, DWORD dwRadiusMeters,
                                                  SIMCONNECT_SIMOBJECT_TYPE type)
{
  if(hSimConnect == NULL || SC_RequestDataOnSimObjectType == nullptr)
    return E_FAIL;
  else
    return SC_RequestDataOnSimObjectType(hSimConnect, RequestID, DefineID, dwRadiusMeters, type);
}

HRESULT SimConnectApi::SetDataOnSimObject(SIMCONNECT_DATA_DEFINITION_ID DefineID, SIMCONNECT_OBJECT_ID ObjectID,
                                          SIMCONNECT_DATA_SET_FLAG Flags, DWORD ArrayCount, DWORD cbUnitSize,
                                          void *pDataSet)
{
  if(hSimConnect == NULL || SC_SetDataOnSimObject == nullptr)
    return E_FAIL;
  else
    return SC_SetDataOnSimObject(hSimConnect, DefineID, ObjectID, Flags, ArrayCount, cbUnitSize, pDataSet);
}

HRESULT SimConnectApi::MapInputEventToClientEvent(SIMCONNECT_INPUT_GROUP_ID GroupID, const char *szInputDefinition,
                                                  SIMCONNECT_CLIENT_EVENT_ID DownEventID, DWORD DownValue,
                                                  SIMCONNECT_CLIENT_EVENT_ID UpEventID, DWORD UpValue,
                                                  WINBOOL bMaskable)
{
  if(hSimConnect == NULL || SC_MapInputEventToClientEvent == nullptr)
    return E_FAIL;
  else
    return SC_MapInputEventToClientEvent(hSimConnect, GroupID, szInputDefinition, DownEventID, DownValue, UpEventID,
                                         UpValue,
                                         bMaskable);
}

HRESULT SimConnectApi::SetInputGroupPriority(SIMCONNECT_INPUT_GROUP_ID GroupID, DWORD uPriority)
{
  if(hSimConnect == NULL || SC_SetInputGroupPriority == nullptr)
    return E_FAIL;
  else
    return SC_SetInputGroupPriority(hSimConnect, GroupID, uPriority);
}

HRESULT SimConnectApi::RemoveInputEvent(SIMCONNECT_INPUT_GROUP_ID GroupID, const char *szInputDefinition)
{
  if(hSimConnect == NULL || SC_RemoveInputEvent == nullptr)
    return E_FAIL;
  else
    return SC_RemoveInputEvent(hSimConnect, GroupID, szInputDefinition);
}

HRESULT SimConnectApi::ClearInputGroup(SIMCONNECT_INPUT_GROUP_ID GroupID)
{
  if(hSimConnect == NULL || SC_ClearInputGroup == nullptr)
    return E_FAIL;
  else
    return SC_ClearInputGroup(hSimConnect, GroupID);
}

HRESULT SimConnectApi::SetInputGroupState(SIMCONNECT_INPUT_GROUP_ID GroupID, DWORD dwState)
{
  if(hSimConnect == NULL || SC_SetInputGroupState == nullptr)
    return E_FAIL;
  else
    return SC_SetInputGroupState(hSimConnect, GroupID, dwState);
}

HRESULT SimConnectApi::RequestReservedKey(SIMCONNECT_CLIENT_EVENT_ID EventID, const char *szKeyChoice1,
                                          const char *szKeyChoice2,
                                          const char *szKeyChoice3)
{
  if(hSimConnect == NULL || SC_RequestReservedKey == nullptr)
    return E_FAIL;
  else
    return SC_RequestReservedKey(hSimConnect, EventID, szKeyChoice1, szKeyChoice2, szKeyChoice3);
}

HRESULT SimConnectApi::SubscribeToSystemEvent(SIMCONNECT_CLIENT_EVENT_ID EventID, const char *SystemEventName)
{
  if(hSimConnect == NULL || SC_SubscribeToSystemEvent == nullptr)
    return E_FAIL;
  else
    return SC_SubscribeToSystemEvent(hSimConnect, EventID, SystemEventName);
}

HRESULT SimConnectApi::UnsubscribeFromSystemEvent(SIMCONNECT_CLIENT_EVENT_ID EventID)
{
  if(hSimConnect == NULL || SC_UnsubscribeFromSystemEvent == nullptr)
    return E_FAIL;
  else
    return SC_UnsubscribeFromSystemEvent(hSimConnect, EventID);
}

HRESULT SimConnectApi::WeatherRequestInterpolatedObservation(SIMCONNECT_DATA_REQUEST_ID RequestID, float lat, float lon,
                                                             float alt)
{
  if(hSimConnect == NULL || SC_WeatherRequestInterpolatedObservation == nullptr)
    return E_FAIL;
  else
    return SC_WeatherRequestInterpolatedObservation(hSimConnect, RequestID, lat, lon, alt);
}

HRESULT SimConnectApi::WeatherRequestObservationAtStation(SIMCONNECT_DATA_REQUEST_ID RequestID, const char *szICAO)
{
  if(hSimConnect == NULL || SC_WeatherRequestObservationAtStation == nullptr)
    return E_FAIL;
  else
    return SC_WeatherRequestObservationAtStation(hSimConnect, RequestID, szICAO);
}

HRESULT SimConnectApi::WeatherRequestObservationAtNearestStation(SIMCONNECT_DATA_REQUEST_ID RequestID, float lat,
                                                                 float lon)
{
  if(hSimConnect == NULL || SC_WeatherRequestObservationAtNearestStation == nullptr)
    return E_FAIL;
  else
    return SC_WeatherRequestObservationAtNearestStation(hSimConnect, RequestID, lat, lon);
}

HRESULT SimConnectApi::WeatherCreateStation(SIMCONNECT_DATA_REQUEST_ID RequestID, const char *szICAO,
                                            const char *szName, float lat, float lon,
                                            float alt)
{
  if(hSimConnect == NULL || SC_WeatherCreateStation == nullptr)
    return E_FAIL;
  else
    return SC_WeatherCreateStation(hSimConnect, RequestID, szICAO, szName, lat, lon, alt);
}

HRESULT SimConnectApi::WeatherRemoveStation(SIMCONNECT_DATA_REQUEST_ID RequestID, const char *szICAO)
{
  if(hSimConnect == NULL || SC_WeatherRemoveStation == nullptr)
    return E_FAIL;
  else
    return SC_WeatherRemoveStation(hSimConnect, RequestID, szICAO);
}

HRESULT SimConnectApi::WeatherSetObservation(DWORD Seconds, const char *szMETAR)
{
  if(hSimConnect == NULL || SC_WeatherSetObservation == nullptr)
    return E_FAIL;
  else
    return SC_WeatherSetObservation(hSimConnect, Seconds, szMETAR);
}

HRESULT SimConnectApi::WeatherSetModeServer(DWORD dwPort, DWORD dwSeconds)
{
  if(hSimConnect == NULL || SC_WeatherSetModeServer == nullptr)
    return E_FAIL;
  else
    return SC_WeatherSetModeServer(hSimConnect, dwPort, dwSeconds);
}

HRESULT SimConnectApi::WeatherSetModeTheme(const char *szThemeName)
{
  if(hSimConnect == NULL || SC_WeatherSetModeTheme == nullptr)
    return E_FAIL;
  else
    return SC_WeatherSetModeTheme(hSimConnect, szThemeName);
}

HRESULT SimConnectApi::WeatherSetModeGlobal()
{
  if(hSimConnect == NULL || SC_WeatherSetModeGlobal == nullptr)
    return E_FAIL;
  else
    return SC_WeatherSetModeGlobal(hSimConnect);
}

HRESULT SimConnectApi::WeatherSetModeCustom()
{
  if(hSimConnect == NULL || SC_WeatherSetModeCustom == nullptr)
    return E_FAIL;
  else
    return SC_WeatherSetModeCustom(hSimConnect);
}

HRESULT SimConnectApi::WeatherSetDynamicUpdateRate(DWORD dwRate)
{
  if(hSimConnect == NULL || SC_WeatherSetDynamicUpdateRate == nullptr)
    return E_FAIL;
  else
    return SC_WeatherSetDynamicUpdateRate(hSimConnect, dwRate);
}

HRESULT SimConnectApi::WeatherRequestCloudState(SIMCONNECT_DATA_REQUEST_ID RequestID, float minLat, float minLon,
                                                float minAlt, float maxLat, float maxLon, float maxAlt,
                                                DWORD dwFlags)
{
  if(hSimConnect == NULL || SC_WeatherRequestCloudState == nullptr)
    return E_FAIL;
  else
    return SC_WeatherRequestCloudState(hSimConnect, RequestID, minLat, minLon, minAlt, maxLat, maxLon, maxAlt, dwFlags);


}

HRESULT SimConnectApi::WeatherCreateThermal(SIMCONNECT_DATA_REQUEST_ID RequestID, float lat, float lon, float alt,
                                            float radius, float height, float coreRate, float coreTurbulence,
                                            float sinkRate, float sinkTurbulence, float coreSize,
                                            float coreTransitionSize, float sinkLayerSize,
                                            float sinkTransitionSize)
{
  if(hSimConnect == NULL || SC_WeatherCreateThermal == nullptr)
    return E_FAIL;
  else
    return SC_WeatherCreateThermal(hSimConnect, RequestID, lat, lon, alt, radius, height, coreRate, coreTurbulence,
                                   sinkRate, sinkTurbulence, coreSize, coreTransitionSize, sinkLayerSize,
                                   sinkTransitionSize);
}

HRESULT SimConnectApi::WeatherRemoveThermal(SIMCONNECT_OBJECT_ID ObjectID)
{
  if(hSimConnect == NULL || SC_WeatherRemoveThermal == nullptr)
    return E_FAIL;
  else
    return SC_WeatherRemoveThermal(hSimConnect, ObjectID);
}

HRESULT SimConnectApi::AICreateParkedATCAircraft(const char *szContainerTitle, const char *szTailNumber,
                                                 const char *szAirportID,
                                                 SIMCONNECT_DATA_REQUEST_ID RequestID)
{
  if(hSimConnect == NULL || SC_AICreateParkedATCAircraft == nullptr)
    return E_FAIL;
  else
    return SC_AICreateParkedATCAircraft(hSimConnect, szContainerTitle, szTailNumber, szAirportID, RequestID);
}

HRESULT SimConnectApi::AICreateEnrouteATCAircraft(const char *szContainerTitle, const char *szTailNumber,
                                                  int iFlightNumber, const char *szFlightPlanPath,
                                                  double dFlightPlanPosition, WINBOOL bTouchAndGo,
                                                  SIMCONNECT_DATA_REQUEST_ID RequestID)
{
  if(hSimConnect == NULL || SC_AICreateEnrouteATCAircraft == nullptr)
    return E_FAIL;
  else
    return SC_AICreateEnrouteATCAircraft(hSimConnect, szContainerTitle, szTailNumber, iFlightNumber, szFlightPlanPath,
                                         dFlightPlanPosition, bTouchAndGo,
                                         RequestID);
}

HRESULT SimConnectApi::AICreateNonATCAircraft(const char *szContainerTitle, const char *szTailNumber,
                                              SIMCONNECT_DATA_INITPOSITION InitPos,
                                              SIMCONNECT_DATA_REQUEST_ID RequestID)
{
  if(hSimConnect == NULL || SC_AICreateNonATCAircraft == nullptr)
    return E_FAIL;
  else
    return SC_AICreateNonATCAircraft(hSimConnect, szContainerTitle, szTailNumber, InitPos, RequestID);
}

HRESULT SimConnectApi::AICreateSimulatedObject(const char *szContainerTitle, SIMCONNECT_DATA_INITPOSITION InitPos,
                                               SIMCONNECT_DATA_REQUEST_ID RequestID)
{
  if(hSimConnect == NULL || SC_AICreateSimulatedObject == nullptr)
    return E_FAIL;
  else
    return SC_AICreateSimulatedObject(hSimConnect, szContainerTitle, InitPos, RequestID);
}

HRESULT SimConnectApi::AIReleaseControl(SIMCONNECT_OBJECT_ID ObjectID, SIMCONNECT_DATA_REQUEST_ID RequestID)
{
  if(hSimConnect == NULL || SC_AIReleaseControl == nullptr)
    return E_FAIL;
  else
    return SC_AIReleaseControl(hSimConnect, ObjectID, RequestID);
}

HRESULT SimConnectApi::AIRemoveObject(SIMCONNECT_OBJECT_ID ObjectID, SIMCONNECT_DATA_REQUEST_ID RequestID)
{
  if(hSimConnect == NULL || SC_AIRemoveObject == nullptr)
    return E_FAIL;
  else
    return SC_AIRemoveObject(hSimConnect, ObjectID, RequestID);
}

HRESULT SimConnectApi::AISetAircraftFlightPlan(SIMCONNECT_OBJECT_ID ObjectID, const char *szFlightPlanPath,
                                               SIMCONNECT_DATA_REQUEST_ID RequestID)
{
  if(hSimConnect == NULL || SC_AISetAircraftFlightPlan == nullptr)
    return E_FAIL;
  else
    return SC_AISetAircraftFlightPlan(hSimConnect, ObjectID, szFlightPlanPath, RequestID);
}

HRESULT SimConnectApi::ExecuteMissionAction(const GUID guidInstanceId)
{
  if(hSimConnect == NULL || SC_ExecuteMissionAction == nullptr)
    return E_FAIL;
  else
    return SC_ExecuteMissionAction(hSimConnect, guidInstanceId);
}

HRESULT SimConnectApi::CompleteCustomMissionAction(const GUID guidInstanceId)
{
  if(hSimConnect == NULL || SC_CompleteCustomMissionAction == nullptr)
    return E_FAIL;
  else
    return SC_CompleteCustomMissionAction(hSimConnect, guidInstanceId);
}

HRESULT SimConnectApi::RetrieveString(SIMCONNECT_RECV *pData, DWORD cbData, void *pStringV, char **pszString,
                                      DWORD *pcbString)
{
  if(SC_RetrieveString == nullptr)
    return E_FAIL;
  else
    return SC_RetrieveString(pData, cbData, pStringV, pszString, pcbString);
}

HRESULT SimConnectApi::GetLastSentPacketID(DWORD *pdwError)
{
  if(hSimConnect == NULL || SC_GetLastSentPacketID == nullptr)
    return E_FAIL;
  else
    return SC_GetLastSentPacketID(hSimConnect, pdwError);
}

HRESULT SimConnectApi::CallDispatch(DispatchProc pfcnDispatch, void *pContext)
{
  if(hSimConnect == NULL || SC_CallDispatch == nullptr)
    return E_FAIL;
  else
    return SC_CallDispatch(hSimConnect, pfcnDispatch, pContext);
}

HRESULT SimConnectApi::GetNextDispatch(SIMCONNECT_RECV **ppData, DWORD *pcbData)
{
  if(hSimConnect == NULL || SC_GetNextDispatch == nullptr)
    return E_FAIL;
  else
    return SC_GetNextDispatch(hSimConnect, ppData, pcbData);
}

HRESULT SimConnectApi::RequestResponseTimes(DWORD nCount, float *fElapsedSeconds)
{
  if(hSimConnect == NULL || SC_RequestResponseTimes == nullptr)
    return E_FAIL;
  else
    return SC_RequestResponseTimes(hSimConnect, nCount, fElapsedSeconds);
}

HRESULT SimConnectApi::InsertString(char *pDest, DWORD cbDest, void **ppEnd, DWORD *pcbStringV, const char *pSource)
{
  if(SC_InsertString == nullptr)
    return E_FAIL;
  else
    return SC_InsertString(pDest, cbDest, ppEnd, pcbStringV, pSource);
}

HRESULT SimConnectApi::CameraSetRelative6DOF(float fDeltaX, float fDeltaY, float fDeltaZ, float fPitchDeg,
                                             float fBankDeg,
                                             float fHeadingDeg)
{
  if(hSimConnect == NULL || SC_CameraSetRelative6DOF == nullptr)
    return E_FAIL;
  else
    return SC_CameraSetRelative6DOF(hSimConnect, fDeltaX, fDeltaY, fDeltaZ, fPitchDeg, fBankDeg, fHeadingDeg);
}

HRESULT SimConnectApi::MenuAddItem(const char *szMenuItem, SIMCONNECT_CLIENT_EVENT_ID MenuEventID, DWORD dwData)
{
  if(hSimConnect == NULL || SC_MenuAddItem == nullptr)
    return E_FAIL;
  else
    return SC_MenuAddItem(hSimConnect, szMenuItem, MenuEventID, dwData);
}

HRESULT SimConnectApi::MenuDeleteItem(SIMCONNECT_CLIENT_EVENT_ID MenuEventID)
{
  if(hSimConnect == NULL || SC_MenuDeleteItem == nullptr)
    return E_FAIL;
  else
    return SC_MenuDeleteItem(hSimConnect, MenuEventID);
}

HRESULT SimConnectApi::MenuAddSubItem(SIMCONNECT_CLIENT_EVENT_ID MenuEventID, const char *szMenuItem,
                                      SIMCONNECT_CLIENT_EVENT_ID SubMenuEventID,
                                      DWORD dwData)
{
  if(hSimConnect == NULL || SC_MenuAddSubItem == nullptr)
    return E_FAIL;
  else
    return SC_MenuAddSubItem(hSimConnect, MenuEventID, szMenuItem, SubMenuEventID, dwData);
}

HRESULT SimConnectApi::MenuDeleteSubItem(SIMCONNECT_CLIENT_EVENT_ID MenuEventID,
                                         const SIMCONNECT_CLIENT_EVENT_ID SubMenuEventID)
{
  if(hSimConnect == NULL || SC_MenuDeleteSubItem == nullptr)
    return E_FAIL;
  else
    return SC_MenuDeleteSubItem(hSimConnect, MenuEventID, SubMenuEventID);
}

HRESULT SimConnectApi::RequestSystemState(SIMCONNECT_DATA_REQUEST_ID RequestID, const char *szState)
{
  if(hSimConnect == NULL || SC_RequestSystemState == nullptr)
    return E_FAIL;
  else
    return SC_RequestSystemState(hSimConnect, RequestID, szState);
}

HRESULT SimConnectApi::SetSystemState(const char *szState, DWORD dwInteger, float fFloat, const char *szString)
{
  if(hSimConnect == NULL || SC_SetSystemState == nullptr)
    return E_FAIL;
  else
    return SC_SetSystemState(hSimConnect, szState, dwInteger, fFloat, szString);
}

HRESULT SimConnectApi::MapClientDataNameToID(const char *szClientDataName, SIMCONNECT_CLIENT_DATA_ID ClientDataID)
{
  if(hSimConnect == NULL || SC_MapClientDataNameToID == nullptr)
    return E_FAIL;
  else
    return SC_MapClientDataNameToID(hSimConnect, szClientDataName, ClientDataID);
}

HRESULT SimConnectApi::CreateClientData(SIMCONNECT_CLIENT_DATA_ID ClientDataID, DWORD dwSize,
                                        SIMCONNECT_CREATE_CLIENT_DATA_FLAG Flags)
{
  if(hSimConnect == NULL || SC_CreateClientData == nullptr)
    return E_FAIL;
  else
    return SC_CreateClientData(hSimConnect, ClientDataID, dwSize, Flags);
}

HRESULT SimConnectApi::AddToClientDataDefinition(SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID, DWORD dwOffset,
                                                 DWORD dwSizeOrType, float fEpsilon,
                                                 DWORD DatumID)
{
  if(hSimConnect == NULL || SC_AddToClientDataDefinition == nullptr)
    return E_FAIL;
  else
    return SC_AddToClientDataDefinition(hSimConnect, DefineID, dwOffset, dwSizeOrType, fEpsilon, DatumID);
}

HRESULT SimConnectApi::ClearClientDataDefinition(SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID)
{
  if(hSimConnect == NULL || SC_ClearClientDataDefinition == nullptr)
    return E_FAIL;
  else
    return SC_ClearClientDataDefinition(hSimConnect, DefineID);
}

HRESULT SimConnectApi::RequestClientData(SIMCONNECT_CLIENT_DATA_ID ClientDataID, SIMCONNECT_DATA_REQUEST_ID RequestID,
                                         SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID,
                                         SIMCONNECT_CLIENT_DATA_PERIOD Period,
                                         SIMCONNECT_CLIENT_DATA_REQUEST_FLAG Flags, DWORD origin, DWORD interval,
                                         DWORD limit)
{
  if(hSimConnect == NULL || SC_RequestClientData == nullptr)
    return E_FAIL;
  else
    return SC_RequestClientData(hSimConnect, ClientDataID, RequestID, DefineID, Period, Flags, origin, interval, limit);


}

HRESULT SimConnectApi::SetClientData(SIMCONNECT_CLIENT_DATA_ID ClientDataID,
                                     SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID,
                                     SIMCONNECT_CLIENT_DATA_SET_FLAG Flags, DWORD dwReserved, DWORD cbUnitSize,
                                     void *pDataSet)
{
  if(hSimConnect == NULL || SC_SetClientData == nullptr)
    return E_FAIL;
  else
    return SC_SetClientData(hSimConnect, ClientDataID, DefineID, Flags, dwReserved, cbUnitSize, pDataSet);
}

HRESULT SimConnectApi::FlightLoad(const char *szFileName)
{
  if(hSimConnect == NULL || SC_FlightLoad == nullptr)
    return E_FAIL;
  else
    return SC_FlightLoad(hSimConnect, szFileName);
}

HRESULT SimConnectApi::FlightSave(const char *szFileName, const char *szTitle, const char *szDescription, DWORD Flags)
{
  if(hSimConnect == NULL || SC_FlightSave == nullptr)
    return E_FAIL;
  else
    return SC_FlightSave(hSimConnect, szFileName, szTitle, szDescription, Flags);
}

HRESULT SimConnectApi::FlightPlanLoad(const char *szFileName)
{
  if(hSimConnect == NULL || SC_FlightPlanLoad == nullptr)
    return E_FAIL;
  else
    return SC_FlightPlanLoad(hSimConnect, szFileName);
}

HRESULT SimConnectApi::Text(SIMCONNECT_TEXT_TYPE type, float fTimeSeconds, SIMCONNECT_CLIENT_EVENT_ID EventID,
                            DWORD cbUnitSize, void *pDataSet)
{
  if(hSimConnect == NULL || SC_Text == nullptr)
    return E_FAIL;
  else
    return SC_Text(hSimConnect, type, fTimeSeconds, EventID, cbUnitSize, pDataSet);
}

HRESULT SimConnectApi::SubscribeToFacilities(SIMCONNECT_FACILITY_LIST_TYPE type, SIMCONNECT_DATA_REQUEST_ID RequestID)
{
  if(hSimConnect == NULL || SC_SubscribeToFacilities == nullptr)
    return E_FAIL;
  else
    return SC_SubscribeToFacilities(hSimConnect, type, RequestID);
}

HRESULT SimConnectApi::UnsubscribeToFacilities(SIMCONNECT_FACILITY_LIST_TYPE type)
{
  if(hSimConnect == NULL || SC_UnsubscribeToFacilities == nullptr)
    return E_FAIL;
  else
    return SC_UnsubscribeToFacilities(hSimConnect, type);
}

HRESULT SimConnectApi::RequestFacilitiesList(SIMCONNECT_FACILITY_LIST_TYPE type, SIMCONNECT_DATA_REQUEST_ID RequestID)
{
  if(hSimConnect == NULL || SC_RequestFacilitiesList == nullptr)
    return E_FAIL;
  else
    return SC_RequestFacilitiesList(hSimConnect, type, RequestID);
}

} // namespace sc
} // namespace fs
} // namespace atools
