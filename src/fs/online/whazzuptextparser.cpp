/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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

#include "fs/online/whazzuptextparser.h"

#include "fs/util/fsutil.h"
#include "sql/sqlquery.h"
#include "sql/sqlutil.h"
#include "geo/calculations.h"
#include "sql/sqldatabase.h"
#include "geo/linestring.h"
#include "fs/common/binarygeometry.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QIODevice>
#include <QJsonObject>

#ifdef QT_CORE5COMPAT_LIB
#include <QtCore5Compat/QTextCodec>
#endif

using atools::sql::SqlDatabase;
using atools::sql::SqlQuery;
using atools::sql::SqlUtil;
using atools::geo::Rect;
using atools::geo::LineString;
using atools::geo::Pos;

namespace atools {
namespace fs {
namespace online {

/* *INDENT-OFF* */

namespace c {

/* Common column indexes in whazzup.txt */
enum Columns
{
  /*  0 */ CALLSIGN,
  /*  1 */ CID,                // IVAO: VID
  /*  2 */ REALNAME,
  /*  3 */ CLIENTTYPE,
  /*  4 */ FREQUENCY,
  /*  5 */ LATITUDE,
  /*  6 */ LONGITUDE,
  /*  7 */ ALTITUDE,
  /*  8 */ GROUNDSPEED,
  /*  9 */ PLANNED_AIRCRAFT,
  /* 10 */ PLANNED_TASCRUISE,
  /* 11 */ PLANNED_DEPAIRPORT,
  /* 12 */ PLANNED_ALTITUDE,
  /* 13 */ PLANNED_DESTAIRPORT,
  /* 14 */ SERVER,
  /* 15 */ PROTREVISION,       // ignored
  /* 16 */ RATING,             // ignored
  /* 17 */ TRANSPONDER,
  /* 18 */ FACILITYTYPE,
  /* 19 */ VISUALRANGE,
  /* 20 */ PLANNED_REVISION,   // Ignored
  /* 21 */ PLANNED_FLIGHTTYPE, // Flight rules: V or I
  /* 22 */ PLANNED_DEPTIME,
  /* 23 */ PLANNED_ACTDEPTIME,
  /* 24 */ PLANNED_HRSENROUTE,
  /* 25 */ PLANNED_MINENROUTE,
  /* 26 */ PLANNED_HRSFUEL,
  /* 27 */ PLANNED_MINFUEL,
  /* 28 */ PLANNED_ALTAIRPORT,
  /* 29 */ PLANNED_REMARKS,    // IVAO: item 18 (other info)
  /* 30 */ PLANNED_ROUTE,
};

}
namespace v {

/* VATSIM column indexes in whazzup.txt */
enum VatsimColumns
{
  /* 31 */ PLANNED_DEPAIRPORT_LAT = 31, // Ignored
  /* 32 */ PLANNED_DEPAIRPORT_LON,      // "
  /* 33 */ PLANNED_DESTAIRPORT_LAT,     // "
  /* 34 */ PLANNED_DESTAIRPORT_LON,     // "
  /* 35 */ ATIS_MESSAGE,
  /* 36 */ TIME_LAST_ATIS_RECEIVED,
  /* 37 */ TIME_LOGON,
  /* 38 */ HEADING,
  /* 39 */ QNH_IHG,// Ignored
  /* 40 */ QNH_MB, // Ignored
  /* 41 */ STATE // Not in whazzup.txt
};

static int NUM_VATSIMCOLUMNS = STATE + 1;
}

namespace i {

/* IVAO column indexes in whazzup.txt */
enum IvaoColumns
{
  // 31 Ignored
  // 32 "
  // 33 "
  // 34 "
  /* 35	*/ ATIS = 35,
  /* 36	*/ ATIS_TIME,
  /* 37	*/ CONNECTION_TIME,
  /* 38	*/ SOFTWARE_NAME,     // Ignored
  /* 39	*/ SOFTWARE_VERSION,  // Ignored
  /* 40	*/ ADMINISTRATIVE_VERSION,
  /* 41	*/ ATC_PILOT_VERSION,
  /* 42	*/ FLIGHTPLAN_2ND_ALTERNATE_AERODROME,
  /* 44	*/ FLIGHTPLAN_TYPE_OF_FLIGHT,
  /* 43	*/ FLIGHTPLAN_PERSONS_ON_BOARD,
  /* 45	*/ HEADING,
  /* 46	*/ ON_GROUND,
  /* 47	*/ SIMULATOR,
  /* 48	*/ PLANE, // Ignored
  /* 49	*/ STATE // Not in whazzup.txt
};
static int NUM_IVAOCOLUMNS = STATE + 1;
}

/* *INDENT-ON* */

WhazzupTextParser::WhazzupTextParser(sql::SqlDatabase *sqlDb, bool verboseErrorReporting)
  : db(sqlDb), error(verboseErrorReporting)
{
  // Prefix column array for both formats
  for(int i = 0; i < std::max(v::NUM_VATSIMCOLUMNS, i::NUM_IVAOCOLUMNS); i++)
    defaultColumns.append(QString());
}

WhazzupTextParser::~WhazzupTextParser()
{
  deInitQueries();
}

bool WhazzupTextParser::read(QString file, Format streamFormat, const QDateTime& lastUpdate)
{
  reset(); // Also resets format

  format = streamFormat;

  if(streamFormat == VATSIM_JSON3 || streamFormat == IVAO_JSON2)
    return readInternalJson(file, lastUpdate);
  else
  {
    QTextStream stream(&file, QIODevice::ReadOnly | QIODevice::Text);
    return readInternalDelimited(stream, lastUpdate);
  }
}

void WhazzupTextParser::readTransceivers(const QString& file)
{
  transceiverMap.clear();

  // [
  // {
  // "callsign": "DAL128",
  // "transceivers": [
  // {
  // "id": 1,
  // "frequency": 135550000,
  // "latDeg": 53.63043975830078,
  // "lonDeg": 141.40301513671875,
  // "heightMslM": 10715.244140625,
  // "heightAglM": 10715.244140625
  // }
  // ]
  // },

  // Open and check for errors
  QJsonParseError jsonErr;
  QJsonDocument doc = QJsonDocument::fromJson(file.toUtf8(), &jsonErr);
  if(jsonErr.error != QJsonParseError::NoError)
    qWarning() << Q_FUNC_INFO << "Error reading data" << jsonErr.errorString() << "at offset" << jsonErr.offset;

  // Top level is an array or unnamed objects
  const QJsonArray transValues = doc.array();
  for(const QJsonValue& transVal : transValues)
  {
    QJsonObject transObj = transVal.toObject();
    QString callsign = transObj.value(QStringLiteral("callsign")).toString();

    // Get all transceivers with coordinates and frequency for this callsign
    const QJsonArray transceiverValues = transObj.value(QStringLiteral("transceivers")).toArray();
    for(const QJsonValue& transceiverVal : transceiverValues)
    {
      QJsonObject transceiverObj = transceiverVal.toObject();
      Pos pos(transceiverObj.value(QStringLiteral("lonDeg")), transceiverObj.value(QStringLiteral("latDeg")));

      // Build and add object to multi hash with callsign as key
      Transceiver transceiver;

      // frequency in kHz
      int frequency = atools::roundToInt(transceiverObj.value(QStringLiteral("frequency")).toDouble() / 1000.f);
      if(frequency < 100 && error)
        qWarning() << Q_FUNC_INFO << "Invalid frequency" << transceiverObj.value("frequency").toInt() << "for" << callsign;
      else
        transceiver.frequency.insert(frequency);
      transceiver.pos = Pos(transceiverObj.value(QStringLiteral("lonDeg")), transceiverObj.value(QStringLiteral("latDeg")));
      transceiverMap.insert(callsign, transceiver);
    } // for(QJsonValue transceiverVal : transObj.value("transceivers").toArray())
  } // for(QJsonValue transVal : doc.array())
}

bool WhazzupTextParser::readInternalJson(const QString& file, const QDateTime& lastUpdate)
{
  // Open and check for errors =============
  QJsonParseError jsonErr;
  QJsonDocument doc = QJsonDocument::fromJson(file.toUtf8(), &jsonErr);
  if(jsonErr.error != QJsonParseError::NoError)
    qWarning() << Q_FUNC_INFO << "Error reading data" << jsonErr.errorString() << "at offset" << jsonErr.offset;

  QDateTime update;
  QJsonObject obj = doc.object();

  if(format == VATSIM_JSON3)
  {
    // Read time from general section =============
    // "general": {
    // "version": 3,
    // "reload": 1,
    // "update": "20210314160704",
    // "update_timestamp": "2021-03-14T16:07:04.9418979Z",
    // "connected_clients": 1857,
    // "unique_users": 1777
    // },
    QJsonObject generalObj = obj.value(QStringLiteral("general")).toObject();
    update = generalObj.value(QStringLiteral("update_timestamp")).toVariant().toDateTime();

    // Version and reload time in minutes
    version = generalObj.value(QStringLiteral("version")).toInt();
    reload = generalObj.value(QStringLiteral("reload")).toInt();
  }
  else if(format == IVAO_JSON2)
    // "updatedAt": "2021-06-20T21:09:19.642Z",
    update = obj.value(QStringLiteral("updatedAt")).toVariant().toDateTime();

  if(update.isValid())
  {
    if(update <= lastUpdate)
      // This is older than the last update - bail out
      return false;

    update.setTimeSpec(Qt::UTC);
    updateTimestamp = update;
  }

  // Read other object arrays ==================================================================
  // Clients/pilots =================================
  QJsonObject clients = format == VATSIM_JSON3 ? obj : obj.value(QStringLiteral("clients")).toObject();
  QJsonArray pilotsArr = clients.value(QStringLiteral("pilots")).toArray();
  if(!pilotsArr.isEmpty())
    db->exec(QStringLiteral("delete from client"));

  readPilotsJson(pilotsArr);

  // Controllers/atcs and observers =================================
  QJsonArray controllersArr = format == VATSIM_JSON3 ? obj.value(QStringLiteral("controllers")).toArray() :
                              obj.value(QStringLiteral("clients")).toObject().value(QStringLiteral("atcs")).toArray();
  if(!controllersArr.isEmpty())
    db->exec(QStringLiteral("delete from atc"));
  readControllersJson(controllersArr, false /* observer */);

  if(format == IVAO_JSON2)
    readControllersJson(clients.value(QStringLiteral("observers")).toArray(), true /* observer */);

  // Servers =================================
  QJsonArray serversArr = obj.value(QStringLiteral("servers")).toArray();
  if(!serversArr.isEmpty())
    db->exec(QStringLiteral("delete from server"));
  readServersJson(serversArr, false /* voice */);

  if(format == IVAO_JSON2)
    readServersJson(obj.value(QStringLiteral("voiceServers")).toArray(), true /* voice */);

  // Prefiles - only VATSIM =================================
  if(format == VATSIM_JSON3)
    readPrefilesJson(obj);

  // ATIS - only VATSIM =================================
  // readAtisJson(obj); TODO Currently ignored since missing connection to controllers

  return true;
}

// Currently ignored
void WhazzupTextParser::readAtisJson(const QJsonObject& obj)
{
  // "atis": [
  // {
  // "cid": 1455628,
  // "name": "ANONYM",
  // "callsign": "WSSS_ATIS",
  // "frequency": "128.600",
  // "facility": 4,
  // "rating": 2,
  // "server": "SINGAPORE",
  // "visual_range": 50,
  // "atis_code": "I",
  // "text_atis": [
  // "SINGAPORE CHANGI ARPT INFO JULIET. TIME ONE SIX ZERO ZERO . EXP",
  // ...
  // "MODE STANDBY.. ACK INFO JULIET ON FIRST CTC WITH ATC."
  // ],
  // "last_updated": "2021-03-14T16:07:01.3407158Z",
  // "logon_time": "2021-03-14T11:51:43.3004568Z"
  // },
  const QJsonArray atisArr = obj.value(QStringLiteral("atis")).toArray();

  for(const QJsonValue& atisVal : atisArr)
  {
    QJsonObject atisObj = atisVal.toObject();

    // Build a column list like the one fetched from the whazzup.txt
    // ident:hostname_or_IP:location:name:clients_connection_allowed:
    // CZECH:212.67.73.150:Czech Republic:CenterEast Europe Server - sponsored by VACC-CZ:1:
    QStringList columns;
    columns.append(atisObj.value(QStringLiteral("callsign")).toVariant().toString());

    // Build a comma separated list of ATIS lines
    QStringList atisList;
    const QJsonArray atisArray = atisObj.value(QStringLiteral("text_atis")).toArray();
    for(const QJsonValue& value : atisArray)
      atisList.append(value.toString());
    atisList.removeAll(QString());
    columns.append(atisList.join(QStringLiteral(", ")));
  }
}

void WhazzupTextParser::readServersJson(const QJsonArray& serversArr, bool voice)
{
  for(const QJsonValue& serverVal : serversArr)
  {
    QJsonObject serverObj = serverVal.toObject();

    // Build a column list like the one fetched from the whazzup.txt
    // ident:hostname_or_IP:location:name:clients_connection_allowed:
    // CZECH:212.67.73.150:Czech Republic:CenterEast Europe Server - sponsored by VACC-CZ:1:
    QStringList columns;

    if(format == VATSIM_JSON3)
    {
      // "servers": [
      // {
      // "ident": "CANADA",
      // "hostname_or_ip": "165.22.239.218",
      // "location": "Toronto, Canada",
      // "name": "ANONYM",
      // "clients_connection_allowed": 1
      // },
      columns.append(serverObj.value(QStringLiteral("ident")).toVariant().toString());
      columns.append(serverObj.value(QStringLiteral("hostname_or_ip")).toVariant().toString());
      columns.append(serverObj.value(QStringLiteral("location")).toVariant().toString());
      columns.append(serverObj.value(QStringLiteral("name")).toVariant().toString());
      columns.append(QString()); // client_connections_allowed
      columns.append(QString()); // allowed_connections
      columns.append(QString()); // voice_type
    }
    else if(format == IVAO_JSON2)
    {
      // "servers": [
      // {
      // "id": "SHARD1",
      // "hostname": "shard1.net.ivao.aero",
      // "ip": "146.59.200.142",
      // "description": "IVAO SHARD1 - Network Server",
      // "countryId": "FR",
      // "currentConnections": 131,
      // "maximumConnections": 750
      // },
      columns.append(serverObj.value(QStringLiteral("id")).toVariant().toString());
      columns.append(serverObj.value(QStringLiteral("hostname")).toVariant().toString());
      columns.append(serverObj.value(QStringLiteral("countryId")).toVariant().toString());
      columns.append(serverObj.value(QStringLiteral("description")).toVariant().toString());
      columns.append(QString()); // client_connections_allowed
      columns.append(QString()); // allowed_connections
      columns.append(voice ? QStringLiteral("T") : QString()); // voice_type
    }

    parseServersSection(columns);
  }
}

void WhazzupTextParser::readControllersJson(const QJsonArray& controllersArr, bool observer)
{
  // Open and check for errors =========================================
  // Prefill with empty strings for pilots/clients delimited format
  for(const QJsonValue& atcVal : controllersArr)
  {
    QStringList columns(defaultColumns);
    QJsonObject atcObj = atcVal.toObject();
    QString callsign = atcObj.value(QStringLiteral("callsign")).toVariant().toString();
    columns[c::CALLSIGN] = callsign;
    columns[c::CLIENTTYPE] = QStringLiteral("ATC");

    if(format == VATSIM_JSON3)
    {
      // "controllers": [
      // {
      // "cid": 813331,
      // "name": "ANONYM",
      // "callsign": "EFIN_D_CTR",
      // "frequency": "121.300",
      // "facility": 6,
      // "rating": 5,
      // "server": "UK-1",
      // "visual_range": 300,
      // "text_atis": [
      // "HELSINKI CONTROL"
      // ],
      // "last_updated": "2021-03-14T16:07:00.8535377Z",
      // "logon_time": "2021-03-14T08:10:47.665987Z"
      // },
      columns[c::CID] = atcObj.value(QStringLiteral("cid")).toVariant().toString();
      columns[c::REALNAME] = atcObj.value(QStringLiteral("name")).toVariant().toString();

      columns[c::FACILITYTYPE] = atcObj.value(QStringLiteral("facility")).toVariant().toString();
      columns[c::SERVER] = atcObj.value(QStringLiteral("server")).toVariant().toString();
      columns[c::VISUALRANGE] = atcObj.value(QStringLiteral("visual_range")).toVariant().toString();

      // Read ATIS message array into linefeed separated string =========
      QStringList atisStrList;
      const QJsonArray atisArray = atcObj.value(QStringLiteral("text_atis")).toArray();
      for(const QJsonValue& value : atisArray)
        atisStrList.append(value.toString());
      atisStrList.removeAll(QString());
      columns[v::ATIS_MESSAGE] = atisStrList.join('\n');
      columns[v::TIME_LAST_ATIS_RECEIVED] = atcObj.value(QStringLiteral("last_updated")).toVariant().toString();
      columns[v::TIME_LOGON] = atcObj.value(QStringLiteral("logon_time")).toVariant().toString();

      // Get all transceivers with callsign =========
      if(transceiverMap.contains(callsign))
      {
        Rect rect;
        QSet<int> frequencies; // kHz
        frequencies.insert(atools::roundToInt(atcObj.value(QStringLiteral("frequency")).toVariant().toDouble() * 1000.f));

        // Read all frequencies and build a bounding rectangle from positions
        const QList<Transceiver> transceivers = transceiverMap.values(callsign);
        for(const Transceiver& transceiver : transceivers)
        {
          frequencies.unite(transceiver.frequency);
          rect.extend(transceiver.pos);
        }
        frequencies.remove(0);

        // Convert frequencies to mHz
        QList<float> frequenciesMhz;
        for(int f : frequencies)
          frequenciesMhz.append(f / 1000.f);

        columns[c::FREQUENCY] = atools::floatVectorToStrList(frequenciesMhz).join('&');

        // Use center of bounding rectangle as position
        columns[c::LONGITUDE] = QString::number(rect.getCenter().getLonX());
        columns[c::LATITUDE] = QString::number(rect.getCenter().getLatY());
      }
      else
      {
        // Center has no geometry in the transceiever list ====================
        columns[c::FREQUENCY] = QString::number(atcObj.value(QStringLiteral("frequency")).toVariant().toDouble());

        if(atcObj.contains(QStringLiteral("latitude")) && atcObj.contains(QStringLiteral("longitude")))
        {
          columns[c::LATITUDE] = atcObj.value(QStringLiteral("latitude")).toVariant().toString();
          columns[c::LONGITUDE] = atcObj.value(QStringLiteral("longitude")).toVariant().toString();
        }
      }
    }
    else if(format == IVAO_JSON2)
    {
      // "atcs": [
      // {
      // "time": 22493,
      // "id": 40650557,
      // "userId": 646135,
      // "callsign": "YBBN_CTR",
      // "serverId": "SHARD2",
      // "softwareTypeId": "aurora",
      // "softwareVersion": "1.2.16b",
      // "createdAt": "2021-06-20T14:54:25.000Z",
      // "atcSession": {
      // "frequency": 124.8,
      // "position": "CTR"
      // },
      // "atis": {
      // "lines": [
      // "eu17.ts.ivao.aero/YBBN_CTR",
      // "Brisbane Centre",
      // "TRL FL110 / TA 10000ft",
      // ""
      // ],
      // "revision": "O",
      // "timestamp": "2021-06-20T20:50:03.891Z"
      // },
      // "lastTrack": {
      // "distance": 1000,
      // "latitude": -27.38417,
      // "longitude": 153.1175,
      // "time": 22474,
      // "timestamp": "2021-06-20T21:08:59.133Z"
      // }
      // },
      columns[c::CID] = atcObj.value(QStringLiteral("id")).toVariant().toString();
      columns[c::REALNAME] = atcObj.value(QStringLiteral("name")).toVariant().toString();
      columns[c::SERVER] = atcObj.value(QStringLiteral("serverId")).toVariant().toString();
      columns[i::SOFTWARE_NAME] = atcObj.value(QStringLiteral("softwareTypeId")).toVariant().toString();
      columns[i::SOFTWARE_VERSION] = atcObj.value(QStringLiteral("softwareVersion")).toVariant().toString();

      // Read ATIS message array =========
      QStringList atisList;
      const QJsonArray atisArr = atcObj.value(QStringLiteral("atis")).toObject().value(QStringLiteral("lines")).toArray();
      for(const QJsonValue& value : atisArr)
        atisList.append(value.toString());
      atisList.removeAll(QString());
      columns[i::ATIS] = atisList.join('\n');
      columns[i::ATIS_TIME] = atcObj.value(QStringLiteral("atis")).toObject().value(QStringLiteral("timestamp")).toString();

      columns[i::CONNECTION_TIME] = atcObj.value(QStringLiteral("createdAt")).toVariant().toString();

      QJsonObject atcSession = atcObj.value(QStringLiteral("atcSession")).toObject();
      columns[c::FREQUENCY] = atcSession.value(QStringLiteral("frequency")).toVariant().toString();

      if(observer)
        columns[c::FACILITYTYPE] = QString::number(fac::OBSERVER);
      else
        columns[c::FACILITYTYPE] = QString::number(textToFacilityType(atcSession.value(QStringLiteral("position")).toVariant().toString()));

      QJsonObject lastTrack = atcObj.value(QStringLiteral("lastTrack")).toObject();
      columns[c::VISUALRANGE] = lastTrack.value(QStringLiteral("distance")).toVariant().toString();
      columns[c::LONGITUDE] = lastTrack.value(QStringLiteral("longitude")).toVariant().toString();
      columns[c::LATITUDE] = lastTrack.value(QStringLiteral("latitude")).toVariant().toString();
    }

    // Read line with method for delimited format
    parseSection(columns, true /* isAtc */, false /* isPrefile */, true /* isJson */);
  } // for(QJsonValue atcVal : controllersArr)
}

void WhazzupTextParser::readPilotsJson(const QJsonArray& pilotsArr)
{
  // Prefill with empty strings for pilots/clients delimited format
  for(const QJsonValue& pilotVal : pilotsArr)
  {
    QStringList columns(defaultColumns);
    QJsonObject pilotObj = pilotVal.toObject();

    columns[c::CALLSIGN] = pilotObj.value(QStringLiteral("callsign")).toString();
    columns[c::CLIENTTYPE] = QStringLiteral("PILOT");

    if(format == VATSIM_JSON3)
    {
      // "pilots": [
      // {
      // "cid": 1474512,
      // "name": "ANONYM",
      // "callsign": "ABS9481",
      // "server": "GERMANY-2",
      // "pilot_rating": 0,
      // "latitude": 33.68793,
      // "longitude": -7.51442,
      // "altitude": 2501,
      // "groundspeed": 193,
      // "transponder": "2000",
      // "heading": 163,
      // "qnh_i_hg": 30.13,
      // "qnh_mb": 1020,
      //
      // "flight_plan": {
      // "flight_rules": "I",
      // "aircraft": "B77L/H-SDE1E2E3FGHIJ2J3J4J5M1RWXY/LB1D1",
      // "aircraft_faa": "H/B77L/L",
      // "aircraft_short": "B77L",
      // "departure": "OMDB",
      // "arrival": "SBGR",
      // "alternate": "SBGL",
      // "cruise_tas": "492",
      // "altitude": "32000",
      // "deptime": "2300",
      // "enroute_time": "1442",
      // "fuel_time": "1638",
      // "remarks": "PBN/A1B1C1D1L1O1S2 DOF/210313 ... /V/",
      // "route": "NABIX3G NABIX P699 OXARI M430 KIA ... UL327 SIDUR UZ10 ILMIG DCT TBE TBE2B"
      // },
      //
      // "logon_time": "2021-03-13T22:38:09.826199Z",
      // "last_updated": "2021-03-14T16:07:00.8565953Z"
      // },
      columns[c::CID] = pilotObj.value(QStringLiteral("cid")).toVariant().toString();
      columns[c::REALNAME] = pilotObj.value(QStringLiteral("name")).toString();
      columns[c::LATITUDE] = pilotObj.value(QStringLiteral("latitude")).toVariant().toString();
      columns[c::LONGITUDE] = pilotObj.value(QStringLiteral("longitude")).toVariant().toString();
      columns[c::ALTITUDE] = pilotObj.value(QStringLiteral("altitude")).toVariant().toString();
      columns[c::GROUNDSPEED] = pilotObj.value(QStringLiteral("groundspeed")).toVariant().toString();
      columns[c::SERVER] = pilotObj.value(QStringLiteral("server")).toVariant().toString();
      columns[c::TRANSPONDER] = pilotObj.value(QStringLiteral("transponder")).toVariant().toString();

      // Insert values from flight plan object
      assignFlightplan(columns, pilotObj.value(QStringLiteral("flight_plan")).toObject());

      // IGNORED planned_depairport_lat
      // IGNORED planned_depairport_lon
      // IGNORED planned_destairport_lat
      // IGNORED planned_destairport_lon
      // atis_message
      // time_last_atis_received
      columns[v::TIME_LOGON] = pilotObj.value(QStringLiteral("logon_time")).toVariant().toString();
      columns[v::HEADING] = pilotObj.value(QStringLiteral("heading")).toVariant().toString();
    }
    else if(format == IVAO_JSON2)
    {
      // "pilots": [
      // {
      // "time": 483139,
      // "id": 40494681,
      // "userId": 396659,
      // "callsign": "ROT071",
      // "serverId": "SHARD3",
      // "softwareTypeId": "altitude",
      // "softwareVersion": "1.10.4b",
      // "createdAt": "2021-06-15T06:57:00.000Z",
      // "flightPlan": {
      // "revision": 0,
      // "aircraftId": "SR22",
      // "aircraftNumber": 1,
      // "departureId": "TNCS",
      // "arrivalId": "TFFJ",
      // "alternativeId": "TNCE",
      // "alternative2Id": null,
      // "route": "WEST MODOR SOUTH",
      // "remarks": "DOF/210615 RMK/WORLDTOUR",
      // "speed": "K0120",
      // "level": "VFR",
      // "flightRules": "V",
      // "flightType": "G",
      // "eet": 900,
      // "endurance": 360,
      // "departureTime": 25500,
      // "actualDepartureTime": 25500,
      // "peopleOnBoard": 1,
      // "createdAt": "2021-06-15T06:57:00.000Z",
      // "updatedAt": "2021-06-15T06:57:00.000Z",
      // "aircraftEquipments": "S",
      // "aircraftTransponderTypes": "S"
      // },
      // "pilotSession": {
      // "simulatorId": "MS2020"
      // },
      // "lastTrack": {
      // "altitude": 128,
      // "altitudeDifference": 0,
      // "arrivalDistance": 26.597875703772427,
      // "departureDistance": 0.046724870958816,
      // "groundSpeed": 0,
      // "heading": 205,
      // "latitude": 17.644595,
      // "longitude": -63.220497,
      // "onGround": true,
      // "state": "Boarding",
      // "time": 140,
      // "timestamp": "2021-06-15T06:59:20.538Z",
      // "transponder": 2000,
      // "transponderMode": "S"
      // }

      columns[c::CID] = pilotObj.value(QStringLiteral("userId")).toVariant().toString();

      QJsonObject lastTrack = pilotObj.value(QStringLiteral("lastTrack")).toObject();
      columns[c::LATITUDE] = lastTrack.value(QStringLiteral("latitude")).toVariant().toString();
      columns[c::LONGITUDE] = lastTrack.value(QStringLiteral("longitude")).toVariant().toString();
      columns[c::ALTITUDE] = lastTrack.value(QStringLiteral("altitude")).toVariant().toString();
      columns[c::GROUNDSPEED] = lastTrack.value(QStringLiteral("groundSpeed")).toVariant().toString();

      columns[c::SERVER] = pilotObj.value(QStringLiteral("serverId")).toVariant().toString();
      columns[c::TRANSPONDER] = lastTrack.value(QStringLiteral("transponder")).toVariant().toString();

      // Insert values from flight plan object
      assignFlightplan(columns, pilotObj.value(QStringLiteral("flightPlan")).toObject());

      columns[i::CONNECTION_TIME] = pilotObj.value(QStringLiteral("createdAt")).toVariant().toString();
      columns[i::SOFTWARE_NAME] = pilotObj.value(QStringLiteral("softwareTypeId")).toVariant().toString();
      columns[i::SOFTWARE_VERSION] = pilotObj.value(QStringLiteral("softwareVersion")).toVariant().toString();
      columns[i::HEADING] = lastTrack.value(QStringLiteral("heading")).toVariant().toString();
      columns[i::ON_GROUND] = lastTrack.value(QStringLiteral("onGround")).toVariant().toBool() ? QStringLiteral("1") : QStringLiteral("0");
      columns[i::STATE] = lastTrack.value(QStringLiteral("state")).toVariant().toString();
      columns[i::SIMULATOR] =
        pilotObj.value(QStringLiteral("pilotSession")).toObject().value(QStringLiteral("simulatorId")).toVariant().toString();
    }

    // Read line with method for delimited format
    parseSection(columns, false /* isAtc */, false /* isPrefile */, true /* isJson */);
  }
}

void WhazzupTextParser::readPrefilesJson(const QJsonObject& obj)
{
  // "prefiles": [
  // {
  // "cid": 1530915,
  // "name": "ANONYM",
  // "callsign": "TAP300",
  // "flight_plan": {
  // "flight_rules": "I",
  // "aircraft": "A320/L",
  // "aircraft_faa": "A320/L",
  // "aircraft_short": "A320",
  // "departure": "LEPA",
  // "arrival": "LIMC",
  // "alternate": "LSZH",
  // "cruise_tas": "449",
  // "altitude": "36000",
  // "deptime": "1340",
  // "enroute_time": "0119",
  // "fuel_time": "0302",
  // "remarks": "PBN/A1B1C1D1O1S1 DOF/210314 REG/N320SB EET/LFFF0026 LIMM0050 OPR/TAP PER/C RMK/TCAS SIMBRIEF /V/",
  // "route": "MEROS UN853 LUMAS UM985 EKSID M985 NOSTA/N0429F300 M985 GEN"
  // },
  // "last_updated": "2021-03-14T13:19:23.9417633Z"
  // },
  // Prefill with empty strings for pilots/clients delimited format
  const QJsonArray prefilesArr = obj.value(QStringLiteral("prefiles")).toArray();
  for(const QJsonValue& prefileVal : prefilesArr)
  {
    QStringList columns(defaultColumns);
    QJsonObject pilotObj = prefileVal.toObject();

    columns[c::CALLSIGN] = pilotObj.value(QStringLiteral("callsign")).toVariant().toString();
    columns[c::CID] = pilotObj.value(QStringLiteral("cid")).toVariant().toString();
    columns[c::REALNAME] = pilotObj.value(QStringLiteral("name")).toVariant().toString();
    columns[c::CLIENTTYPE] = QStringLiteral("PILOT");

    // Insert values from flight plan object
    assignFlightplan(columns, pilotObj.value(QStringLiteral("flight_plan")).toObject());

    // Prefill with empty strings for pilots/clients delimited format
    parseSection(columns, false /*ATC*/, true /* prefile */, true /* isJson */);
  }
}

void WhazzupTextParser::assignFlightplan(QStringList& columns, const QJsonObject& flightplanObj)
{
  columns[c::PLANNED_REMARKS] = flightplanObj.value(QStringLiteral("remarks")).toVariant().toString();
  columns[c::PLANNED_ROUTE] = flightplanObj.value(QStringLiteral("route")).toVariant().toString();

  if(format == VATSIM_JSON3)
  {
    // "flight_plan": {
    // "flight_rules": "I",
    // "aircraft": "B744/H-SDE3FGHIJ3J5J6M1M2P2RWXYZ/LB1D1",
    // "aircraft_faa": "H/B744/L",
    // "aircraft_short": "B744",
    // "departure": "SAEZ",
    // "arrival": "EGLL",
    // "alternate": "EGSS",
    // "cruise_tas": "511",
    // "altitude": "29000",
    // "deptime": "0010",
    // "enroute_time": "1251",
    // "fuel_time": "1433",
    // "remarks": "PBN/A1B1C1D1L1O1S2 NAV/RNVD1E2A1 RNP2 DAT/CPDLCX 1FANSP2PDC SUR/260B RSP180 DO...",
    // "route": "N0511F290 LANDA UW64 MCS UB688 ... UN472 BADUR UN585 FEJAC DCT JSY DCT REVTU UP87 ROXOG"
    // },

    columns[c::PLANNED_AIRCRAFT] = flightplanObj.value(QStringLiteral("aircraft")).toVariant().toString();
    columns[c::PLANNED_TASCRUISE] = flightplanObj.value(QStringLiteral("cruise_tas")).toVariant().toString();
    columns[c::PLANNED_DEPAIRPORT] = flightplanObj.value(QStringLiteral("departure")).toVariant().toString();
    columns[c::PLANNED_ALTITUDE] = flightplanObj.value(QStringLiteral("altitude")).toVariant().toString();
    columns[c::PLANNED_DESTAIRPORT] = flightplanObj.value(QStringLiteral("arrival")).toVariant().toString();

    columns[c::PLANNED_FLIGHTTYPE] = flightplanObj.value(QStringLiteral("flight_rules")).toVariant().toString();
    columns[c::PLANNED_DEPTIME] = flightplanObj.value(QStringLiteral("deptime")).toVariant().toString();

    // Split time into hours and minutes as in delimited format
    QString minEnrouteHours = flightplanObj.value(QStringLiteral("enroute_time")).toVariant().toString();
    minEnrouteHours.chop(2);
    columns[c::PLANNED_HRSENROUTE] = minEnrouteHours;
    columns[c::PLANNED_MINENROUTE] = flightplanObj.value(QStringLiteral("enroute_time")).toVariant().toString().right(2);

    // Split time into hours and minutes as in delimited format
    QString minFuelHours = flightplanObj.value(QStringLiteral("fuel_time")).toVariant().toString();
    minFuelHours.chop(2);
    columns[c::PLANNED_HRSFUEL] = minFuelHours;
    columns[c::PLANNED_MINFUEL] = flightplanObj.value(QStringLiteral("fuel_time")).toVariant().toString().right(2);

    columns[c::PLANNED_ALTAIRPORT] = flightplanObj.value(QStringLiteral("alternate")).toVariant().toString();
  }
  else if(format == IVAO_JSON2)
  {
    // "flightPlan": {
    // "revision": 0,
    // "aircraftId": "SR22",
    // "aircraftNumber": 1,
    // "departureId": "TNCS",
    // "arrivalId": "TFFJ",
    // "alternativeId": "TNCE",
    // "alternative2Id": null,
    // "route": "WEST MODOR SOUTH",
    // "remarks": "DOF/210615 RMK/WORLDTOUR",
    // "speed": "K0120",
    // "level": "VFR",
    // "flightRules": "V",
    // "flightType": "G",
    // "eet": 900,
    // "endurance": 360,
    // "departureTime": 25500,
    // "actualDepartureTime": 25500,
    // "peopleOnBoard": 1,
    // "createdAt": "2021-06-15T06:57:00.000Z",
    // "updatedAt": "2021-06-15T06:57:00.000Z",
    // "aircraftEquipments": "S",
    // "aircraftTransponderTypes": "S"
    // },
    columns[c::PLANNED_AIRCRAFT] = flightplanObj.value(QStringLiteral("aircraftId")).toVariant().toString();
    columns[c::PLANNED_TASCRUISE] = flightplanObj.value(QStringLiteral("speed")).toVariant().toString();
    columns[c::PLANNED_DEPAIRPORT] = flightplanObj.value(QStringLiteral("departureId")).toVariant().toString();
    columns[c::PLANNED_ALTITUDE] = flightplanObj.value(QStringLiteral("level")).toVariant().toString();
    columns[c::PLANNED_DESTAIRPORT] = flightplanObj.value(QStringLiteral("arrivalId")).toVariant().toString();

    columns[c::PLANNED_FLIGHTTYPE] = flightplanObj.value(QStringLiteral("flightRules")).toVariant().toString();
    columns[i::FLIGHTPLAN_TYPE_OF_FLIGHT] = flightplanObj.value(QStringLiteral("flightType")).toVariant().toString();
    columns[i::FLIGHTPLAN_PERSONS_ON_BOARD] = flightplanObj.value(QStringLiteral("peopleOnBoard")).toVariant().toString();

    if(format == IVAO_JSON2)
    {
      // All values are given in seconds
      // Split time into hours and minutes as in delimited format
      int departureTime = flightplanObj.value(QStringLiteral("departureTime")).toVariant().toInt();
      int depTimeH = departureTime / 3600;
      int depTimeM = (departureTime / 60) - (depTimeH * 60);
      columns[c::PLANNED_DEPTIME] =
        QStringLiteral("%1%2").arg(depTimeH, 2, 10, QChar('0')).arg(depTimeM, 2, 10, QChar('0'));

      int actualDepartureTime = flightplanObj.value(QStringLiteral("actualDepartureTime")).toVariant().toInt();
      int actDepTimeH = actualDepartureTime / 3600;
      int actDepTimeM = (actualDepartureTime / 60) - (actDepTimeH * 60);
      columns[c::PLANNED_ACTDEPTIME] =
        QStringLiteral("%1%2").arg(actDepTimeH, 2, 10, QChar('0')).arg(actDepTimeM, 2, 10, QChar('0'));

      int eet = flightplanObj.value(QStringLiteral("eet")).toVariant().toInt();
      int eetH = eet / 3600;
      int eetM = (eet / 60) - (eetH * 60);
      columns[c::PLANNED_HRSENROUTE] = QStringLiteral("%1").arg(eetH);
      columns[c::PLANNED_MINENROUTE] = QStringLiteral("%1").arg(eetM);

      int endurance = flightplanObj.value(QStringLiteral("endurance")).toVariant().toInt();
      int enduranceH = endurance / 3600;
      int enduranceM = (endurance / 60) - (enduranceH * 60);
      columns[c::PLANNED_HRSFUEL] = QStringLiteral("%1").arg(enduranceH);
      columns[c::PLANNED_MINFUEL] = QStringLiteral("%1").arg(enduranceM);
    }
    else
    {
      // Split time into hours and minutes as in delimited format
      columns[c::PLANNED_DEPTIME] = flightplanObj.value(QStringLiteral("departureTime")).toVariant().toString();
      columns[c::PLANNED_ACTDEPTIME] = flightplanObj.value(QStringLiteral("actualDepartureTime")).toVariant().toString();

      QString minEnrouteHours = flightplanObj.value(QStringLiteral("eet")).toVariant().toString();
      minEnrouteHours.chop(2);
      columns[c::PLANNED_HRSENROUTE] = minEnrouteHours;
      columns[c::PLANNED_MINENROUTE] = flightplanObj.value(QStringLiteral("eet")).toVariant().toString().right(2);

      QString minFuelHours = flightplanObj.value(QStringLiteral("endurance")).toVariant().toString();
      minFuelHours.chop(2);
      columns[c::PLANNED_HRSFUEL] = minFuelHours;
      columns[c::PLANNED_MINFUEL] = flightplanObj.value(QStringLiteral("endurance")).toVariant().toString().right(2);
    }

    columns[c::PLANNED_ALTAIRPORT] = flightplanObj.value(QStringLiteral("alternativeId")).toVariant().toString();
    columns[i::FLIGHTPLAN_2ND_ALTERNATE_AERODROME] = flightplanObj.value(QStringLiteral("alternative2Id")).toVariant().toString();
  }
}

bool WhazzupTextParser::readInternalDelimited(QTextStream& stream, const QDateTime& lastUpdate)
{
  QSet<QString> sections;

  // Read through file to get all sections
  while(!stream.atEnd())
  {
    QString line = stream.readLine().trimmed();
    if(line.startsWith(QStringLiteral("!")))
      // Remember section
      sections.insert(line.mid(1).toUpper().trimmed().replace(':', ""));
  }

  // Delete tables for available sections and keep others
  if(sections.contains(QStringLiteral("CLIENTS")))
  {
    db->exec(QStringLiteral("delete from client"));
    db->exec(QStringLiteral("delete from atc"));
  }

  if(sections.contains(QStringLiteral("SERVERS")))
    db->exec(QStringLiteral("delete from server where voice_type is null"));

  if(sections.contains(QStringLiteral("VOICE")) || sections.contains(QStringLiteral("VOICE_SERVERS")) ||
     sections.contains(QStringLiteral("VOICE SERVERS")))
    db->exec(QStringLiteral("delete from server where voice_type is not null"));

  // Got back and read the whole file
  stream.seek(0);

  while(!stream.atEnd())
  {
    QString line = stream.readLine().trimmed();

    // Skip comments and empty lines
    if(line.isEmpty() || line.startsWith(QStringLiteral(";")) || line.startsWith(QStringLiteral("#")))
      continue;

    if(line.startsWith(QStringLiteral("!")))
      // Remember section
      curSection = line.mid(1).toUpper().trimmed().replace(':', "");
    else
    {
      // Parse the section data  (CSV like with : separator
      if(curSection == QStringLiteral("GENERAL"))
      {
        QDateTime update = parseGeneralSection(line.split(QStringLiteral("=")));

        if(update.isValid())
        {
          if(update <= lastUpdate)
            // This is older than the last update - bail out
            return false;

          update.setTimeSpec(Qt::UTC);
          updateTimestamp = update;
        }
      }
      else if(curSection == QStringLiteral("CLIENTS"))
      {
        QStringList columns = line.split(':');

        // Check client type
        parseSection(columns, at(columns, 3, error) == QStringLiteral("ATC") /*ATC*/, false /* prefile */, false /* isJson */);
      }
      else if(curSection == QStringLiteral("PREFILE"))
        parseSection(line.split(':'), false /*ATC*/, true /* prefile */, false /* isJson */);
      else if(curSection == QStringLiteral("SERVERS"))
        parseServersSection(line.split(':'));
      else if(curSection == QStringLiteral("VOICE") || curSection == QStringLiteral("VOICE_SERVERS") ||
              curSection == QStringLiteral("VOICE SERVERS") || curSection == QStringLiteral("AIRPORTS"))
        parseVoiceSection(line.split(':'));
    }
  }

  return true;
}

QDateTime WhazzupTextParser::parseGeneralSection(const QStringList& line)
{
  // VERSION = 8
  // RELOAD = 2
  // UPDATE = 20180318175511
  // ATIS ALLOW MIN = 5
  // CONNECTED CLIENTS = 1118
  QString key = at(line, 0, error).trimmed().toUpper();
  QString value = at(line, 1, error).trimmed().toUpper();
  QDateTime update;
  if(key == QStringLiteral("VERSION"))
    version = value.toInt();
  else if(key == QStringLiteral("RELOAD")) // RELOAD  is time in minutes this file will be updated
    reload = value.toInt();
  else if(key == QStringLiteral("UPDATE")) // UPDATE is the last date and time this file has been updated. Format is yyyymmddhhnnss
    update = QDateTime::fromString(value, QStringLiteral("yyyyMMddhhmmss"));
  // else if(key == QStringLiteral("ATIS ALLOW MIN"))
  // atisAllowMin = value.toInt();
  // else if(key == QStringLiteral("CONNECTED CLIENTS"))
  // connectedClients = value.toInt();

  return update;
}

void WhazzupTextParser::parseSection(const QStringList& line, bool isAtc, bool prefile, bool isJson)
{
  // Columns in file
  // IVAO format .................................. // VATSIM format
  // 0  Callsign                                    // 0  callsign
  // 1  VID                                         // 1  cid
  // 2  Name                                        // 2  realname
  // 3  Client Type                                 // 3  clienttype
  // 4  Frequency                                   // 4  frequency
  // 5  Latitude                                    // 5  latitude
  // 6  Longitude                                   // 6  longitude
  // 7  Altitude                                    // 7  altitude
  // 8  Groundspeed                                 // 8  groundspeed
  // 9  Flightplan: Aircraft                        // 9  planned_aircraft
  // 10 Flightplan: Cruising Speed                  // 10 planned_tascruise
  // 11 Flightplan: Departure Aerodrome             // 11 planned_depairport
  // 12 Flightplan: Cruising Level                  // 12 planned_altitude
  // 13 Flightplan: Destination Aerodrome           // 13 planned_destairport
  // 14 Server                                      // 14 server
  // 15 Protocol                                    // 15 protrevision
  // 16 IGNORED                                     // 16 rating
  // 17 Transponder Code                            // 17 transponder
  // 18 Facility Type                               // 18 facilitytype
  // 19 Visual Range                                // 19 visualrange
  // 20 Flightplan: revision                        // 20 planned_revision
  // 21 Flightplan: flight rules                    // 21 planned_flighttype
  // 22 Flightplan: departure time                  // 22 planned_deptime
  // 23 Flightplan: actual departure time           // 23 planned_actdeptime
  // 24 Flightplan: EET (hours)                     // 24 planned_hrsenroute
  // 25 Flightplan: EET (minutes)                   // 25 planned_minenroute
  // 26 Flightplan: endurance (hours)               // 26 planned_hrsfuel
  // 27 Flightplan: endurance (minutes)             // 27 planned_minfuel
  // 28 Flightplan: Alternate Aerodrome             // 28 planned_altairport
  // 29 Flightplan: item 18 (other info)            // 29 planned_remarks
  // 30 Flightplan: route                           // 30 planned_route
  // 4 unused                                       // 31 IGNORED planned_depairport_lat
  // .............................................. // 34 IGNORED planned_depairport_lon
  // .............................................. // 31 IGNORED planned_destairport_lat
  // .............................................. // 32 IGNORED planned_destairport_lon
  // 33 ATIS                                        // 35 atis_message
  // 34 ATIS Time                                   // 36 time_last_atis_received
  // 35 Connection Time                             // 37 time_logon
  // 36 Software Name
  // 37 Software Version
  // 38 Administrative Version
  // 30 ATC/Pilot Version
  // 39 Flightplan: 2nd Alternate Aerodrome
  // 41 Flightplan: Persons on Board
  // 32 Flightplan: Type of Flight
  // 43 Heading                                     // 38 heading
  // 44 On ground
  // 45 Simulator
  // 46 Plane
  // .............................................. // 39 QNH_iHg
  // .............................................. // 40 QNH_Mb
  // IVAO format .................................. // VATSIM format

  atools::sql::SqlQuery *insertQuery = isAtc ? atcInsertQuery : clientInsertQuery;

  insertQuery->clearBoundValues();

  const QString callsign = at(line, c::CALLSIGN, error);
  insertQuery->bindValue(QStringLiteral(":callsign"), callsign);

  const QString vid = at(line, c::CID, error);
  insertQuery->bindValue(QStringLiteral(":vid"), vid);
  insertQuery->bindValue(QStringLiteral(":name"), convertName(at(line, c::REALNAME, error), isJson));

  // Get client type so we can check if it goes into a atc or client table
  QString clientType = at(line, c::CLIENTTYPE, error);
  bool atc = clientType == QStringLiteral("ATC");
  insertQuery->bindValue(QStringLiteral(":client_type"), clientType);

  if(atc)
  {
    // Add frequencies separated by "&" ====================
    QStringList freqStrToBind;
    const QStringList split = at(line, c::FREQUENCY, error).split(QStringLiteral("&"));
    for(const QString& str : split)
    {
      // MHz to kHz
      QString freqStr = QString::number(atools::roundToInt(str.trimmed().toDouble() * 1000.));
      if(error && (freqStr == QStringLiteral("0") || freqStr.isEmpty()))
        qWarning() << "Invalid number" << str << "at" << c::FREQUENCY << "for" << line;

      freqStrToBind.append(freqStr);
    }
    std::sort(freqStrToBind.begin(), freqStrToBind.end());
    insertQuery->bindValue(QStringLiteral(":frequency"), freqStrToBind.join('&'));
  }

  // Coordinates ====================
  Pos position;
  bool hasCoordinates;
  if(!at(line, c::LATITUDE, error).isEmpty() && !at(line, c::LONGITUDE, error).isEmpty())
  {
    position = Pos(atFloat(line, c::LONGITUDE, error), atFloat(line, c::LATITUDE, error));
    insertQuery->bindValue(QStringLiteral(":lonx"), position.getLonX());
    insertQuery->bindValue(QStringLiteral(":laty"), position.getLatY());
    hasCoordinates = true;
  }
  else
  {
    insertQuery->bindNullFloat(QStringLiteral(":laty"));
    insertQuery->bindNullFloat(QStringLiteral(":lonx"));
    hasCoordinates = false;
  }

  if(!atc)
  {
    // Decode and add altitude - store as feet ====================
    QString alt = at(line, c::ALTITUDE, error).trimmed();
    if(alt.startsWith(QStringLiteral("FL")))
      // Convert flight level FL to altitude
      insertQuery->bindValue(QStringLiteral(":altitude"), alt.mid(2).toInt() * 100);
    else if(alt.startsWith(QStringLiteral("F")))
      // Convert flight level with prefix QStringLiteral("F") to altitude
      insertQuery->bindValue(QStringLiteral(":altitude"), alt.mid(1).toInt() * 100);
    else
      insertQuery->bindValue(QStringLiteral(":altitude"), alt.toInt());

    insertQuery->bindValue(QStringLiteral(":groundspeed"), at(line, c::GROUNDSPEED, error));
    insertQuery->bindValue(QStringLiteral(":flightplan_aircraft"), at(line, c::PLANNED_AIRCRAFT, error));
    insertQuery->bindValue(QStringLiteral(":flightplan_cruising_speed"), at(line, c::PLANNED_TASCRUISE, error));
    insertQuery->bindValue(QStringLiteral(":flightplan_departure_aerodrome"), at(line, c::PLANNED_DEPAIRPORT, error));
    insertQuery->bindValue(QStringLiteral(":flightplan_cruising_level"), at(line, c::PLANNED_ALTITUDE, error));
    insertQuery->bindValue(QStringLiteral(":flightplan_destination_aerodrome"), at(line, c::PLANNED_DESTAIRPORT, error));
    insertQuery->bindValue(QStringLiteral(":transponder_code"), at(line, c::TRANSPONDER, error));
  }

  insertQuery->bindValue(QStringLiteral(":server"), at(line, c::SERVER, error));

  int visualRange = atInt(line, c::VISUALRANGE, error);

  atools::fs::online::fac::FacilityType facilityType =
    static_cast<atools::fs::online::fac::FacilityType>(atInt(line, c::FACILITYTYPE, error));
  int circleRadius = 10;

  if(atc)
  {
    insertQuery->bindValue(QStringLiteral(":facility_type"), QString::number(facilityType));

    // Convert the facility type to database airspace types ==============
    QString boundaryType, comType;
    switch(facilityType)
    {
      case atools::fs::online::fac::UNKNOWN:
        break;

      case atools::fs::online::fac::OBSERVER:
        boundaryType = QStringLiteral("OBS");
        break;

      case atools::fs::online::fac::FLIGHT_INFORMATION:
        boundaryType = QStringLiteral("C"); // Center
        comType = QStringLiteral("INF"); // Information
        break;

      case atools::fs::online::fac::DELIVERY:
        boundaryType = QStringLiteral("CL"); // Clearance
        comType = QStringLiteral("C"); // Clearance delivery
        break;

      case atools::fs::online::fac::GROUND:
        boundaryType = QStringLiteral("G");
        comType = QStringLiteral("G"); // Ground control
        break;

      case atools::fs::online::fac::TOWER:
        boundaryType = QStringLiteral("T");
        comType = QStringLiteral("T"); // Tower, Air Traffic Control
        break;

      case atools::fs::online::fac::APPROACH:
        boundaryType = QStringLiteral("A");
        comType = QStringLiteral("A"); // Approach control
        break;

      case atools::fs::online::fac::ACC:
        boundaryType = QStringLiteral("C"); // Center
        comType = QStringLiteral("CTR"); // Area control center
        break;

      case atools::fs::online::fac::DEPARTURE:
        boundaryType = QStringLiteral("D");
        comType = QStringLiteral("D"); // Departure control
        break;
    }

    if(atcSizeMap.contains(facilityType))
    {
      const AtcSizeMapValue& value = atcSizeMap.value(facilityType);
      if(value.first)
        // Use given range if valid
        circleRadius = visualRange > 0 ? visualRange : value.second;
      else
        // Always use user defined range
        circleRadius = value.second;
    }

    insertQuery->bindValue(QStringLiteral(":type"), boundaryType);
    insertQuery->bindValue(QStringLiteral(":com_type"), comType);
    insertQuery->bindValue(QStringLiteral(":radius"), circleRadius);
    insertQuery->bindValue(QStringLiteral(":visual_range"), visualRange);
  } // if(atc)
  else
  {
    // Not ATC - client ====================
    insertQuery->bindValue(QStringLiteral(":flightplan_flight_rules"), at(line, c::PLANNED_FLIGHTTYPE, error));

    QString departureTime = at(line, c::PLANNED_DEPTIME, error);
    if(!departureTime.isEmpty() && departureTime != QStringLiteral("0"))
      insertQuery->bindValue(QStringLiteral(":flightplan_departure_time"), departureTime);

    QString actualDepartureTime = at(line, c::PLANNED_ACTDEPTIME, error);
    if(!actualDepartureTime.isEmpty() && actualDepartureTime != QStringLiteral("0"))
      insertQuery->bindValue(QStringLiteral(":flightplan_actual_departure_time"), actualDepartureTime);

    // Convert two fields to minutes
    int hoursEnroute = atInt(line, c::PLANNED_HRSENROUTE, error);
    int minsEnroute = atInt(line, c::PLANNED_MINENROUTE, error);
    insertQuery->bindValue(QStringLiteral(":flightplan_enroute_minutes"), hoursEnroute * 60 + minsEnroute);

    // Convert two fields to minutes
    int hoursEndurance = atInt(line, c::PLANNED_HRSFUEL, error);
    int minsEndurance = atInt(line, c::PLANNED_MINFUEL, error);
    insertQuery->bindValue(QStringLiteral(":flightplan_endurance_minutes"), hoursEndurance * 60 + minsEndurance);

    // Calculate estimated arrival time ================================
    QTime eta;
    double enrouteMin = hoursEnroute * 60 + minsEnroute;
    if(enrouteMin > 0.)
    {
      QTime depTime = atools::timeFromHourMinStr(departureTime);
      QTime actDepTime = atools::timeFromHourMinStr(actualDepartureTime);

      if(actDepTime.isValid())
        eta = QTime::fromMSecsSinceStartOfDay(
          static_cast<int>(actDepTime.msecsSinceStartOfDay() + enrouteMin * 60. * 1000.));
      else if(depTime.isValid())
        eta = QTime::fromMSecsSinceStartOfDay(
          static_cast<int>(depTime.msecsSinceStartOfDay() + enrouteMin * 60. * 1000.));

      if(eta.isValid())
        insertQuery->bindValue(QStringLiteral(":flightplan_estimated_arrival_time"), eta.toString(QStringLiteral("HHmm")));
    }

    insertQuery->bindValue(QStringLiteral(":flightplan_alternate_aerodrome"), at(line, c::PLANNED_ALTAIRPORT, error));
    insertQuery->bindValue(QStringLiteral(":flightplan_other_info"), at(line, c::PLANNED_REMARKS, error));
    insertQuery->bindValue(QStringLiteral(":flightplan_route"), at(line, c::PLANNED_ROUTE, error));
  } // else if(atc)

  if(format == IVAO || format == IVAO_JSON2)
  {
    insertQuery->bindValue(QStringLiteral(":connection_time"),
                           parseDateTime(line, i::CONNECTION_TIME, format == IVAO_JSON2 /* jsonFormat */));

    if(atc)
    {
      insertQuery->bindValue(QStringLiteral(":atis"), convertAtisText(at(line, i::ATIS, error)));
      insertQuery->bindValue(QStringLiteral(":atis_time"), parseDateTime(line, i::ATIS_TIME, format == IVAO_JSON2 /* jsonFormat */));
    }
    else
    {
      insertQuery->bindValue(QStringLiteral(":flightplan_2nd_alternate_aerodrome"), at(line, i::FLIGHTPLAN_2ND_ALTERNATE_AERODROME, error));
      insertQuery->bindValue(QStringLiteral(":flightplan_type_of_flight"), at(line, i::FLIGHTPLAN_TYPE_OF_FLIGHT, error));
      insertQuery->bindValue(QStringLiteral(":flightplan_persons_on_board"), atInt(line, i::FLIGHTPLAN_PERSONS_ON_BOARD, error));
      insertQuery->bindValue(QStringLiteral(":heading"), atInt(line, i::HEADING, error));
      insertQuery->bindValue(QStringLiteral(":on_ground"), atInt(line, i::ON_GROUND, error));
      insertQuery->bindValue(QStringLiteral(":simulator"), at(line, i::SIMULATOR, error));

      if(format == IVAO_JSON2)
        insertQuery->bindValue(QStringLiteral(":state"), at(line, i::STATE, error));
    }
  }
  else if(format == VATSIM || format == VATSIM_JSON3)
  {
    insertQuery->bindValue(QStringLiteral(":connection_time"), parseDateTime(line, v::TIME_LOGON, format == VATSIM_JSON3 /* jsonFormat */));

    if(atc)
    {
      insertQuery->bindValue(QStringLiteral(":atis"), convertAtisText(at(line, v::ATIS_MESSAGE, error)));
      insertQuery->bindValue(QStringLiteral(":atis_time"),
                             parseDateTime(line, v::TIME_LAST_ATIS_RECEIVED, format == VATSIM_JSON3 /* jsonFormat */));
    }
    else
    {
      // On ground flag not available - determine roughly by ground speed ===============
      insertQuery->bindValue(QStringLiteral(":heading"), atInt(line, v::HEADING, error));
      // Use on ground if speed below 30 knots
      bool ok = false;
      float gs = at(line, c::GROUNDSPEED, error).toFloat(&ok);
      insertQuery->bindValue(QStringLiteral(":on_ground"), (ok && gs < 30.f) || prefile); // Either slow or prefile
      insertQuery->bindValue(QStringLiteral(":state"), prefile ? QStringLiteral("Prefile") : QString());
    }
  }

  if(atc)
  {
    if(hasCoordinates)
    {
      // Geometry for centers =============================================================================
      LineString lineString;
      if(geometryCallback)
      {
        // Try to get from callback (i.e. user airspace database)
        const LineString *ls = geometryCallback(callsign, facilityType);
        if(ls != nullptr && ls->isValidPolygon())
          // Copy cache object if valid
          lineString = *ls;
      }

      if(lineString.isEmpty())
      {
        // Nothing found or no callback - create a circle shape
        // Create a circular polygon with 10 degree segments

        // at least 1/10 nm radius
        lineString = LineString(position, atools::geo::nmToMeter(std::min(1000.f, std::max(1.f, static_cast<float>(circleRadius)))), 36);
      }

      // Add bounding rectangle
      Rect bounding = lineString.boundingRect();
      insertQuery->bindValue(QStringLiteral(":max_lonx"), bounding.getEast());
      insertQuery->bindValue(QStringLiteral(":max_laty"), bounding.getNorth());
      insertQuery->bindValue(QStringLiteral(":min_lonx"), bounding.getWest());
      insertQuery->bindValue(QStringLiteral(":min_laty"), bounding.getSouth());

      // Store geometry in same format as boundaries
      insertQuery->bindValue(QStringLiteral(":geometry"),
                             atools::fs::common::BinaryGeometry(atools::fs::util::correctBoundary(lineString)).writeToByteArray());
    }
    else
    {
      insertQuery->bindNullFloat(QStringLiteral(":max_lonx"));
      insertQuery->bindNullFloat(QStringLiteral(":max_laty"));
      insertQuery->bindNullFloat(QStringLiteral(":min_lonx"));
      insertQuery->bindNullFloat(QStringLiteral(":min_laty"));
      insertQuery->bindNullBytes(QStringLiteral(":geometry"));
    }
  }

  // =============================================================================
  // Create a hash key to identify rows with the same data - avoid id changes on reload
  // Allows only unique vid and callsign combinations
  QStringList hashKey;
  hashKey << callsign << QString::number(facilityType) << vid;

  // Look up recent database id by key or get a new one
  int id = semiPermanentId(isAtc ? atcIdMap : clientIdMap, isAtc ? curAtcId : curClientId, hashKey.join(QStringLiteral("|")));

  // qDebug() << hashKey << id;
  insertQuery->bindValue(isAtc ? QStringLiteral(":atc_id") : QStringLiteral(":client_id"), id);

  insertQuery->exec();
}

int WhazzupTextParser::semiPermanentId(QHash<QString, int>& idMap, int& curId, const QString& key)
{
  int id = idMap.value(key, -1);
  if(id == -1)
  {
    id = curId++;
    idMap.insert(key, id);
  }
  return id;
}

QDateTime WhazzupTextParser::parseDateTime(const QStringList& line, int index, bool jsonFormat)
{
  QString str = at(line, index, error);
  if(!str.isEmpty())
  {
    QDateTime datetime = jsonFormat ?
                         // 2021-03-13T22:38:09.826199Z
                         QDateTime::fromString(str, Qt::ISODateWithMs) :
                         // 20180322162024
                         QDateTime::fromString(str, QStringLiteral("yyyyMMddhhmmss"));

    if(!datetime.isValid() && error)
      qWarning() << "Invalid datetime at index" << index << line.at(index) << "in line" << line;
    return datetime;
  }
  else
    return QDateTime();
}

void WhazzupTextParser::parseServersSection(const QStringList& line)
{
  // IVAO
  // Ident    The identification of the server (unique).    n/a
  // Host name / IP     The host name or IP address of the server     n/a
  // Location     The physical location of the server.    n/a
  // Name     The descriptive name of the server.     n/a
  // Client Connections Allowed.    A flag indicating if connections to this server are allowed or not.     Boolean
  // Maximum Connection     The maximum number of connections to this server.     n/a
  //
  // VATSIM
  // ident:hostname_or_IP:location:name:clients_connection_allowed:
  // CZECH:212.67.73.150:Czech Republic:CenterEast Europe Server - sponsored by VACC-CZ:1:
  int index = 0;
  serverInsertQuery->clearBoundValues();
  serverInsertQuery->bindValue(QStringLiteral(":ident"), at(line, index++, error));
  serverInsertQuery->bindValue(QStringLiteral(":hostname"), at(line, index++, error));
  serverInsertQuery->bindValue(QStringLiteral(":location"), at(line, index++, error));
  serverInsertQuery->bindValue(QStringLiteral(":name"), at(line, index++, error));
  index++; // client_connections_allowed

  if(format == IVAO || format == IVAO_JSON2)
    index++; // allowed_connections

  if(format == IVAO_JSON2)
    serverInsertQuery->bindValue(QStringLiteral(":voice_type"), at(line, index++, error));

  serverInsertQuery->exec();
}

void WhazzupTextParser::parseVoiceSection(const QStringList& line)
{
  // VATSIM
  // hostname_or_IP:location:name:clients_connection_allowed:type_of_voice_server:
  // canada.voice.vatsim.net:Canada:VATSIM Server - Canada:1:R:

  int index = 0;
  serverInsertQuery->clearBoundValues();
  serverInsertQuery->bindValue(QStringLiteral(":hostname"), at(line, index++, error));
  serverInsertQuery->bindValue(QStringLiteral(":location"), at(line, index++, error));
  serverInsertQuery->bindValue(QStringLiteral(":name"), at(line, index++, error));
  index++; // allowed_connections
  serverInsertQuery->bindValue(QStringLiteral(":voice_type"), at(line, index++, error));
  serverInsertQuery->exec();
}

void WhazzupTextParser::initQueries()
{
  deInitQueries();

  SqlUtil util(db);

  clientInsertQuery = new SqlQuery(db);
  clientInsertQuery->prepare(util.buildInsertStatement(QStringLiteral("client"), QStringLiteral("or replace")));

  atcInsertQuery = new SqlQuery(db);
  atcInsertQuery->prepare(util.buildInsertStatement(QStringLiteral("atc"), QStringLiteral("or replace")));

  serverInsertQuery = new SqlQuery(db);
  serverInsertQuery->prepare(util.buildInsertStatement(QStringLiteral("server"), QString(), {QStringLiteral("server_id")}));
}

void WhazzupTextParser::deInitQueries()
{
  delete clientInsertQuery;
  clientInsertQuery = nullptr;

  delete atcInsertQuery;
  atcInsertQuery = nullptr;

  delete serverInsertQuery;
  serverInsertQuery = nullptr;
}

void WhazzupTextParser::resetForNewOptions()
{
  // Clear the id maps but do not reset the current ids to avoid overlaps
  atcIdMap.clear();
  clientIdMap.clear();
  reset();
}

void WhazzupTextParser::reset()
{
  curSection.clear();
  version = reload = 0;
  format = atools::fs::online::UNKNOWN;
  updateTimestamp = QDateTime();
}

QString WhazzupTextParser::convertAtisText(QString atis)
{
  if(atis.startsWith(QStringLiteral("$")))
    atis = atis.mid(1).trimmed();

  QStringList lines = atis.split(QStringLiteral("^§"));
  for(QString& line : lines)
    line = line.trimmed();
  return lines.join(QStringLiteral("\n"));
}

QString WhazzupTextParser::convertName(QString name, bool utf8)
{
  if(!utf8)
  {
#ifdef QT_CORE5COMPAT_LIB
    // Need to use compat module since Qt 6 removed the capability to use codecs
    QTextCodec *codec = QTextCodec::codecForName("Windows-1252");
    if(codec != nullptr)
      return codec->fromUnicode(name);
#endif
  }
  return name;
}

} // namespace online
} // namespace fs
} // namespace atools
