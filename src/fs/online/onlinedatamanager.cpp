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
#include "sql/sqlutil.h"
#include "sql/sqlscript.h"

#include <QDebug>

using atools::sql::SqlDatabase;
using atools::sql::SqlQuery;
using atools::sql::SqlUtil;
using atools::sql::SqlScript;

namespace atools {
namespace fs {
namespace online {

OnlineDataManager::OnlineDataManager(sql::SqlDatabase *sqlDb)
  : db(sqlDb)
{
  status = new StatusTextParser;
  whazzup = new WhazzupTextParser(db);
}

OnlineDataManager::~OnlineDataManager()
{
  delete status;
  delete whazzup;
}

void OnlineDataManager::readFromWhazzup(const QString& whazzupTxt, atools::fs::online::Format format)
{
  whazzup->read(whazzupTxt, format);
}

void OnlineDataManager::readFromStatus(const QString& statusTxt)
{
  status->read(statusTxt);
}

QString OnlineDataManager::getWhazzupUrlFromStatus() const
{
  return status->getRandomUrl();
}

QString OnlineDataManager::getWhazzupVoiceUrlFromStatus() const
{
  return status->getRandomVoiceUrl();
}

const QString& OnlineDataManager::getMessageFromStatus()
{
  return status->getMessage();
}

int OnlineDataManager::getAtisAllowMinutesFromWhazzup() const
{
  return whazzup->getAtisAllowMinutes();
}

QDateTime OnlineDataManager::getLastUpdateTimeFromWhazzup() const
{
  return whazzup->getLastUpdateTime();
}

int OnlineDataManager::getReloadMinutesFromWhazzup() const
{
  return whazzup->getReloadMinutes();
}

bool OnlineDataManager::hasSchema()
{
  return SqlUtil(db).hasTable("client");
}

bool OnlineDataManager::hasData()
{
  return SqlUtil(db).hasTableAndRows("client");
}

void OnlineDataManager::createSchema()
{
  qDebug() << Q_FUNC_INFO;

  SqlScript script(db, true /*options->isVerbose()*/);

  script.executeScript(":/atools/resources/sql/fs/online/create_online_schema.sql");
  db->commit();
}

void OnlineDataManager::clearData()
{
  QStringList tables = db->tables();
  for(const QString& table : tables)
    db->exec("delete from " + table);
}

void OnlineDataManager::updateSchema()
{
  // for later schema evolution
}

void OnlineDataManager::dropSchema()
{
  qDebug() << Q_FUNC_INFO;

  SqlScript script(db, true /*options->isVerbose()*/);

  script.executeScript(":/atools/resources/sql/fs/online/drop_online_schema.sql");
  db->commit();
}

void OnlineDataManager::reset()
{
  status->reset();
  whazzup->reset();
}

void OnlineDataManager::initQueries()
{
  whazzup->initQueries();
}

void OnlineDataManager::deInitQueries()
{
  whazzup->deInitQueries();
}

} // namespace online
} // namespace fs
} // namespace atools
