/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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
#include "sql/sqlrecord.h"

#include "sql/sqldatabase.h"
#include "sql/sqlquery.h"
#include "sql/sqlutil.h"
#include "util/version.h"
#include "exception.h"

#include <QDebug>

namespace atools {
namespace fs {
namespace db {

using Qt::hex;
using Qt::dec;
using Qt::endl;

using atools::sql::SqlUtil;
using atools::sql::SqlQuery;

DatabaseMeta::DatabaseMeta()
{

}

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

util::Version DatabaseMeta::getDatabaseVersion() const
{
  return util::Version(majorVersionDb, minorVersionDb);
}

util::Version DatabaseMeta::getApplicationVersion()
{
  return util::Version(DB_VERSION_MAJOR, DB_VERSION_MINOR);
}

void DatabaseMeta::init()
{
  if(db != nullptr)
  {
    SqlUtil util(db);
    if(util.hasTable("metadata"))
    {
      SqlQuery query(db);

      // Use star instead of column names to allow adding new ones
      query.exec("select * from metadata limit 1");

      if(query.next())
      {
        sql::SqlRecord rec = query.record();
        majorVersionDb = rec.valueInt("db_version_major");
        minorVersionDb = rec.valueInt("db_version_minor");
        lastLoadTime = rec.value("last_load_timestamp").toDateTime();
        sidStar = rec.valueBool("has_sid_star", false);
        airacCycle = rec.valueStr("airac_cycle", QString());
        validThrough = rec.valueStr("valid_through", QString());
        dataSource = rec.valueStr("data_source", QString());
        compilerVersion = rec.valueStr("compiler_version", QString());

        properties.clear();
        properties.readString(rec.valueStr("properties", QString()));
        valid = true;
      }
      query.finish();
    }

    schema = util.hasTable("airport");
    data = util.hasTableAndRows("airport");
    script = util.hasTableAndRows("script");
    boundary = util.hasTableAndRows("boundary");

    routeType = util.getTableColumnAndDistinctRows("airway", "route_type") > 0;
  }
}

void DatabaseMeta::deInit()
{
  db = nullptr;
}

void DatabaseMeta::updateVersion(int majorVer, int minorVer)
{
  if(db == nullptr)
    throw atools::Exception("Database is null");

  majorVersionDb = majorVer;
  minorVersionDb = minorVer;

  SqlQuery query(db);
  query.exec("delete from metadata");

  query.prepare("insert into metadata (db_version_major, db_version_minor) values(:major, :minor)");
  query.bindValue(":major", majorVersionDb);
  query.bindValue(":minor", minorVersionDb);
  query.exec();
  db->commit();
}

void DatabaseMeta::updateVersion()
{
  updateVersion(DB_VERSION_MAJOR, DB_VERSION_MINOR);
}

void DatabaseMeta::updateAiracCycle(const QString& cycle, const QString& fromTo)
{
  if(db == nullptr)
    throw atools::Exception("Database is null");

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
  if(db == nullptr)
    throw atools::Exception("Database is null");

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
  if(db == nullptr)
    throw atools::Exception("Database is null");

  compilerVersion = versionStr;
  SqlQuery query(db);
  query.prepare("update metadata set compiler_version = :vers");
  query.bindValue(":vers", versionStr);
  query.exec();
  db->commit();
}

void DatabaseMeta::updateProperties()
{
  updateProperties(properties);
}

void DatabaseMeta::updateProperties(const atools::util::Properties& props)
{
  if(db == nullptr)
    throw atools::Exception("Database is null");

  properties = props;

  SqlQuery query(db);
  query.prepare("update metadata set properties = :props");
  query.bindValue(":props", properties.writeString());
  query.exec();
  db->commit();
}

void DatabaseMeta::updateCompilerVersion()
{
  updateCompilerVersion(compilerVersion);
}

void DatabaseMeta::updateTimestamp()
{
  if(db == nullptr)
    throw atools::Exception("Database is null");

  lastLoadTime = QDateTime::currentDateTime();

  SqlQuery query(db);
  query.prepare("update metadata set last_load_timestamp = :loadts");
  query.bindValue(":loadts", lastLoadTime);
  query.exec();
  db->commit();
}

void DatabaseMeta::updateFlags()
{
  if(db == nullptr)
    throw atools::Exception("Database is null");

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
  updateProperties();
}

bool DatabaseMeta::isDatabaseCompatible() const
{
  if(valid)
  {
    if(majorVersionDb == DB_VERSION_MAJOR)
      // Major version is ok. Check if minor version is outdated
      return minorVersionDb > DB_VERSION_MINOR_OUTDATED;
    else
      return majorVersionDb == DB_VERSION_MAJOR;
  }

  return false;
}

// bool valid = false, sidStar = false, routeType = false, data = false, schema = false, script = false, boundary = false;
// QString airacCycle, validThrough, dataSource, compilerVersion;
// atools::util::Properties properties;
QDebug operator<<(QDebug out, const DatabaseMeta& meta)
{
  QDebugStateSaver saver(out);

  out.noquote().nospace()
    << "===================================" << endl
    << "DatabaseMeta name " << meta.db->getName() << endl
    << "databaseName " << meta.db->databaseName() << endl
    << "Airac Cycle " << meta.airacCycle << endl
    << "Compiler Version " << meta.compilerVersion << endl
    << "Data Source " << meta.dataSource << endl
    << "Last Load Time " << meta.lastLoadTime << endl
    << "Database Version " << meta.getDatabaseVersion() << endl
    << "Application Version " << DatabaseMeta::getApplicationVersion() << endl
    << "Valid Through " << meta.validThrough << endl
    << "valid " << meta.valid << " sidStar " << meta.sidStar << " routeType " << meta.routeType << " data " << meta.data
    << " schema " << meta.schema << " script " << meta.script << " boundary " << meta.boundary
    << " properties " << meta.properties;
  return out;
}

} // namespace db
} // namespace fs
} // namespace atools
