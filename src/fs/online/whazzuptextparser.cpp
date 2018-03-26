/*****************************************************************************
* Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
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

#include <QTextCodec>

using atools::sql::SqlDatabase;
using atools::sql::SqlQuery;
using atools::sql::SqlUtil;
using atools::sql::SqlTransaction;

namespace atools {
namespace fs {
namespace online {

WhazzupTextParser::WhazzupTextParser(sql::SqlDatabase *sqlDb)
  : db(sqlDb)
{
}

WhazzupTextParser::~WhazzupTextParser()
{
  deInitQueries();
}

void WhazzupTextParser::read(QString file, Format streamFormat)
{
  QTextStream stream(&file, QIODevice::ReadOnly | QIODevice::Text);
  read(stream, streamFormat);
}

void WhazzupTextParser::read(QTextStream& stream, Format streamFormat)
{
  reset();

  // Read through file to get all sections
  QSet<QString> sections;
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
        parseGeneralSection(line);
      else if(curSection == "CLIENTS")
      {
        QStringList columns = line.split(":");

        // Check client type
        if(at(columns, 3) == "ATC")
          parseSection(columns, true /*ATC*/, false /* prefile */);
        else
          parseSection(columns, false /*ATC*/, false /* prefile */);
      }
      else if(curSection == "PREFILE")
        parseSection(line.split(":"), false /*ATC*/, true /* prefile */);
      else if(curSection == "SERVERS")
        parseServersSection(line);
      else if(curSection == "VOICE" || curSection == "VOICE_SERVERS" || curSection == "VOICE SERVERS")
        parseVoiceSection(line);
      else if(curSection == "AIRPORTS")
        parseVoiceSection(line);
    }
  }
}

void WhazzupTextParser::parseGeneralSection(const QString& line)
{
  // VERSION = 8
  // RELOAD = 2
  // UPDATE = 20180318175511
  // ATIS ALLOW MIN = 5
  // CONNECTED CLIENTS = 1118
  QString key = line.section('=', 0, 0).trimmed().toUpper();
  QString value = line.section('=', 1).trimmed().toUpper();

  if(key == "VERSION")
    version = value.toInt();
  else if(key == "RELOAD")
    reload = value.toInt();
  else if(key == "UPDATE")
    update = QDateTime::fromString(value, "yyyyMMddhhmmss");
  else if(key == "ATIS ALLOW MIN")
    atisAllowMin = value.toInt();
  // else if(key == "CONNECTED CLIENTS")
  // connectedClients = value.toInt();
}

void WhazzupTextParser::parseSection(const QStringList& line, bool isAtc, bool isPrefile)
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
  insertQuery->bindValue(":callsign", at(line, index++));
  insertQuery->bindValue(":vid", at(line, index++));
  insertQuery->bindValue(":name", convertName(at(line, index++)));

  if(!isAtc)
    insertQuery->bindValue(":prefile", isPrefile);

  // Get client type so we can check if it goes into a atc or client table
  QString clientType = at(line, index++);
  bool atc = clientType == "ATC";
  insertQuery->bindValue(":client_type", clientType);

  if(atc)
    insertQuery->bindValue(":frequency", at(line, index));
  index++;

  insertQuery->bindValue(":laty", atFloat(line, index++));
  insertQuery->bindValue(":lonx", atFloat(line, index++));

  if(!atc)
  {
    QString alt = at(line, index).trimmed();
    if(alt.startsWith("FL"))
      // Convert flight level to altitude
      insertQuery->bindValue(":altitude", alt.mid(2).toInt() * 100);
    else if(alt.startsWith("F"))
      insertQuery->bindValue(":altitude", alt.mid(1).toInt() * 100);
    else
      insertQuery->bindValue(":altitude", alt.toInt());
  }
  index++;

  if(!atc)
    insertQuery->bindValue(":groundspeed", at(line, index));
  index++;

  if(!atc)
  {
    insertQuery->bindValue(":flightplan_aircraft", at(line, index++));
    insertQuery->bindValue(":flightplan_cruising_speed", at(line, index++));
    insertQuery->bindValue(":flightplan_departure_aerodrome", at(line, index++));
    insertQuery->bindValue(":flightplan_cruising_level", at(line, index++));
    insertQuery->bindValue(":flightplan_destination_aerodrome", at(line, index++));
  }
  else
    index += 5;

  insertQuery->bindValue(":server", at(line, index++));
  insertQuery->bindValue(":protocol", at(line, index++));
  insertQuery->bindValue(":combined_rating", at(line, index++));

  if(!atc)
    insertQuery->bindValue(":transponder_code", at(line, index));
  index++;

  insertQuery->bindValue(":facility_type", atInt(line, index++));
  insertQuery->bindValue(":visual_range", atInt(line, index++));

  if(!atc)
  {
    insertQuery->bindValue(":flightplan_revision", at(line, index++));
    insertQuery->bindValue(":flightplan_flight_rules", at(line, index++));
    insertQuery->bindValue(":flightplan_departure_time", at(line, index++));
    insertQuery->bindValue(":flightplan_actual_departure_time", at(line, index++));

    // Convert two fields to minutes
    int hoursEnroute = atInt(line, index++);
    int minsEnroute = atInt(line, index++);
    insertQuery->bindValue(":flightplan_enroute_minutes", hoursEnroute * 60 + minsEnroute);

    // Convert two fields to minutes
    int hoursEndurance = atInt(line, index++);
    int minsEndurance = atInt(line, index++);
    insertQuery->bindValue(":flightplan_endurance_minutes", hoursEndurance * 60 + minsEndurance);

    insertQuery->bindValue(":flightplan_alternate_aerodrome", at(line, index++));
    insertQuery->bindValue(":flightplan_other_info", at(line, index++));
    insertQuery->bindValue(":flightplan_route", at(line, index++));
  }
  else
    index += 11;

  if(format == IVAO)
  {
    // unused fields
    index += 4;
    if(atc)
    {
      insertQuery->bindValue(":atis", convertAtisText(at(line, index++)));
      insertQuery->bindValue(":atis_time", parseDateTime(line, index++));
    }
    else
      index += 2;

    insertQuery->bindValue(":connection_time", parseDateTime(line, index++));
    insertQuery->bindValue(":software_name", at(line, index++));
    insertQuery->bindValue(":software_version", at(line, index++));
    insertQuery->bindValue(":administrative_rating", atInt(line, index++));
    insertQuery->bindValue(":atc_pilot_rating", atInt(line, index++));

    if(!atc)
    {
      insertQuery->bindValue(":flightplan_2nd_alternate_aerodrome", at(line, index++));
      insertQuery->bindValue(":flightplan_type_of_flight", at(line, index++));
      insertQuery->bindValue(":flightplan_persons_on_board", atInt(line, index++));
      insertQuery->bindValue(":heading", atInt(line, index++));
      insertQuery->bindValue(":on_ground", atInt(line, index++));
    }
    else
      index += 5;

    insertQuery->bindValue(":simulator", at(line, index++));
    if(!atc)
      insertQuery->bindValue(":plane", at(line, index));
    index++;
  }
  else if(format == VATSIM)
  {
    // 31 planned_depairport_lat // 32 planned_depairport_lon // 33 planned_destairport_lat // 34 planned_destairport_lon
    index += 4;

    if(atc)
    {
      insertQuery->bindValue(":atis", convertAtisText(at(line, index++)));
      insertQuery->bindValue(":atis_time", parseDateTime(line, index++));
    }
    else
      index += 2;

    insertQuery->bindValue(":connection_time", parseDateTime(line, index++));
    if(!atc)
      insertQuery->bindValue(":heading", atInt(line, index));
    index++;

    float qnhInHg = atFloat(line, index++);
    float qnhInMbar = atFloat(line, index++);
    insertQuery->bindValue(":qnh_mb", (atools::geo::inHgToMbar(qnhInHg) + qnhInMbar) / 2.f);
  }
  insertQuery->exec();
}

