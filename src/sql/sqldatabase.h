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

#ifndef ATOOLS_SQL_SQLDATABASE_H
#define ATOOLS_SQL_SQLDATABASE_H

#include "sql/sqlquery.h"

#include <QSqlDriver>
#include <QSqlError>
#include <QSqlIndex>
#include <QStringList>

class QSettings;

namespace atools {
namespace sql {

/*
 * Wrapper around QSqlDatabase that adds exceptions to avoid plenty of
 * boilerplate coding. In case of error or invalid connections SqlException
 * is thrown. The driver has to support transactions.
 * This class also add a normal commit/rollback mechanism which always keeps an
 * transaction open.
 */
class SqlDatabase
{
public:
  SqlDatabase();
  SqlDatabase(const SqlDatabase& other);

  /* Create a database from settings in the given group.
   * [groupName]
   *  Type=QSQLITE
   *  ConnectionName=
   *  ConnectionOptions=
   *  HostName=
   *  Port=
   *  UserName=
   *  Password=
   */
  SqlDatabase(const QSettings& settings, const QString& groupName);
  ~SqlDatabase();

  SqlDatabase& operator=(const SqlDatabase& other);

  void open(const QStringList& pragmas = QStringList());
  void open(const QString& user, const QString& password, const QStringList& pragmas = QStringList());
  void close();
  bool isOpen() const;
  QStringList tables(QSql::TableType type = QSql::Tables) const;
  QSqlIndex primaryIndex(const QString& tablename) const;

  SqlRecord record(const QString& tablename) const;
  SqlQuery exec(const QString& query = QString()) const;
  QSqlError lastError() const;
  bool isValid() const;

  /* commit all changes. No need to call transaction before. This is done
   * automatically.
   */
  void commit();

  /* rollback all changes. No need to call transaction before. This is done
   * automatically.
   */
  void rollback();

  void setDatabaseName(const QString& name);
  void setUserName(const QString& name);
  void setPassword(const QString& password);
  void setHostName(const QString& host);
  void setPort(int p);
  void setConnectOptions(const QString& options = QString());
  QString databaseName() const;
  QString userName() const;
  QString password() const;
  QString hostName() const;
  QString driverName() const;
  int port() const;
  QString connectOptions() const;
  QString connectionName() const;
  void setNumericalPrecisionPolicy(QSql::NumericalPrecisionPolicy precisionPolicy);
  QSql::NumericalPrecisionPolicy numericalPrecisionPolicy() const;

  QSqlDriver *driver() const;

  static SqlDatabase addDatabase(const QString& type, const QString& connectionName = QLatin1String(
                                   QSqlDatabase::defaultConnection));
  static SqlDatabase addDatabase(QSqlDriver *driver, const QString& connectionName = QLatin1String(
                                   QSqlDatabase::defaultConnection));
  static SqlDatabase cloneDatabase(const SqlDatabase& other, const QString& connectionName);
  static SqlDatabase database(const QString& connectionName = QLatin1String(
                                QSqlDatabase::defaultConnection), bool open = true);
  static void removeDatabase(const QString& connectionName);
  static bool contains(const QString& connectionName = QLatin1String(QSqlDatabase::defaultConnection));
  static QStringList drivers();
  static QStringList connectionNames();
  static void registerSqlDriver(const QString& name, QSqlDriverCreatorBase *creator);
  static bool isDriverAvailable(const QString& name);

  const QSqlDatabase& getQSqlDatabase() const;

  bool isAutocommit() const;
  void setAutocommit(bool value);

  /* Rolls the current transaction back and executes the list of pragmas. Opens transaction again afteerwards. */
  void executePragmas(const QStringList& pragmas);

private:
  SqlDatabase(const QSqlDatabase& other);

  /* Keep it private so we can handle this automatically */
  void transaction();

  /* check for return value of a method and throw Exception if false */
  void checkError(bool retval = true, const QString& msg = QString()) const;

  QSqlDatabase db;
  bool autocommit = false;
};

} // namespace sql
} // namespace atools

#endif // ATOOLS_SQL_SQLDATABASE_H
