/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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
  if(SqlUtil(db).hasTable("metadata"))
  {
    SqlQuery query(db);

    query.exec("select db_version_major, db_version_minor, last_load_timestamp, has_sid_star from metadata limit 1");

    if(query.next())
    {
      majorVersion = query.value("db_version_major").toInt();
      minorVersion = query.value("db_version_minor").toInt();
      lastLoadTime = query.value("last_load_timestamp").toDateTime();
      sidStar = query.value("has_sid_star").toBool();
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

  query.prepare("insert into metadata (db_version_major, db_version_minor) "
                "values(:major, :minor)");
  query.bindValue(":major", majorVersion);
  query.bindValue(":minor", minorVersion);
  query.exec();
  db->commit();
}

void DatabaseMeta::updateVersion()
{
  updateVersion(DB_VERSION_MAJOR, DB_VERSION_MINOR);
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
  lastLoadTime = QDateTime::currentDateTime();

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
}

bool DatabaseMeta::hasSchema() const
{
  return SqlUtil(db).hasTable("airport");
}

bool DatabaseMeta::hasData() const
{
  return hasSchema() && SqlUtil(db).rowCount("airport") > 0;
}

bool DatabaseMeta::isDatabaseCompatible() const
{
  return isDatabaseCompatible(DB_VERSION_MAJOR);
}

bool DatabaseMeta::isDatabaseCompatible(int majorVersion) const
{
  if(isValid())
    return majorVersion == getMajorVersion();

  return false;
}

} // namespace db
} // namespace fs
} // namespace atools
