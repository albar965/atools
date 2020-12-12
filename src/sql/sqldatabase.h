/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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

#include <QDateTime>
#include <QSqlDatabase>
#include <QStringList>

class QSettings;

namespace atools {
namespace sql {

class SqlTransaction;
class SqlQuery;
class SqlRecord;

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
  explicit SqlDatabase(const QString& connectionName);

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
  explicit SqlDatabase(const QSettings& settings, const QString& groupName);
  ~SqlDatabase();

  SqlDatabase& operator=(const SqlDatabase& other);

  void open(const QStringList& pragmas = QStringList());
  void open(const QString& user, const QString& password, const QStringList& pragmas = QStringList());
  void close();
  bool isOpen() const;
  QStringList tables(QSql::TableType type = QSql::Tables) const;
  QSqlIndex primaryIndex(const QString& tablename) const;

  SqlRecord record(const QString& tablename, const QString& prefix = QString()) const;
  SqlQuery exec(const QString& query = QString()) const;
  QSqlError lastError() const;
  bool isValid() const;

  /* true if file was modifed since last call to recordFileMetadata() */
  bool isFileModified() const;

  /* Remember size and modification time to check if file was replaced by user */
  void recordFileMetadata();

  /* Commit all changes. No need to call transaction before. This is done
   * automatically.
   * Opens a transaction after rollback if automatic transactions are on,
   */
  void commit();

  /* Rollback all changes. No need to call transaction before. This is done
   * automatically.
   * Opens a transaction after rollback if automatic transactions are on,
   */
  void rollback();

  /* Use only if automatic transactions are off */
  void transaction();

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

  /* Sqlite only.
   * Rolls the current transaction back and executes the list of pragmas. Opens transaction again afterwards. */
  void executePragmas(const QStringList& pragmas);

  bool isAutocommit() const
  {
    return autocommit;
  }

  void setAutocommit(bool value)
  {
    autocommit = value;
  }

  bool isReadonly() const
  {
    return readonly;
  }

  void setReadonly(bool value = true)
  {
    readonly = value;
  }

  /* Sqlite only.
   * Rolls the current transaction back and attaches or detaches a database. Opens transaction again afterwards. */
  void attachDatabase(const QString& file, const QString& name);
  void detachDatabase(const QString& name);

  /* Sqlite only. Compresses the database */
  void vacuum();

  /* Sqlite only. Gather schema statistics for query optimization. */
  void analyze();

  bool isAutomaticTransactions() const
  {
    return automaticTransactions;
  }

  void setAutomaticTransactions(bool value)
  {
    automaticTransactions = value;
  }

private:
  friend class atools::sql::SqlTransaction;

  SqlDatabase(const QSqlDatabase& other);

  /* check for return value of a method and throw Exception if false */
  void checkError(bool retval = true, const QString& msg = QString()) const;
  void transactionInternal();

  QSqlDatabase db;
  bool autocommit = false, readonly = false, automaticTransactions = true;
  QString name;

  qint64 fileSize = 0L;
  QDateTime fileModificationTime;
};

} // namespace sql
} // namespace atools

#endif // ATOOLS_SQL_SQLDATABASE_H
