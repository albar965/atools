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

#include "sql/sqldatabase.h"
#include "sql/sqlexception.h"
#include "sql/sqlquery.h"
#include "sql/sqlrecord.h"

#include <QSettings>
#include <QDebug>
#include <QFileInfo>
#include <QSqlIndex>
#include <QSqlDriver>
#include <QSqlError>

namespace atools {

namespace sql {

SqlDatabase::SqlDatabase()
{
}

SqlDatabase::SqlDatabase(const QSqlDatabase& other)
{
  db = QSqlDatabase(other);
}

SqlDatabase::SqlDatabase(const SqlDatabase& other)
{
  db = QSqlDatabase(other.db);
  autocommit = other.autocommit;
  readonly = other.readonly;
  automaticTransactions = other.automaticTransactions;
  name = other.name;
}

SqlDatabase::SqlDatabase(const QString& connectionName)
{
  db = QSqlDatabase::database(connectionName, false);
  name = connectionName;
}

SqlDatabase::SqlDatabase(const QSettings& settings, const QString& groupName)
{
  QString type = settings.value(groupName + "/Type").toString();
  if(type.isEmpty())
    type = "QSQLITE";

  name = settings.value(groupName + "/ConnectionName").toString();
  if(name.isEmpty())
    name = QLatin1String(QSqlDatabase::defaultConnection);
  db = QSqlDatabase::addDatabase(type, name);

  db.setConnectOptions(settings.value(groupName + "/ConnectionOptions").toString());
  db.setHostName(settings.value(groupName + "/HostName").toString());
  db.setPort(settings.value(groupName + "/Port").toInt());
  db.setUserName(settings.value(groupName + "/UserName").toString());
  db.setPassword(settings.value(groupName + "/Password").toString());
}

SqlDatabase::~SqlDatabase()
{
}

SqlDatabase& SqlDatabase::operator=(const SqlDatabase& other)
{
  db = QSqlDatabase(other.db);
  autocommit = other.autocommit;
  readonly = other.readonly;
  automaticTransactions = other.automaticTransactions;
  name = other.name;
  return *this;
}

void SqlDatabase::open(const QStringList& pragmas, bool readonlyParam)
{
  readonly = readonlyParam;

  checkError(!isOpen(), "Opening a database that is already open");

  if(readonly)
    db.setConnectOptions("QSQLITE_OPEN_READONLY");
  else
    // Have to reset since this option persist
    db.setConnectOptions();

  checkError(db.open(), "Error opening database");
  checkError(isValid(), "Database not valid after opening");

  if(readonly)
    // Clear option set above
    db.setConnectOptions();

  for(const QString& pragma : pragmas)
  {
    QSqlQuery(db).exec(pragma);
    checkError(isValid(), "Database not valid after \"" + pragma + "\"");
  }

  if(!readonly && automaticTransactions)
    transactionInternal();

  qInfo() << Q_FUNC_INFO << "Opened database" << databaseName();

#ifdef DEBUG_INFORMATION
  atools::sql::SqlQuery query(this);
  for(const QString& pragmaQuery : pragmas)
  {
    QString pragma = pragmaQuery.section("=", 0, 0);
    query.exec(pragma);
    if(query.next())
      qInfo() << Q_FUNC_INFO << pragma << "value is now: " << query.value(0).toString();
    query.finish();
  }
#endif

  recordFileMetadata();
}

void SqlDatabase::close()
{
  checkError(isValid(), "Trying to close invalid database");
  checkError(isOpen(), "Closing already closed database");
  if(!readonly && automaticTransactions)
    rollback();

  if(readonly && isFileModified())
    qWarning() << Q_FUNC_INFO << "Readonly database modified when closed" << databaseName();

  db.close();

  qInfo() << Q_FUNC_INFO << "Closed database" << databaseName();

  QString journalName(db.databaseName() + "-journal");
  QFileInfo journal(journalName);
  if(journal.exists() && journal.isFile() && journal.size() == 0)
  {
    if(QFile::remove(journalName))
      qDebug() << Q_FUNC_INFO << "Removed" << journalName;
  }
}

bool SqlDatabase::isFileModified() const
{
  if(isOpen())
  {
    QFileInfo file(databaseName());
    if(file.exists())
    {
      if(file.size() != fileSize)
        return true;

      if(fileModificationTime != file.lastModified())
        return true;
    }
  }
  return false;
}

void SqlDatabase::recordFileMetadata()
{
  fileSize = 0L;
  fileModificationTime = QDateTime();

  if(isOpen())
  {
    QFileInfo file(databaseName());
    if(file.exists())
    {
      fileSize = file.size();
      fileModificationTime = file.lastModified();
    }
  }
}

void SqlDatabase::executePragmas(const QStringList& pragmas)
{
  checkError(db.rollback(), "SqlDatabase::pragma() error");

  for(const QString& pragma : pragmas)
  {
    QSqlQuery(db).exec(pragma);
    checkError(isValid(), "Database not valid after \"" + pragma + "\"");
  }

  checkError(db.transaction(), "SqlDatabase::pragma() error");
}

void SqlDatabase::attachDatabase(const QString& file, const QString& dbName)
{
  checkError(db.rollback(), "SqlDatabase::attachDatabase() error");

  SqlQuery query(this);
  query.prepare("attach database :db as :name");
  query.bindValue(":db", file);
  query.bindValue(":name", dbName);
  query.exec();

  checkError(db.transaction(), "SqlDatabase::attachDatabase() error");
}

void SqlDatabase::detachDatabase(const QString& dbName)
{
  checkError(db.rollback(), "SqlDatabase::detachDatabase() error");

  SqlQuery query(this);
  query.prepare("detach database :name");
  query.bindValue(":name", dbName);
  query.exec();

  checkError(db.transaction(), "SqlDatabase::detachDatabase() error");
}

void SqlDatabase::vacuum()
{
  checkError(db.rollback(), "SqlDatabase::detachDatabase() error");
  exec("vacuum");
  checkError(db.transaction(), "SqlDatabase::detachDatabase() error");
}

void SqlDatabase::analyze()
{
  exec("analyze");

  /* Force reload of statistics */
  exec("analyze sqlite_master");
}

bool SqlDatabase::isOpen() const
{
  return db.isOpen();
}

QStringList SqlDatabase::tables(QSql::TableType type) const
{
  checkError(isValid(), "SqlDatabase::tables() on invalid database");
  checkError(isOpen(), "SqlDatabase::tables() on closed database");
  return db.tables(type);
}

QSqlIndex SqlDatabase::primaryIndex(const QString& tablename) const
{
  checkError(isValid(), "SqlDatabase::primaryIndex() on invalid database");
  checkError(isOpen(), "SqlDatabase::primaryIndex() on closed database");
  return db.primaryIndex(tablename);
}

SqlRecord SqlDatabase::record(const QString& tablename, const QString& prefix, const QStringList& excludeColumns) const
{
  checkError(isValid(), "SqlDatabase::record() on invalid database");
  checkError(isOpen(), "SqlDatabase::record() on closed database");
  SqlRecord rec(db.record(tablename));

  if(prefix.isEmpty())
    return rec;
  else
  {
    SqlRecord prefixedRec;
    for(int i = 0; i < rec.count(); i++)
    {
      if(!excludeColumns.contains(rec.fieldName(i)))
        prefixedRec.appendField(prefix + rec.fieldName(i), rec.fieldType(i));
    }
    return prefixedRec;
  }
}

int SqlDatabase::exec(const QString& queryText) const
{
  SqlQuery query = SqlQuery(this);
  query.exec(queryText);
  return query.numRowsAffected();
}

QSqlError SqlDatabase::lastError() const
{
  return db.lastError();
}

bool SqlDatabase::isValid() const
{
  return db.isValid();
}

void SqlDatabase::transactionInternal()
{
  checkError(isValid(), "SqlDatabase::transaction() on invalid database");
  checkError(isOpen(), "SqlDatabase::transaction() on closed database");
  if(!db.driver()->hasFeature(QSqlDriver::Transactions))
    throw SqlException(this, "Database has no transaction support");
  checkError(db.transaction(), "SqlDatabase::transaction() error");
}

void SqlDatabase::transaction()
{
  if(automaticTransactions)
    return;

  transactionInternal();
}

void SqlDatabase::commit()
{
  checkError(!readonly, "SqlDatabase::commit() on read only database");
  checkError(isValid(), "SqlDatabase::commit() on invalid database");
  checkError(isOpen(), "SqlDatabase::commit() on closed database");
  if(!db.driver()->hasFeature(QSqlDriver::Transactions))
    throw SqlException(this, "Database has no transaction support");
  checkError(db.commit(), "SqlDatabase::commit() error");

  if(automaticTransactions)
    transactionInternal();
}

void SqlDatabase::rollback()
{
  checkError(!readonly, "SqlDatabase::commit() on read only database");
  checkError(isValid(), "SqlDatabase::rollback() on invalid database");
  checkError(isOpen(), "SqlDatabase::rollback() on closed database");
  if(!db.driver()->hasFeature(QSqlDriver::Transactions))
    throw SqlException(this, "Database has no transaction support");
  checkError(db.rollback(), "SqlDatabase::rollback() error");

  if(automaticTransactions)
    transactionInternal();
}

void SqlDatabase::setDatabaseName(const QString& dbName)
{
  checkError(!isOpen(), "SqlDatabase::setDatabaseName() on opened database");
  db.setDatabaseName(dbName);
}

QString SqlDatabase::databaseName() const
{
  return db.databaseName();
}

QString SqlDatabase::connectOptions() const
{
  return db.connectOptions();
}

QString SqlDatabase::connectionName() const
{
  return db.connectionName();
}

void SqlDatabase::setNumericalPrecisionPolicy(QSql::NumericalPrecisionPolicy precisionPolicy)
{
  checkError(isValid(), "SqlDatabase::setNumericalPrecisionPolicy() on invalid database");
  db.setNumericalPrecisionPolicy(precisionPolicy);
}

QSql::NumericalPrecisionPolicy SqlDatabase::numericalPrecisionPolicy() const
{
  checkError(isValid(), "SqlDatabase::numericalPrecisionPolicy() on invalid database");
  return db.numericalPrecisionPolicy();
}

QSqlDriver *SqlDatabase::driver() const
{
  checkError(isValid(), "SqlDatabase::driver() on invalid database");
  return db.driver();
}

SqlDatabase SqlDatabase::addDatabase(const QString& type, const QString& connectionName)
{
  return SqlDatabase(QSqlDatabase::addDatabase(type, connectionName));
}

SqlDatabase SqlDatabase::addDatabase(QSqlDriver *driver, const QString& connectionName)
{
  return SqlDatabase(QSqlDatabase::addDatabase(driver, connectionName));
}

SqlDatabase SqlDatabase::cloneDatabase(const SqlDatabase& other, const QString& connectionName)
{
  return SqlDatabase(QSqlDatabase::cloneDatabase(other.db, connectionName));
}

SqlDatabase SqlDatabase::database(const QString& connectionName, bool open)
{
  return SqlDatabase(QSqlDatabase::database(connectionName, open));
}

void SqlDatabase::removeDatabase(const QString& connectionName)
{
  QSqlDatabase::removeDatabase(connectionName);
}

bool SqlDatabase::contains(const QString& connectionName)
{
  return QSqlDatabase::contains(connectionName);
}

QStringList SqlDatabase::drivers()
{
  return QSqlDatabase::drivers();
}

QStringList SqlDatabase::connectionNames()
{
  return QSqlDatabase::connectionNames();
}

void SqlDatabase::registerSqlDriver(const QString& name, QSqlDriverCreatorBase *creator)
{
  QSqlDatabase::registerSqlDriver(name, creator);
}

bool SqlDatabase::isDriverAvailable(const QString& name)
{
  return QSqlDatabase::isDriverAvailable(name);
}

void SqlDatabase::checkError(bool retval, const QString& msg) const
{
  if(!retval || db.lastError().isValid())
    throw SqlException(this, msg);
}

const QSqlDatabase& SqlDatabase::getQSqlDatabase() const
{
  return db;
}

} // namespace sql

} // namespace atools
