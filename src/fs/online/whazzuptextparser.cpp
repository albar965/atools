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

#include "fs/online/whazzuptextparser.h"

#include "sql/sqlquery.h"
#include "sql/sqlutil.h"
#include "geo/calculations.h"
#include "sql/sqldatabase.h"
#include "geo/linestring.h"
#include "fs/common/binarygeometry.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextCodec>

using atools::sql::SqlDatabase;
using atools::sql::SqlQuery;
using atools::sql::SqlUtil;
using atools::geo::Rect;
using atools::geo::LineString;
using atools::geo::Pos;

namespace atools {
namespace fs {
namespace online {

enum VatsimColumns
{
  /*  0 */
  CALLSIGN,
  /*  1 */ CID,
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
  /* 15 */ PROTREVISION,
  /* 16 */ RATING,
  /* 17 */ TRANSPONDER,
  /* 18 */ FACILITYTYPE,
  /* 19 */ VISUALRANGE,
  /* 20 */ PLANNED_REVISION,
  /* 21 */ PLANNED_FLIGHTTYPE,
  /* 22 */ PLANNED_DEPTIME,
  /* 23 */ PLANNED_ACTDEPTIME,
  /* 24 */ PLANNED_HRSENROUTE,
  /* 25 */ PLANNED_MINENROUTE,
  /* 26 */ PLANNED_HRSFUEL,
  /* 27 */ PLANNED_MINFUEL,
  /* 28 */ PLANNED_ALTAIRPORT,
  /* 29 */ PLANNED_REMARKS,
  /* 30 */ PLANNED_ROUTE,
  /* 31 */ PLANNED_DEPAIRPORT_LAT,
  /* 32 */ PLANNED_DEPAIRPORT_LON,
  /* 33 */ PLANNED_DESTAIRPORT_LAT,
  /* 34 */ PLANNED_DESTAIRPORT_LON,
  /* 35 */ ATIS_MESSAGE,
  /* 36 */ TIME_LAST_ATIS_RECEIVED,
  /* 37 */ TIME_LOGON,
  /* 38 */ HEADING,
  /* 39 */ QNH_IHG,
  /* 40 */ QNH_MB
};

static int NUM_VATSIMCOLUMNS = QNH_MB + 1;

WhazzupTextParser::WhazzupTextParser(sql::SqlDatabase *sqlDb, bool verboseErrorReporting)
  : db(sqlDb), error(verboseErrorReporting)
{
}

WhazzupTextParser::~WhazzupTextParser()
{
  deInitQueries();
}

bool WhazzupTextParser::read(QString file, Format streamFormat, const QDateTime& lastUpdate)
{
  if(streamFormat == VATSIM_JSON3)
    return readInternalJson(file, lastUpdate);
  else
  {
    QTextStream stream(&file, QIODevice::ReadOnly | QIODevice::Text);
    return readInternalDelimited(stream, streamFormat, lastUpdate);
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
  for(QJsonValue transVal : doc.array())
  {
    QJsonObject transObj = transVal.toObject();
    QString callsign = transObj.value("callsign").toString();

    // Get all transceivers with coordinates and frequency for this callsign
    for(QJsonValue transceiverVal : transObj.value("transceivers").toArray())
    {
      QJsonObject transceiverObj = transceiverVal.toObject();
      Pos pos(transceiverObj.value("lonDeg"), transceiverObj.value("latDeg"));

      // Build and add object to multi hash with callsign as key
      Transceiver transceiver;

      // frequency in kHz
      int frequency = atools::roundToInt(transceiverObj.value("frequency").toDouble() / 1000.f);
      if(frequency < 100 && error)
        qWarning() << Q_FUNC_INFO << "Invalid frequency" << transceiverObj.value("frequency").toInt()
                   << "for" << callsign;
      else
        transceiver.frequency.insert(frequency);
      transceiver.pos = Pos(transceiverObj.value("lonDeg"), transceiverObj.value("latDeg"));
      transceiverMap.insertMulti(callsign, transceiver);
    } // for(QJsonValue transceiverVal : transObj.value("transceivers").toArray())
  } // for(QJsonValue transVal : doc.array())
}

bool WhazzupTextParser::readInternalJson(const QString& file, const QDateTime& lastUpdate)
{
  reset();

  // Format is always VATSIM JSON version 3
  format = VATSIM_JSON3;

  // Open and check for errors =============
  QJsonParseError jsonErr;
  QJsonDocument doc = QJsonDocument::fromJson(file.toUtf8(), &jsonErr);
  if(jsonErr.error != QJsonParseError::NoError)
    qWarning() << Q_FUNC_INFO << "Error reading data" << jsonErr.errorString() << "at offset" << jsonErr.offset;

  // Read time from general section =============
  // "general": {
  // "version": 3,
  // "reload": 1,
  // "update": "20210314160704",
  // "update_timestamp": "2021-03-14T16:07:04.9418979Z",
  // "connected_clients": 1857,
  // "unique_users": 1777
  // },
  QJsonObject obj = doc.object();
  QJsonObject generalObj = obj.value("general").toObject();
  QDateTime update = generalObj.value("update_timestamp").toVariant().toDateTime();

  if(update.isValid())
  {
    if(update <= lastUpdate)
      // This is older than the last update - bail out
      return false;

    updateTimestamp = update;
  }

  // Version and reload time in minutes
  version = generalObj.value("version").toInt();
  reload = generalObj.value("reload").toInt();

  // Read other object arrays ================================
  readPilotsJson(obj);
  readControllersJson(obj);
  readServersJson(obj);
  readPrefilesJson(obj);
  readAtisJson(obj);

  // TODO
  // QJsonArray facilitiesArr = obj.value("facilities").toArray();
  // QJsonArray ratingsArr = obj.value("ratings").toArray();
  // QJsonArray pilotRatingsArr = obj.value("pilot_ratings").toArray();

  return true;
}

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
  // "ILS APCH, RWY IN USE RWY 02L. RWY 02C 20C AND RWY 02R 20L",
  // "CLOSED.. SURFACE WIND ZERO ONE ZERO DEGREES, SEVEN KNOTS..",
  // "VISIBILITY ONE, ZERO KILOMETERS OR MORE.. CLOUD FEW ONE THOUSAND",
  // "EIGHT HUNDRED FEET, BROKEN THREE ZERO THOUSAND FEET. TEMPERATURE",
  // "PLUS TWO SIX. DEWPOINT PLUS TWO FOUR. QNH ONE ZERO ONE ONE. HPA.",
  // "CHANGI DEPS, INITIAL CLIMB 3000 FT. NEW TWY DESIGNATIONS IN",
  // "USE. REFER TO CHARTS.. . ACFT ON GND ARE RQSTD TO SET XPDR TO",
  // "MODE STANDBY.. ACK INFO JULIET ON FIRST CTC WITH ATC."
  // ],
  // "last_updated": "2021-03-14T16:07:01.3407158Z",
  // "logon_time": "2021-03-14T11:51:43.3004568Z"
  // },
  QJsonArray atisArr = obj.value("atis").toArray();

  if(!atisArr.isEmpty())
    db->exec("delete from airport");

  for(QJsonValue atisVal : atisArr)
  {
    QJsonObject atisObj = atisVal.toObject();

    // Build a column list like the one fetched from the whazzup.txt
    // ident:hostname_or_IP:location:name:clients_connection_allowed:
    // CZECH:212.67.73.150:Czech Republic:CenterEast Europe Server - sponsored by VACC-CZ:1:
    QStringList columns;
    columns.append(atisObj.value("callsign").toVariant().toString());

    QStringList atisList;
    for(QJsonValue value : atisObj.value("text_atis").toArray())
      atisList.append(value.toString());
    atisList.removeAll(QString());
    columns.append(atisList.join(", "));

    parseAirportSection(columns);
  }
}

void WhazzupTextParser::readServersJson(const QJsonObject& obj)
{
  // "servers": [
  // {
  // "ident": "CANADA",
  // "hostname_or_ip": "165.22.239.218",
  // "location": "Toronto, Canada",
  // "name": "ANONYM",
  // "clients_connection_allowed": 1
  // },
  QJsonArray serversArr = obj.value("servers").toArray();

  if(!serversArr.isEmpty())
    db->exec("delete from server");

  for(QJsonValue serverVal : serversArr)
  {
    QJsonObject serverObj = serverVal.toObject();

    // Build a column list like the one fetched from the whazzup.txt
    // ident:hostname_or_IP:location:name:clients_connection_allowed:
    // CZECH:212.67.73.150:Czech Republic:CenterEast Europe Server - sponsored by VACC-CZ:1:
    QStringList columns;
    columns.append(serverObj.value("ident").toVariant().toString());
    columns.append(serverObj.value("hostname_or_ip").toVariant().toString());
    columns.append(serverObj.value("location").toVariant().toString());
    columns.append(serverObj.value("name").toVariant().toString());
    columns.append(serverObj.value("clients_connection_allowed").toVariant().toString());

    parseServersSection(columns);
  }
}

void WhazzupTextParser::readControllersJson(const QJsonObject& obj)
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

