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

#ifndef ATOOLS_SQL_SQLQUERY_H
#define ATOOLS_SQL_SQLQUERY_H

#include <QDateTime>
#include <QSqlQuery>
#include <QVariant>

class QSqlResult;

namespace atools {
namespace sql {

class SqlDatabase;
class SqlRecord;
class SqlRecordVector;

/*
 * Wrapper around QSqlQuery that adds exceptions to avoid plenty of
 * boilerplate coding. In case of error or invalid connections SqlException
 * is thrown.
 * Exception is also thrown if a bind() or value() receive an invalid
 * name or index.
 */
class SqlQuery
{
public:
  explicit SqlQuery(QSqlResult *r);
  explicit SqlQuery(const QString& queryStr, const SqlDatabase& sqlDb);
  explicit SqlQuery(const QString& queryStr, const SqlDatabase *sqlDb);
  explicit SqlQuery(const SqlDatabase& db);
  explicit SqlQuery(const SqlDatabase *sqlDb);

  SqlQuery(const SqlQuery& other);
  SqlQuery& operator=(const SqlQuery& other);

  ~SqlQuery();

  bool isValid() const;
  bool isActive() const;
  bool isNull(int field) const;
  bool isNull(const QString& name) const;
  int at() const;
  QString lastQuery() const;
  int numRowsAffected() const;
  QSqlError lastError() const;
  bool isSelect() const;
  int size() const;
  const QSqlDriver *driver() const;
  const QSqlResult *result() const;
  bool isForwardOnly() const;
  SqlRecord record(bool allowInvalidQuery = false) const;
  QSqlRecord sqlRecord() const;

  void setForwardOnly(bool forward);
  void exec(const QString& queryString);
  QVariant value(int i) const;
  QVariant value(const QString& name) const;
  bool hasField(const QString& name) const;

  /* Typed getters. Throw exception if value does not exist as field. */
  QString valueStr(int i) const
  {
    return value(i).toString();
  }

  QString valueStr(const QString& name) const
  {
    return value(name).toString();
  }

  int valueInt(int i) const
  {
    return value(i).toInt();
  }

  int valueInt(const QString& name) const
  {
    return value(name).toInt();
  }

  float valueFloat(int i) const
  {
    return value(i).toFloat();
  }

  float valueFloat(const QString& name) const
  {
    return value(name).toFloat();
  }

  double valueDouble(int i) const
  {
    return value(i).toDouble();
  }

  double valueDouble(const QString& name) const
  {
    return value(name).toDouble();
  }

  QDateTime valueDateTime(int i) const
  {
    return value(i).toDateTime();
  }

  QDateTime valueDateTime(const QString& name) const
  {
    return value(name).toDateTime();
  }

  int valueBool(int i) const
  {
    return value(i).toBool();
  }

  int valueBool(const QString& name) const
  {
    return value(name).toBool();
  }

  /* Getters which return a default value if field does not exit in record instead of throwing an exception. */
  QVariant value(const QString& name, const QVariant& defaulValue) const
  {
    return hasField(name) ? value(name) : defaulValue;
  }

  QString valueStr(const QString& name, const QString& defaultValue) const
  {
    return hasField(name) ? valueStr(name) : defaultValue;
  }

  int valueInt(const QString& name, int defaultValue) const
  {
    return hasField(name) ? valueInt(name) : defaultValue;
  }

  float valueFloat(const QString& name, float defaultValue) const
  {
    return hasField(name) ? valueFloat(name) : defaultValue;
  }

  double valueDouble(const QString& name, double defaultValue) const
  {
    return hasField(name) ? valueDouble(name) : defaultValue;
  }

  QDateTime valueDateTime(const QString& name, QDateTime defaultValue) const
  {
    return hasField(name) ? valueDateTime(name) : defaultValue;
  }

  int valueBool(const QString& name, bool defaultValue) const
  {
    return hasField(name) ? valueBool(name) : defaultValue;
  }

  void setNumericalPrecisionPolicy(QSql::NumericalPrecisionPolicy precisionPolicy);
  QSql::NumericalPrecisionPolicy numericalPrecisionPolicy() const;

  bool seek(int i, bool relative = false);
  bool next();
  bool previous();
  bool first();
  bool last();

  void clear();

  /* Nullifies all bound values */
  void clearBoundValues();

  void exec();

  void execBatch(QSqlQuery::BatchExecutionMode mode = QSqlQuery::ValuesAsRows);
  void prepare(const QString& queryString);
  void bindValue(const QString& placeholder, const QVariant& val, QSql::ParamType type = QSql::In);
  void bindValue(int pos, const QVariant& val, QSql::ParamType type = QSql::In);
  void addBindValue(const QVariant& val, QSql::ParamType type = QSql::In);

  void bindRecord(const atools::sql::SqlRecord& record);

  void bindAndExecRecords(const atools::sql::SqlRecordVector& records);
  void bindAndExecRecord(const SqlRecord& record);

  QVariant boundValue(const QString& placeholder, bool ignoreInvalid = false) const;
  QVariant boundValue(int pos, bool ignoreInvalid = false) const;

  QMap<QString, QVariant> boundValues() const;
  QString executedQuery() const;
  QVariant lastInsertId() const;
  void finish();
  bool nextResult();

  const QSqlQuery& getQSqlQuery() const;

  const QString& getQueryString() const
  {
    return queryString;
  }

  /* Get a full annotated query string with bound values replaced for debugging (only for named bindings) */
  QString getFullQueryString() const;

  void bindValues(const QVector<std::pair<QString, QVariant> >& bindValues);
  void bindValues(const QVector<std::pair<int, QVariant> >& bindValues);

private:
  friend class SqlDatabase;

  SqlQuery(const QSqlQuery& otherQuery, QString queryStr);

  void checkError(bool retval = true, const QString& msg = QString()) const;

  QSqlQuery query;
  QString queryString;
  SqlDatabase *db = nullptr;
  QString boundValuesAsString() const;

};

} // namespace sql
} // namespace atools

#endif // ATOOLS_SQL_SQLQUERY_H
