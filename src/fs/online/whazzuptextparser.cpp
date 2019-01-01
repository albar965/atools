/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel albar965@mailbox.org
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
  QTextStream stream(&file, QIODevice::ReadOnly | QIODevice::Text);
  return read(stream, streamFormat, lastUpdate);
}

bool WhazzupTextParser::read(QTextStream& stream, Format streamFormat, const QDateTime& lastUpdate)
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
      {
        QDateTime update = parseGeneralSection(line);

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
        QStringList columns = line.split(":");

        // Check client type
        parseSection(columns, at(columns, 3, error) == "ATC" /*ATC*/, false /* prefile */);
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
  return true;
}

QDateTime WhazzupTextParser::parseGeneralSection(const QString& line)
{
  // VERSION = 8
  // RELOAD = 2
  // UPDATE = 20180318175511
  // ATIS ALLOW MIN = 5
  // CONNECTED CLIENTS = 1118
  QString key = line.section('=', 0, 0).trimmed().toUpper();
  QString value = line.section('=', 1).trimmed().toUpper();
  QDateTime update;
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

  return update;
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

  const QString callsign = at(line, index++, error);
  insertQuery->bindValue(":callsign", callsign);

  const QString vid = at(line, index++, error);
  insertQuery->bindValue(":vid", vid);
  insertQuery->bindValue(":name", convertName(at(line, index++, error)));

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
      freqStrToBind.append(QString::number(atools::roundToInt(str.trimmed().toDouble() * 1000.)));
    insertQuery->bindValue(":frequency", freqStrToBind);
  }
  index++;

  float laty = atFloat(line, index++, error);
  insertQuery->bindValue(":laty", laty);

  float lonx = atFloat(line, index++, error);
  insertQuery->bindValue(":lonx", lonx);

  if(!atc)
  {
    QString alt = at(line, index, error).trimmed();
    if(alt.startsWith("FL"))
      // Convert flight level to altitude
      insertQuery->bindValue(":altitude", alt.mid(2).toInt() * 100);
    else if(alt.startsWith("F"))
      insertQuery->bindValue(":altitude", alt.mid(1).toInt() * 100);
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
        insertQuery->bindValue(":flightplan_estimated_arrival_time", eta.toString("hhmm"));
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
      insertQuery->bindValue(":atis_time", parseDateTime(line, index++));
    }
    else
      index += 2;

    insertQuery->bindValue(":connection_time", parseDateTime(line, index++));
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
  else if(format == VATSIM)
  {
    // 31 planned_depairport_lat // 32 planned_depairport_lon // 33 planned_destairport_lat // 34 planned_destairport_lon
    index += 4;

    if(atc)
    {
      insertQuery->bindValue(":atis", convertAtisText(at(line, index++, error)));
      insertQuery->bindValue(":atis_time", parseDateTime(line, index++));
    }
    else
      index += 2;

    insertQuery->bindValue(":connection_time", parseDateTime(line, index++));
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
    // =============================================================================
    // Prepare bounding rectangle and a pre-compile circle geometry so it can be used by the same
    // airspace query classes

    // Create a circular polygon with 10 degree segments
    Pos center(lonx, laty);

    // at least 1/10 nm radius
    LineString lineString(center,
                          atools::geo::nmToMeter(std::min(1000.f, std::max(1.f, static_cast<float>(circleRadius)))),
                          36);

    // Add bounding rectancle
    Rect bounding = lineString.boundingRect();
    insertQuery->bindValue(":max_lonx", bounding.getEast());
    insertQuery->bindValue(":max_laty", bounding.getNorth());
    insertQuery->bindValue(":min_lonx", bounding.getWest());
    insertQuery->bindValue(":min_laty", bounding.getSouth());

    // Store geometry in same format as boundaries
    atools::fs::common::BinaryGeometry geo(lineString);
    insertQuery->bindValue(":geometry", geo.writeToByteArray());
  }

  // =============================================================================
  // Create sort of a hash key to identify rows with the same data
  QString hashKey = callsign + "|" + QString::number(facilityType) + "|" + vid;
  // Look up recent database id by key or get a new one
  int id = getSemiPermanentId(isAtc ? atcIdMap : clientIdMap, isAtc ? curAtcId : curClientId, hashKey);

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

QDateTime WhazzupTextParser::parseDateTime(const QStringList& line, int index)
{
  QString str = at(line, index++, error);
  if(!str.isEmpty())
  {
    QDateTime datetime = QDateTime::fromString(str, "yyyyMMddhhmmss");
    if(!datetime.isValid() && error)
      qWarning() << "Invalid datetime at index" << index << line.at(index) << "in line" << line;
    return datetime;
  }
  else
    return QDateTime();
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
  serverInsertQuery->bindValue(":ident", at(columns, index++, error));
  serverInsertQuery->bindValue(":hostname", at(columns, index++, error));
  serverInsertQuery->bindValue(":location", at(columns, index++, error));
  serverInsertQuery->bindValue(":name", at(columns, index++, error));
  serverInsertQuery->bindValue(":client_connections_allowed", atInt(columns, index++, error));

  if(format == IVAO)
    serverInsertQuery->bindValue(":allowed_connections", atInt(columns, index++, error));
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
  serverInsertQuery->bindValue(":hostname", at(columns, index++, error));
  serverInsertQuery->bindValue(":location", at(columns, index++, error));
  serverInsertQuery->bindValue(":name", at(columns, index++, error));
  serverInsertQuery->bindValue(":allowed_connections", atInt(columns, index++, error));
  serverInsertQuery->bindValue(":voice_type", at(columns, index++, error));
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
  airportInsertQuery->bindValue(":ident", at(columns, index++, error));
  airportInsertQuery->bindValue(":atis", at(columns, index++, error));
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
  version = reload = atisAllowMin = 0;
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

QString WhazzupTextParser::convertName(QString name)
{
  QTextCodec *codec = QTextCodec::codecForName("Windows-1252");
  return codec->fromUnicode(name);
}

} // namespace online
} // namespace fs
} // namespace atools