  // Open and check for errors =========================================
  QJsonArray controllersArr = obj.value("controllers").toArray();
  QStringList columnsDefault;
  for(int i = 0; i < NUM_VATSIMCOLUMNS; i++)
    columnsDefault.append(QString());

  if(!controllersArr.isEmpty())
    db->exec("delete from atc");

  for(QJsonValue atcVal : controllersArr)
  {
    QStringList columns(columnsDefault);
    QJsonObject atcObj = atcVal.toObject();
    QString callsign = atcObj.value("callsign").toVariant().toString();

    columns[CALLSIGN] = callsign;
    columns[CID] = atcObj.value("cid").toVariant().toString();
    columns[REALNAME] = atcObj.value("name").toVariant().toString();

    columns[FACILITYTYPE] = atcObj.value("facility").toVariant().toString();
    columns[RATING] = atcObj.value("rating").toVariant().toString();
    columns[SERVER] = atcObj.value("server").toVariant().toString();
    columns[VISUALRANGE] = atcObj.value("visual_range").toVariant().toString();

    // Read ATIS message array =========
    QStringList atisList;
    for(QJsonValue value : atcObj.value("text_atis").toArray())
      atisList.append(value.toString());
    atisList.removeAll(QString());
    columns[ATIS_MESSAGE] = atisList.join('\n');

    columns[TIME_LOGON] = atcObj.value("logon_time").toVariant().toString();
    columns[CLIENTTYPE] = "ATC";

    // Get all transceivers with callsign =========
    if(transceiverMap.contains(callsign))
    {
      Rect rect;
      QSet<int> frequencies; // kHz
      frequencies.insert(atools::roundToInt(atcObj.value("frequency").toVariant().toDouble() * 1000.f));

      // Read all frequencies and build a bounding rectangle from positions
      QList<Transceiver> transceivers = transceiverMap.values(callsign);
      for(const Transceiver& transceiver : transceivers)
      {
        frequencies.unite(transceiver.frequency);
        rect.extend(transceiver.pos);
      }
      frequencies.remove(0);

      // Convert frequencies to mHz
      QVector<float> frequenciesMhz;
      for(int f : frequencies)
        frequenciesMhz.append(f / 1000.f);

      columns[FREQUENCY] = atools::floatVectorToStrList(frequenciesMhz).join('&');

      // Use center of bounding rectangle as position
      columns[LONGITUDE] = QString::number(rect.getCenter().getLonX());
      columns[LATITUDE] = QString::number(rect.getCenter().getLatY());

      parseSection(columns, true /* isAtc */, false /* isPrefile */, true /* isJson */);
    }
    else
    {
      // Center has no geometry in the transceiever list ====================
      columns[FREQUENCY] = QString::number(atcObj.value("frequency").toVariant().toDouble());

      if(atcObj.contains("latitude") && atcObj.contains("longitude"))
      {
        columns[LATITUDE] = atcObj.value("latitude").toVariant().toString();
        columns[LONGITUDE] = atcObj.value("longitude").toVariant().toString();
      }
      parseSection(columns, true /* isAtc */, false /* isPrefile */, true /* isJson */);
    }
  } // for(QJsonValue atcVal : controllersArr)
}

void WhazzupTextParser::readPilotsJson(const QJsonObject& obj)
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
  QStringList columnsDefault;
  for(int i = 0; i < NUM_VATSIMCOLUMNS; i++)
    columnsDefault.append(QString());

