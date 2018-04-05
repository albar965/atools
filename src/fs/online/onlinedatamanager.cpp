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

#include "fs/online/onlinedatamanager.h"

#include "fs/online/statustextparser.h"
#include "fs/online/whazzuptextparser.h"

#include "sql/sqlquery.h"
#include "sql/sqltransaction.h"
#include "sql/sqlutil.h"
#include "sql/sqlscript.h"
#include "fs/sc/simconnectaircraft.h"

#include <QDebug>

using atools::sql::SqlDatabase;
using atools::sql::SqlQuery;
using atools::sql::SqlTransaction;
using atools::sql::SqlUtil;
using atools::sql::SqlScript;

namespace atools {
namespace fs {
namespace online {

OnlinedataManager::OnlinedataManager(sql::SqlDatabase *sqlDb)
  : db(sqlDb)
{
  status = new StatusTextParser;
  whazzup = new WhazzupTextParser(db);
  whazzupServers = new WhazzupTextParser(db);
}

OnlinedataManager::~OnlinedataManager()
{
  delete status;
  delete whazzup;
  delete whazzupServers;
}

void OnlinedataManager::readFromWhazzup(const QString& whazzupTxt, atools::fs::online::Format format)
{
  SqlTransaction transaction(db);
  whazzup->read(whazzupTxt, format);
  transaction.commit();
}

void OnlinedataManager::readServersFromWhazzup(const QString& whazzupTxt, Format format)
{
  SqlTransaction transaction(db);
  whazzupServers->read(whazzupTxt, format);
  transaction.commit();
}

void OnlinedataManager::readFromStatus(const QString& statusTxt)
{
  status->read(statusTxt);
}

QString OnlinedataManager::getWhazzupUrlFromStatus(bool& gzipped) const
{
  return status->getRandomUrl(gzipped);
}

QString OnlinedataManager::getWhazzupVoiceUrlFromStatus() const
{
  return status->getRandomVoiceUrl();
}

const QString& OnlinedataManager::getMessageFromStatus()
{
  return status->getMessage();
}

int OnlinedataManager::getAtisAllowMinutesFromWhazzup() const
{
  return whazzup->getAtisAllowMinutes();
}

QDateTime OnlinedataManager::getLastUpdateTimeFromWhazzup() const
{
  return whazzup->getLastUpdateTime();
}

int OnlinedataManager::getReloadMinutesFromWhazzup() const
{
  return whazzup->getReloadMinutes();
}

bool OnlinedataManager::hasSchema()
{
  return SqlUtil(db).hasTable("client");
}

bool OnlinedataManager::hasData()
{
  return SqlUtil(db).hasTableAndRows("client");
}

void OnlinedataManager::createSchema()
{
  qDebug() << Q_FUNC_INFO;

  SqlTransaction transaction(db);
  SqlScript script(db, true /*options->isVerbose()*/);

  script.executeScript(":/atools/resources/sql/fs/online/create_online_schema.sql");
  transaction.commit();
}

void OnlinedataManager::clearData()
{
  QStringList tables = db->tables();
  for(const QString& table : tables)
    db->exec("delete from " + table);
}

void OnlinedataManager::updateSchema()
{
  // for later schema evolution
}

void OnlinedataManager::dropSchema()
{
  qDebug() << Q_FUNC_INFO;

  SqlTransaction transaction(db);
  SqlScript script(db, true /*options->isVerbose()*/);

  script.executeScript(":/atools/resources/sql/fs/online/drop_online_schema.sql");
  transaction.commit();
}

void OnlinedataManager::reset()
{
  status->reset();
  whazzup->reset();
  whazzupServers->reset();
}

void OnlinedataManager::resetForNewOptions()
{
  status->reset();
  whazzup->resetForNewOptions();
  whazzupServers->resetForNewOptions();
}

void OnlinedataManager::getClientAircraftById(atools::fs::sc::SimConnectAircraft& aircraft, int id)
{
  SqlQuery query(db);
  query.prepare("select * from client where client_id = :id");
  query.bindValue(":id", id);
  query.exec();
  if(query.next())
    fillFromClient(aircraft, query.record());
  query.finish();
}

void OnlinedataManager::fillFromClient(sc::SimConnectAircraft& ac, const sql::SqlRecord& record)
{
  if(record.valueBool("prefile") || record.valueStr("client_type") != "PILOT")
    return;

  using namespace atools::fs::sc;

  ac.headingTrueDeg =
    ac.indicatedAltitudeFt =
      ac.indicatedSpeedKts =
        ac.trueAirspeedKts =
          ac.machSpeed =
            ac.verticalSpeedFeetPerMin = atools::fs::sc::SC_INVALID_FLOAT;

  ac.modelRadiusFt = ac.wingSpanFt = 0;

  ac.category = atools::fs::sc::UNKNOWN;
  ac.engineType = atools::fs::sc::UNSUPPORTED;
  ac.numberOfEngines = 0;

  ac.objectId = static_cast<quint32>(record.valueInt("client_id"));
  ac.airplaneReg = record.valueStr("callsign");

  // record.valueStr("vid");
  // record.valueStr("name");
  // record.valueStr("prefile");

  // ac.airplaneTitle,
  // ac.airplaneType,
  // ac.airplaneModel,
  // ac.airplaneReg,
  // ac.airplaneAirline,
  // ac.airplaneFlightnumber,

  ac.groundSpeedKts = record.valueFloat("groundspeed");
  ac.airplaneType = record.valueStr("flightplan_aircraft");

  // record.valueStr("flightplan_cruising_speed");

  ac.fromIdent = record.valueStr("flightplan_departure_aerodrome");

  // record.valueStr("flightplan_cruising_level");

  ac.toIdent = record.valueStr("flightplan_destination_aerodrome");

  // record.valueStr("server");
  // record.valueStr("protocol");
  // record.valueStr("combined_rating");
  // record.valueStr("transponder_code");
  // record.valueStr("facility_type");
  // record.valueStr("visual_range");
  // record.valueStr("flightplan_revision");
  // record.valueStr("flightplan_flight_rules");
  // record.valueStr("flightplan_departure_time");
  // record.valueStr("flightplan_actual_departure_time");
  // record.valueStr("flightplan_enroute_minutes");
  // record.valueStr("flightplan_endurance_minutes");
  // record.valueStr("flightplan_alternate_aerodrome");
  // record.valueStr("flightplan_other_info");
  // record.valueStr("flightplan_route");
  // record.valueStr("connection_time");
  // record.valueStr("software_name");
  // record.valueStr("software_version");
  // record.valueStr("administrative_rating");
  // record.valueStr("atc_pilot_rating");
  // record.valueStr("flightplan_2nd_alternate_aerodrome");
  // record.valueStr("flightplan_type_of_flight");
  // record.valueStr("flightplan_persons_on_board");

  ac.headingMagDeg = record.valueFloat("heading");

  ac.flags = atools::fs::sc::SIM_ONLINE;
  if(record.valueBool("on_ground"))
    ac.flags |= atools::fs::sc::ON_GROUND;

  // record.valueStr("simulator");
  // record.valueStr("plane");
  // record.valueStr("qnh_mb");

  ac.position = atools::geo::Pos(record.valueFloat("lonx"), record.valueFloat("laty"), record.valueFloat("altitude"));

}

void OnlinedataManager::initQueries()
{
  whazzup->initQueries();
  whazzupServers->initQueries();
}

void OnlinedataManager::deInitQueries()
{
  whazzupServers->deInitQueries();
}

} // namespace online
} // namespace fs
} // namespace atools
