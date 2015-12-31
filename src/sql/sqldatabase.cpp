/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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
}

SqlDatabase::~SqlDatabase()
{
}

SqlDatabase& SqlDatabase::operator=(const SqlDatabase& other)
{
  db = QSqlDatabase(other.db);
  return *this;
}

void SqlDatabase::open()
{
  checkError(!isOpen(), "Opening a database that is alread open");
  checkError(db.open(), "Error opening database");
  checkError(isValid(), "Database not valid after opening");
  transaction();
}

void SqlDatabase::open(const QString& user, const QString& password)
{
  checkError(!isOpen(), "Opening a database that is alread open");
  checkError(db.open(user, password), "Error opening database");
  checkError(isValid(), "Database not valid after opening");
  transaction();
}

void SqlDatabase::close()
{
  checkError(isValid(), "Trying to close invalid database");
  checkError(isOpen(), "Closing already closed database");
  rollback();
  db.close();
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

QSqlRecord SqlDatabase::record(const QString& tablename) const
{
  checkError(isValid(), "SqlDatabase::record() on invalid database");
  checkError(isOpen(), "SqlDatabase::record() on closed database");
  return db.record(tablename);
}

SqlQuery SqlDatabase::exec(const QString& query) const
{
  SqlQuery q = SqlQuery(db.exec(query), query);
  checkError(true, "SqlDatabase::exec() error creating query");
  return q;
}

QSqlError SqlDatabase::lastError() const
{
  checkError(isValid(), "SqlDatabase::lastError() on invalid database");
  checkError(isOpen(), "SqlDatabase::lastError() on closed database");
  return db.lastError();
}

bool SqlDatabase::isValid() const
{
  return db.isValid();
}

void SqlDatabase::transaction()
{
  checkError(isValid(), "SqlDatabase::transaction() on invalid database");
  checkError(isOpen(), "SqlDatabase::transaction() on closed database");
  if(!db.driver()->hasFeature(QSqlDriver::Transactions))
    throw SqlException("Database has no transaction support");
  checkError(db.transaction(), "SqlDatabase::transaction() error");
}

void SqlDatabase::commit()
{
  checkError(isValid(), "SqlDatabase::commit() on invalid database");
  checkError(isOpen(), "SqlDatabase::commit() on closed database");
  if(!db.driver()->hasFeature(QSqlDriver::Transactions))
    throw SqlException("Database has no transaction support");
  checkError(db.commit(), "SqlDatabase::commit() error");
  transaction();
}

void SqlDatabase::rollback()
{
  checkError(isValid(), "SqlDatabase::rollback() on invalid database");
  checkError(isOpen(), "SqlDatabase::rollback() on closed database");
  if(!db.driver()->hasFeature(QSqlDriver::Transactions))
    throw SqlException("Database has no transaction support");
  checkError(db.rollback(), "SqlDatabase::rollback() error");
  transaction();
}

void SqlDatabase::setDatabaseName(const QString& name)
{
  checkError(!isOpen(), "SqlDatabase::setDatabaseName() on opened database");
  db.setDatabaseName(name);
}

void SqlDatabase::setUserName(const QString& name)
{
  checkError(!isOpen(), "SqlDatabase::setUserName() on opened database");
  db.setUserName(name);
}

void SqlDatabase::setPassword(const QString& password)
{
  checkError(!isOpen(), "SqlDatabase::setPassword() on opened database");
  db.setPassword(password);
}

void SqlDatabase::setHostName(const QString& host)
{
  checkError(!isOpen(), "SqlDatabase::setHostName() on opened database");
  db.setHostName(host);
}

void SqlDatabase::setPort(int p)
{
  checkError(!isOpen(), "SqlDatabase::setPort() on opened database");
  db.setPort(p);
}

void SqlDatabase::setConnectOptions(const QString& options)
{
  checkError(!isOpen(), "SqlDatabase::setConnectOptions() on opened database");
  db.setConnectOptions(options);
}

QString SqlDatabase::databaseName() const
{
  return db.databaseName();
}

QString SqlDatabase::userName() const
{
  return db.userName();
}

QString SqlDatabase::password() const
{
  return db.password();
}

QString SqlDatabase::hostName() const
{
  return db.hostName();
}

QString SqlDatabase::driverName() const
{
  return db.driverName();
}

int SqlDatabase::port() const
{
  return db.port();
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
    throw SqlException(db.lastError(), msg);
}

const QSqlDatabase& SqlDatabase::getQSqlDatabase() const
{
  return db;
}

} // namespace sql

} // namespace atools