  QJsonArray pilotsArr = obj.value("pilots").toArray();

  if(!pilotsArr.isEmpty())
    db->exec("delete from client");

  for(QJsonValue pilotVal : pilotsArr)
  {
    QStringList columns(columnsDefault);
    QJsonObject pilotObj = pilotVal.toObject();

    columns[CALLSIGN] = pilotObj.value("callsign").toString();
    columns[CID] = pilotObj.value("cid").toVariant().toString();
    columns[REALNAME] = pilotObj.value("name").toString();
    columns[CLIENTTYPE] = "PILOT";
    columns[LATITUDE] = pilotObj.value("latitude").toVariant().toString();
    columns[LONGITUDE] = pilotObj.value("longitude").toVariant().toString();
    columns[ALTITUDE] = pilotObj.value("altitude").toVariant().toString();
    columns[GROUNDSPEED] = pilotObj.value("groundspeed").toVariant().toString();
    columns[SERVER] = pilotObj.value("server").toVariant().toString();
    columns[RATING] = pilotObj.value("pilot_rating").toVariant().toString();
    columns[TRANSPONDER] = pilotObj.value("transponder").toVariant().toString();

    // Insert values from flight plan object
    assignFlightplan(columns, pilotObj.value("flight_plan").toObject());

    // IGNORED planned_depairport_lat
    // IGNORED planned_depairport_lon
    // IGNORED planned_destairport_lat
    // IGNORED planned_destairport_lon
    // atis_message
    // time_last_atis_received
    columns[TIME_LOGON] = pilotObj.value("logon_time").toVariant().toString();
    columns[HEADING] = pilotObj.value("heading").toVariant().toString();
    columns[QNH_IHG] = pilotObj.value("qnh_i_hg").toVariant().toString();
    columns[QNH_MB] = pilotObj.value("qnh_mb").toVariant().toString();

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
  QStringList columnsDefault;
  for(int i = 0; i < NUM_VATSIMCOLUMNS; i++)
    columnsDefault.append(QString());

  QJsonArray prefilesArr = obj.value("prefiles").toArray();
  for(QJsonValue prefileVal : prefilesArr)
  {
    QStringList columns(columnsDefault);
    QJsonObject pilotObj = prefileVal.toObject();

    columns[CALLSIGN] = pilotObj.value("callsign").toVariant().toString();
    columns[CID] = pilotObj.value("cid").toVariant().toString();
    columns[REALNAME] = pilotObj.value("name").toVariant().toString();
    columns[CLIENTTYPE] = "PILOT";

    // Insert values from flight plan object
    assignFlightplan(columns, pilotObj.value("flight_plan").toObject());

    parseSection(columns, false /*ATC*/, true /* prefile */, true /* isJson */);
  }
}

void WhazzupTextParser::assignFlightplan(QStringList& columns, const QJsonObject& flightplanObj)
{
  columns[PLANNED_AIRCRAFT] = flightplanObj.value("aircraft").toVariant().toString();
  columns[PLANNED_TASCRUISE] = flightplanObj.value("cruise_tas").toVariant().toString();
  columns[PLANNED_DEPAIRPORT] = flightplanObj.value("departure").toVariant().toString();
  columns[PLANNED_ALTITUDE] = flightplanObj.value("altitude").toVariant().toString();
  columns[PLANNED_DESTAIRPORT] = flightplanObj.value("arrival").toVariant().toString();

  columns[PLANNED_FLIGHTTYPE] = flightplanObj.value("flight_rules").toVariant().toString();
  columns[PLANNED_DEPTIME] = flightplanObj.value("deptime").toVariant().toString();

  QString minEnrouteHours = flightplanObj.value("enroute_time").toVariant().toString();
  minEnrouteHours.chop(2);
  columns[PLANNED_HRSENROUTE] = minEnrouteHours;
  columns[PLANNED_MINENROUTE] = flightplanObj.value("enroute_time").toVariant().toString().right(2);

  QString minFuelHours = flightplanObj.value("fuel_time").toVariant().toString();
  minFuelHours.chop(2);
  columns[PLANNED_HRSFUEL] = minFuelHours;
  columns[PLANNED_MINFUEL] = flightplanObj.value("fuel_time").toVariant().toString().right(2);

  columns[PLANNED_ALTAIRPORT] = flightplanObj.value("alternate").toVariant().toString();
  columns[PLANNED_REMARKS] = flightplanObj.value("remarks").toVariant().toString();
  columns[PLANNED_ROUTE] = flightplanObj.value("route").toVariant().toString();
}

bool WhazzupTextParser::readInternalDelimited(QTextStream& stream, Format streamFormat, const QDateTime& lastUpdate)
{
  reset();

  QSet<QString> sections;

  // Read through file to get all sections
  while(!stream.atEnd())
  {
    QString line = stream.readLine().trimmed();
    if(line.startsWith("!"))
      // Remember section
      sections.insert(line.mid(1).toUpper().trimmed().replace(':', ""));
  }

  // Delete tables for available sections and keep others
  if(sections.contains("CLIENTS"))
  {
    db->exec("delete from client");
    db->exec("delete from atc");
  }

  if(sections.contains("SERVERS"))
    db->exec("delete from server where voice_type is null");

  if(sections.contains("AIRPORTS"))
    db->exec("delete from airport");

  if(sections.contains("VOICE") || sections.contains("VOICE_SERVERS") || sections.contains("VOICE SERVERS"))
    db->exec("delete from server where voice_type is not null");

  // Got back and read the whole file
  stream.seek(0);
  format = streamFormat;
  while(!stream.atEnd())
  {
    QString line = stream.readLine().trimmed();

    // Skip comments and empty lines
    if(line.isEmpty() || line.startsWith(";") || line.startsWith("#"))
      continue;

    if(line.startsWith("!"))
      // Remember section
      curSection = line.mid(1).toUpper().trimmed().replace(':', "");
    else
    {
      // Parse the section data  (CSV like with : separator
      if(curSection == "GENERAL")
      {
        QDateTime update = parseGeneralSection(line.split("="));

        if(update.isValid())
        {
          if(update <= lastUpdate)
            // This is older than the last update - bail out
            return false;

          updateTimestamp = update;
        }
      }
      else if(curSection == "CLIENTS")
      {
        QStringList columns = line.split(':');

        // Check client type
        parseSection(columns, at(columns, 3, error) == "ATC" /*ATC*/, false /* prefile */, false /* isJson */);
      }
      else if(curSection == "PREFILE")
        parseSection(line.split(':'), false /*ATC*/, true /* prefile */, false /* isJson */);
      else if(curSection == "SERVERS")
        parseServersSection(line.split(':'));
      else if(curSection == "VOICE" || curSection == "VOICE_SERVERS" || curSection == "VOICE SERVERS" ||
              curSection == "AIRPORTS")
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
  if(key == "VERSION")
    version = value.toInt();
  else if(key == "RELOAD")
    reload = value.toInt();
  else if(key == "UPDATE")
    update = QDateTime::fromString(value, "yyyyMMddhhmmss");
  // else if(key == "ATIS ALLOW MIN")
  // atisAllowMin = value.toInt();
  // else if(key == "CONNECTED CLIENTS")
  // connectedClients = value.toInt();

  return update;
}

void WhazzupTextParser::parseSection(const QStringList& line, bool isAtc, bool isPrefile, bool isJson)
{
  // Columns in file
  // IVAO format .................................. // VATSIM format
  // 0	Callsign                                    // 0  callsign
  // 1	VID                                         // 1  cid
  // 2	Name                                        // 2  realname
  // 3	Client Type                                 // 3  clienttype
  // 4	Frequency                                   // 4  frequency
  // 5  Latitude                                    // 5  latitude
  // 6  Longitude                                   // 6  longitude
  // 7  Altitude                                    // 7  altitude
  // 8  Groundspeed                                 // 8  groundspeed
  // 9  Flightplan: Aircraft                        // 9  planned_aircraft
  // 10	Flightplan: Cruising Speed                  // 10 planned_tascruise
  // 11	Flightplan: Departure Aerodrome             // 11 planned_depairport
  // 12	Flightplan: Cruising Level                  // 12 planned_altitude
  // 13	Flightplan: Destination Aerodrome           // 13 planned_destairport
  // 14	Server                                      // 14 server
  // 15	Protocol                                    // 15 protrevision
  // 16	IGNORED                                     // 16 rating
  // 17	Transponder Code                            // 17 transponder
  // 18	Facility Type                               // 18 facilitytype
  // 19	Visual Range                                // 19 visualrange
  // 20	Flightplan: revision                        // 20 planned_revision
  // 21	Flightplan: flight rules                    // 21 planned_flighttype
  // 22	Flightplan: departure time                  // 22 planned_deptime
  // 23	Flightplan: actual departure time           // 23 planned_actdeptime
  // 24	Flightplan: EET (hours)                     // 24 planned_hrsenroute
  // 25	Flightplan: EET (minutes)                   // 25 planned_minenroute
  // 26	Flightplan: endurance (hours)               // 26 planned_hrsfuel
  // 27	Flightplan: endurance (minutes)             // 27 planned_minfuel
  // 28	Flightplan: Alternate Aerodrome             // 28 planned_altairport
  // 29	Flightplan: item 18 (other info)            // 29 planned_remarks
  // 30	Flightplan: route                           // 30 planned_route
  // 4 unused                                       // 31 IGNORED planned_depairport_lat
  // .............................................. // 34 IGNORED planned_depairport_lon
  // .............................................. // 31 IGNORED planned_destairport_lat
  // .............................................. // 32 IGNORED planned_destairport_lon
  // 33	ATIS                                        // 35 atis_message
  // 34	ATIS Time                                   // 36 time_last_atis_received
  // 35	Connection Time                             // 37 time_logon
  // 36	Software Name
  // 37	Software Version
  // 38	Administrative Version
  // 30	ATC/Pilot Version
  // 39	Flightplan: 2nd Alternate Aerodrome
  // 41	Flightplan: Persons on Board
  // 32	Flightplan: Type of Flight
  // 43	Heading                                     // 38 heading
  // 44	On ground
  // 45	Simulator
  // 46	Plane
  // .............................................. // 39 QNH_iHg
  // .............................................. // 40 QNH_Mb
  // IVAO format .................................. // VATSIM format

  atools::sql::SqlQuery *insertQuery = isAtc ? atcInsertQuery : clientInsertQuery;

  int index = 0;
  insertQuery->clearBoundValues();

  const QString callsign = at(line, index++, error);
  insertQuery->bindValue(":callsign", callsign);

  const QString vid = at(line, index++, error);
  insertQuery->bindValue(":vid", vid);
  insertQuery->bindValue(":name", convertName(at(line, index++, error), isJson));

  if(!isAtc)
    insertQuery->bindValue(":prefile", isPrefile);

  // Get client type so we can check if it goes into a atc or client table
  QString clientType = at(line, index++, error);
  bool atc = clientType == "ATC";
  insertQuery->bindValue(":client_type", clientType);

  if(atc)
  {
    QStringList freqStrToBind;
    for(const QString& str : at(line, index, error).split("&"))
    {
      // MHz to kHz
      QString freqStr = QString::number(atools::roundToInt(str.trimmed().toDouble() * 1000.));
      if(error && (freqStr == "0" || freqStr.isEmpty()))
        qWarning() << "Invalid number" << str << "at" << index << "for" << line;

      freqStrToBind.append(freqStr);
    }
    std::sort(freqStrToBind.begin(), freqStrToBind.end());
    insertQuery->bindValue(":frequency", freqStrToBind.join('&'));
  }
  index++;

  Pos position;
  bool hasCoordinates;
  if(!at(line, index, error).isEmpty() && !at(line, index + 1, error).isEmpty())
  {
    position = Pos(atFloat(line, index + 1, error), atFloat(line, index, error));
    insertQuery->bindValue(":lonx", position.getLonX());
    insertQuery->bindValue(":laty", position.getLatY());
    index += 2;
    hasCoordinates = true;
  }
  else
  {
    insertQuery->bindNullFloat(":laty");
    insertQuery->bindNullFloat(":lonx");
    index += 2;
    hasCoordinates = false;
  }

  if(!atc)
  {
    QString alt = at(line, index, error).trimmed();
    if(alt.startsWith("FL"))
      // Convert flight level to altitude
      insertQuery->bindValue(":altitude", alt.midRef(2).toInt() * 100);
    else if(alt.startsWith("F"))
      insertQuery->bindValue(":altitude", alt.midRef(1).toInt() * 100);
    else
      insertQuery->bindValue(":altitude", alt.toInt());
  }
  index++;

  QString groundspeed = at(line, index, error);
  if(!atc)
    insertQuery->bindValue(":groundspeed", groundspeed);
  index++;

  if(!atc)
  {
    insertQuery->bindValue(":flightplan_aircraft", at(line, index++, error));
    insertQuery->bindValue(":flightplan_cruising_speed", at(line, index++, error));
    insertQuery->bindValue(":flightplan_departure_aerodrome", at(line, index++, error));
    insertQuery->bindValue(":flightplan_cruising_level", at(line, index++, error));
    insertQuery->bindValue(":flightplan_destination_aerodrome", at(line, index++, error));
  }
  else
    index += 5;

  insertQuery->bindValue(":server", at(line, index++, error));
  insertQuery->bindValue(":protocol", at(line, index++, error));
  insertQuery->bindValue(":combined_rating", at(line, index++, error));

  if(!atc)
    insertQuery->bindValue(":transponder_code", at(line, index, error));
  index++;

  atools::fs::online::fac::FacilityType facilityType =
    static_cast<atools::fs::online::fac::FacilityType>(atInt(line, index++, error));
  insertQuery->bindValue(":facility_type", facilityType);

  int visualRange = atInt(line, index++, error);
  int circleRadius = visualRange;

  if(atc)
  {
    // Convert the facility type to database airspace types
    QString boundaryType, comType;
    switch(facilityType)
    {
      case atools::fs::online::fac::UNKNOWN:
        break;
      case atools::fs::online::fac::OBSERVER:
        boundaryType = "OBS";
        break;
      case atools::fs::online::fac::FLIGHT_INFORMATION:
        boundaryType = "C"; // Center
        comType = "INF"; // Information
        break;
      case atools::fs::online::fac::DELIVERY:
        boundaryType = "CL"; // Clearance
        comType = "C"; // Clearance delivery
        break;
      case atools::fs::online::fac::GROUND:
        boundaryType = "G";
        comType = "G"; // Ground control
        break;
      case atools::fs::online::fac::TOWER:
        boundaryType = "T";
        comType = "T"; // Tower, Air Traffic Control
        break;
      case atools::fs::online::fac::APPROACH:
        boundaryType = "A";
        comType = "A"; // Approach control
        break;
      case atools::fs::online::fac::ACC:
        boundaryType = "C"; // Center
        comType = "CTR"; // Area control center
        break;
      case atools::fs::online::fac::DEPARTURE:
        boundaryType = "D";
        comType = "D"; // Departure control
        break;

    }

    // Value -1: not applicable / not set
    circleRadius = atcRadius.value(facilityType, -1);
    if(circleRadius == -1)
      circleRadius = visualRange;

    insertQuery->bindValue(":type", boundaryType);
    insertQuery->bindValue(":com_type", comType);
    insertQuery->bindValue(":radius", circleRadius);
  }

  insertQuery->bindValue(":visual_range", visualRange);

  if(!atc)
  {
    insertQuery->bindValue(":flightplan_revision", at(line, index++, error));
    insertQuery->bindValue(":flightplan_flight_rules", at(line, index++, error));
    QString departureTime = at(line, index++, error);
    if(!departureTime.isEmpty() && departureTime != "0")
      insertQuery->bindValue(":flightplan_departure_time", departureTime);
    QString actualDepartureTime = at(line, index++, error);
    if(!actualDepartureTime.isEmpty() && actualDepartureTime != "0")
      insertQuery->bindValue(":flightplan_actual_departure_time", actualDepartureTime);

    // Convert two fields to minutes
    int hoursEnroute = atInt(line, index++, error);
    int minsEnroute = atInt(line, index++, error);
    insertQuery->bindValue(":flightplan_enroute_minutes", hoursEnroute * 60 + minsEnroute);

    // Convert two fields to minutes
    int hoursEndurance = atInt(line, index++, error);
    int minsEndurance = atInt(line, index++, error);
    insertQuery->bindValue(":flightplan_endurance_minutes", hoursEndurance * 60 + minsEndurance);

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
        insertQuery->bindValue(":flightplan_estimated_arrival_time", eta.toString("HHmm"));
    }

    insertQuery->bindValue(":flightplan_alternate_aerodrome", at(line, index++, error));
    insertQuery->bindValue(":flightplan_other_info", at(line, index++, error));
    insertQuery->bindValue(":flightplan_route", at(line, index++, error));
  }
  else
    index += 11;

  if(format == IVAO)
  {
    // unused fields
    index += 4;
    if(atc)
    {
      insertQuery->bindValue(":atis", convertAtisText(at(line, index++, error)));
      insertQuery->bindValue(":atis_time", parseDateTime(line, index++, false /* hasMilliseconds */));
    }
    else
      index += 2;

    insertQuery->bindValue(":connection_time", parseDateTime(line, index++, false /* hasMilliseconds */));
    insertQuery->bindValue(":software_name", at(line, index++, error));
    insertQuery->bindValue(":software_version", at(line, index++, error));
    insertQuery->bindValue(":administrative_rating", atInt(line, index++, error));
    insertQuery->bindValue(":atc_pilot_rating", atInt(line, index++, error));

    if(!atc)
    {
      insertQuery->bindValue(":flightplan_2nd_alternate_aerodrome", at(line, index++, error));
      insertQuery->bindValue(":flightplan_type_of_flight", at(line, index++, error));
      insertQuery->bindValue(":flightplan_persons_on_board", atInt(line, index++, error));
      insertQuery->bindValue(":heading", atInt(line, index++, error));
      insertQuery->bindValue(":on_ground", atInt(line, index++, error));
    }
    else
      index += 5;

    insertQuery->bindValue(":simulator", at(line, index++, error));
    if(!atc)
      insertQuery->bindValue(":plane", at(line, index, error));
    index++;
  }
  else if(format == VATSIM || format == VATSIM_JSON3)
  {
    // 31 planned_depairport_lat // 32 planned_depairport_lon // 33 planned_destairport_lat // 34 planned_destairport_lon
    index += 4;

    if(atc)
    {
      insertQuery->bindValue(":atis", convertAtisText(at(line, index++, error)));
      insertQuery->bindValue(":atis_time", parseDateTime(line, index++, format == VATSIM_JSON3 /* hasMilliseconds */));
    }
    else
      index += 2;

    insertQuery->bindValue(":connection_time", parseDateTime(line, index++, format == VATSIM_JSON3 /* hasMs */));
    if(!atc)
      insertQuery->bindValue(":heading", atInt(line, index, error));
    index++;

    if(!atc)
    {
      bool ok = false;
      float gs = groundspeed.toFloat(&ok);
      insertQuery->bindValue(":on_ground", ok && gs < 30.f);
    }

    float qnhInHg = atFloat(line, index++, error);
    float qnhInMbar = atFloat(line, index++, error);
    insertQuery->bindValue(":qnh_mb", (atools::geo::inHgToMbar(qnhInHg) + qnhInMbar) / 2.f);
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
        LineString *ptr = geometryCallback(callsign, facilityType);
        if(ptr != nullptr)
          // Copy cache object
          lineString = *ptr;
      }

      if(lineString.isEmpty())
      {
        // Nothing found or no callback - create a circle shape
        // Create a circular polygon with 10 degree segments

        // at least 1/10 nm radius
        lineString = LineString(position,
                                atools::geo::nmToMeter(
                                  std::min(1000.f, std::max(1.f, static_cast<float>(circleRadius)))), 36);
      }

      // Add bounding rectangle
      Rect bounding = lineString.boundingRect();
      insertQuery->bindValue(":max_lonx", bounding.getEast());
      insertQuery->bindValue(":max_laty", bounding.getNorth());
      insertQuery->bindValue(":min_lonx", bounding.getWest());
      insertQuery->bindValue(":min_laty", bounding.getSouth());

      // Store geometry in same format as boundaries
      insertQuery->bindValue(":geometry", atools::fs::common::BinaryGeometry(lineString).writeToByteArray());
    }
    else
    {
      insertQuery->bindNullFloat(":max_lonx");
      insertQuery->bindNullFloat(":max_laty");
      insertQuery->bindNullFloat(":min_lonx");
      insertQuery->bindNullFloat(":min_laty");
      insertQuery->bindValue(":geometry", QVariant(QVariant::ByteArray));
    }
  }

