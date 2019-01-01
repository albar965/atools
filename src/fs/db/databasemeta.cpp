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

#include "fs/db/databasemeta.h"

#include "sql/sqlquery.h"
#include "sql/sqlutil.h"

namespace atools {
namespace fs {
namespace db {

using atools::sql::SqlUtil;
using atools::sql::SqlQuery;

DatabaseMeta::DatabaseMeta(atools::sql::SqlDatabase *sqlDb)
  : db(sqlDb)
{
  init();
}

DatabaseMeta::DatabaseMeta(sql::SqlDatabase& sqlDb)
  : db(&sqlDb)
{
  init();
}

void DatabaseMeta::init()
{
  if(SqlUtil(db).hasTable("metadata"))
  {
    SqlQuery query(db);

    // Use star instead of column names to allow adding new ones
    query.exec("select * from metadata limit 1");

    if(query.next())
    {
      sql::SqlRecord rec = query.record();
      majorVersion = rec.valueInt("db_version_major");
      minorVersion = rec.valueInt("db_version_minor");
      lastLoadTime = rec.value("last_load_timestamp").toDateTime();
      sidStar = rec.valueBool("has_sid_star", false);
      airacCycle = rec.valueStr("airac_cycle", QString());
      validThrough = rec.valueStr("valid_through", QString());
      dataSource = rec.valueStr("data_source", QString());
      compilerVersion = rec.valueStr("compiler_version", QString());
      valid = true;
    }
    query.finish();
  }
}

void DatabaseMeta::updateVersion(int majorVer, int minorVer)
{
  majorVersion = majorVer;
  minorVersion = minorVer;

  SqlQuery query(db);
  query.exec("delete from metadata");

  query.prepare("insert into metadata (db_version_major, db_version_minor) values(:major, :minor)");
  query.bindValue(":major", majorVersion);
  query.bindValue(":minor", minorVersion);
  query.exec();
  db->commit();
}

void DatabaseMeta::updateVersion()
{
  updateVersion(DB_VERSION_MAJOR, DB_VERSION_MINOR);
}

void DatabaseMeta::updateAiracCycle(const QString& cycle, const QString& fromTo)
{
  airacCycle = cycle;
  SqlQuery query(db);
  query.prepare("update metadata set airac_cycle = :cycle, valid_through = :valid");
  query.bindValue(":cycle", cycle);
  query.bindValue(":valid", fromTo);
  query.exec();
  db->commit();
}

void DatabaseMeta::updateAiracCycle()
{
  updateAiracCycle(airacCycle, validThrough);
}

void DatabaseMeta::updateDataSource(const QString& src)
{
  dataSource = src;
  SqlQuery query(db);
  query.prepare("update metadata set data_source = :src");
  query.bindValue(":src", src);
  query.exec();
  db->commit();
}

void DatabaseMeta::updateDataSource()
{
  updateDataSource(dataSource);
}

void DatabaseMeta::updateCompilerVersion(const QString& versionStr)
{
  compilerVersion = versionStr;
  SqlQuery query(db);
  query.prepare("update metadata set compiler_version = :vers");
  query.bindValue(":vers", versionStr);
  query.exec();
  db->commit();
}

void DatabaseMeta::updateCompilerVersion()
{
  updateCompilerVersion(compilerVersion);
}

void DatabaseMeta::updateTimestamp()
{
  lastLoadTime = QDateTime::currentDateTime();

  SqlQuery query(db);
  query.prepare("update metadata set last_load_timestamp = :loadts");
  query.bindValue(":loadts", lastLoadTime);
  query.exec();
  db->commit();
}

void DatabaseMeta::updateFlags()
{
  bool hasSidStar = false;
  SqlQuery select(db);
  select.exec("select approach_id from approach "
              "where type = 'GPS' and suffix in ('A', 'D') and has_gps_overlay = 1 limit 1");
  if(select.next())
    hasSidStar = true;
  select.finish();

  SqlQuery query(db);
  query.prepare("update metadata set has_sid_star = :sidstar");
  query.bindValue(":sidstar", hasSidStar);
  query.exec();
  db->commit();
}

void DatabaseMeta::updateAll()
{
  updateVersion();
  updateTimestamp();
  updateFlags();
  updateAiracCycle();
  updateDataSource();
  updateCompilerVersion();
}

bool DatabaseMeta::hasSchema() const
{
  return SqlUtil(db).hasTable("airport");
}

bool DatabaseMeta::hasData() const
{
  return SqlUtil(db).hasTableAndRows("airport");
}

bool DatabaseMeta::hasAirspaces() const
{
  return SqlUtil(db).hasTableAndRows("boundary");
}

bool DatabaseMeta::isDatabaseCompatible() const
{
  return isDatabaseCompatible(DB_VERSION_MAJOR);
}

bool DatabaseMeta::isDatabaseCompatible(int major) const
{
  if(isValid())
    return major == getMajorVersion();

  return false;
}

} // namespace db
} // namespace fs
} // namespace atools