QDateTime WhazzupTextParser::parseDateTime(const QStringList& line, int index)
{
  QDateTime datetime = QDateTime::fromString(at(line, index++), "yyyyMMddhhmmss");
  if(!datetime.isValid())
    qWarning() << "Invalid datetime at index" << index << "in line" << line;
  return datetime;
}

void WhazzupTextParser::parseServersSection(const QString& line)
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
  QStringList columns = line.split(":");
  int index = 0;
  serverInsertQuery->clearBoundValues();
  serverInsertQuery->bindValue(":ident", at(columns, index++));
  serverInsertQuery->bindValue(":hostname", at(columns, index++));
  serverInsertQuery->bindValue(":location", at(columns, index++));
  serverInsertQuery->bindValue(":name", at(columns, index++));
  serverInsertQuery->bindValue(":client_connections_allowed", atInt(columns, index++));

  if(format == IVAO)
    serverInsertQuery->bindValue(":allowed_connections", atInt(columns, index++));
  serverInsertQuery->exec();
}

void WhazzupTextParser::parseVoiceSection(const QString& line)
{
  // VATSIM
  // hostname_or_IP:location:name:clients_connection_allowed:type_of_voice_server:
  // canada.voice.vatsim.net:Canada:VATSIM Server - Canada:1:R:

  QStringList columns = line.split(":");
  int index = 0;
  serverInsertQuery->clearBoundValues();
  serverInsertQuery->bindValue(":hostname", at(columns, index++));
  serverInsertQuery->bindValue(":location", at(columns, index++));
  serverInsertQuery->bindValue(":name", at(columns, index++));
  serverInsertQuery->bindValue(":allowed_connections", atInt(columns, index++));
  serverInsertQuery->bindValue(":voice_type", at(columns, index++));
  serverInsertQuery->exec();
}

void WhazzupTextParser::parseAirportSection(const QString& line)
{
  // IVAO
  // Name   Description   Unit
  // ICAO   The ICAO code of the airport.   n/a
  // ATIS   The ATIS of the airport.  n/a
  QStringList columns = line.split(":");
  int index = 0;
  airportInsertQuery->clearBoundValues();
  airportInsertQuery->bindValue(":ident", at(columns, index++));
  airportInsertQuery->bindValue(":atis", at(columns, index++));
  airportInsertQuery->exec();
}

void WhazzupTextParser::initQueries()
{
  deInitQueries();

  SqlUtil util(db);

  clientInsertQuery = new SqlQuery(db);
  clientInsertQuery->prepare(util.buildInsertStatement("client", QString(), {"client_id"}));

  atcInsertQuery = new SqlQuery(db);
  atcInsertQuery->prepare(util.buildInsertStatement("atc", QString(), {"atc_id"}));

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

QString WhazzupTextParser::convertAtisText(QString atis)
{
  if(atis.startsWith("$"))
    atis = atis.mid(1).trimmed();

  QStringList lines = atis.split("^§");
  for(QString& line : lines)
    line = line.trimmed();
  return lines.join("\n");
}

QString WhazzupTextParser::convertName(QString name)
{
  QTextCodec *codec = QTextCodec::codecForName("Windows-1252");
  return codec->fromUnicode(name);
}

} // namespace online
} // namespace fs
} // namespace atools