  // =============================================================================
  // Create a hash key to identify rows with the same data - avoid id changes on reload
  // Allows only unique vid and callsign combinations
  QStringList hashKey;
  hashKey << callsign << QString::number(facilityType) << vid;

  // Look up recent database id by key or get a new one
  int id = getSemiPermanentId(isAtc ? atcIdMap : clientIdMap, isAtc ? curAtcId : curClientId, hashKey.join("|"));

  // qDebug() << hashKey << id;
  insertQuery->bindValue(isAtc ? ":atc_id" : ":client_id", id);

  insertQuery->exec();
}

int WhazzupTextParser::getSemiPermanentId(QHash<QString, int>& idMap, int& curId, const QString& key)
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
                         QDateTime::fromString(str, "yyyyMMddhhmmss");
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
  serverInsertQuery->bindValue(":ident", at(line, index++, error));
  serverInsertQuery->bindValue(":hostname", at(line, index++, error));
  serverInsertQuery->bindValue(":location", at(line, index++, error));
  serverInsertQuery->bindValue(":name", at(line, index++, error));
  serverInsertQuery->bindValue(":client_connections_allowed", atInt(line, index++, error));

  if(format == IVAO)
    serverInsertQuery->bindValue(":allowed_connections", atInt(line, index++, error));
  serverInsertQuery->exec();
}

