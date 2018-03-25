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
  whazzup->read(whazzupTxt, format);
}

void OnlinedataManager::readServersFromWhazzup(const QString& whazzupTxt, Format format)
{
  whazzupServers->read(whazzupTxt, format);
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