void WhazzupTextParser::parseVoiceSection(const QStringList& line)
{
  // VATSIM
  // hostname_or_IP:location:name:clients_connection_allowed:type_of_voice_server:
  // canada.voice.vatsim.net:Canada:VATSIM Server - Canada:1:R:

  int index = 0;
  serverInsertQuery->clearBoundValues();
  serverInsertQuery->bindValue(":hostname", at(line, index++, error));
  serverInsertQuery->bindValue(":location", at(line, index++, error));
  serverInsertQuery->bindValue(":name", at(line, index++, error));
  serverInsertQuery->bindValue(":allowed_connections", atInt(line, index++, error));
  serverInsertQuery->bindValue(":voice_type", at(line, index++, error));
  serverInsertQuery->exec();
}

void WhazzupTextParser::parseAirportSection(const QStringList& line)
{
  // IVAO
  // Name   Description   Unit
  // ICAO   The ICAO code of the airport.   n/a
  // ATIS   The ATIS of the airport.  n/a
  int index = 0;
  airportInsertQuery->clearBoundValues();
  airportInsertQuery->bindValue(":ident", at(line, index++, error));
  airportInsertQuery->bindValue(":atis", at(line, index++, error));
  airportInsertQuery->exec();
}

void WhazzupTextParser::initQueries()
{
  deInitQueries();

  SqlUtil util(db);

  clientInsertQuery = new SqlQuery(db);
  clientInsertQuery->prepare(util.buildInsertStatement("client", "or replace"));

  atcInsertQuery = new SqlQuery(db);
  atcInsertQuery->prepare(util.buildInsertStatement("atc", "or replace"));

  serverInsertQuery = new SqlQuery(db);
  serverInsertQuery->prepare(util.buildInsertStatement("server", QString(), {"server_id"}));

  airportInsertQuery = new SqlQuery(db);
  airportInsertQuery->prepare(util.buildInsertStatement("airport", QString(), {"airport_id"}));
}

void WhazzupTextParser::deInitQueries()
{
  delete clientInsertQuery;
  clientInsertQuery = nullptr;

  delete atcInsertQuery;
  atcInsertQuery = nullptr;

  delete serverInsertQuery;
  serverInsertQuery = nullptr;

  delete airportInsertQuery;
  airportInsertQuery = nullptr;
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
  if(atis.startsWith("$"))
    atis = atis.mid(1).trimmed();

  QStringList lines = atis.split("^§");
  for(QString& line : lines)
    line = line.trimmed();
  return lines.join("\n");
}

QString WhazzupTextParser::convertName(QString name, bool utf8)
{
  if(utf8)
    return name;
  else
  {
    QTextCodec *codec = QTextCodec::codecForName("Windows-1252");
    return codec->fromUnicode(name);
  }
}

} // namespace online
} // namespace fs
} // namespace atools
