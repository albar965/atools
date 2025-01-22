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

#ifndef ATOOLS_SQL_SQLQUERY_H
#define ATOOLS_SQL_SQLQUERY_H

#include "atools.h"
#include "sql/sqltypes.h"

#include <QDateTime>
#include <QSet>
#include <QSqlQuery>
#include <QVariant>

class QSqlResult;

namespace atools {
namespace sql {

class SqlDatabase;
class SqlRecord;

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
  explicit SqlQuery(const QString& queryStr, const atools::sql::SqlDatabase& sqlDb);
  explicit SqlQuery(const QString& queryStr, const atools::sql::SqlDatabase *sqlDb);
  explicit SqlQuery(const atools::sql::SqlDatabase& sqlDb);
  explicit SqlQuery(const atools::sql::SqlDatabase *sqlDb);

  SqlQuery(const atools::sql::SqlQuery& other);
  SqlQuery& operator=(const atools::sql::SqlQuery& other);

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
  bool isForwardOnly() const;
  SqlRecord record(bool allowInvalidQuery = false) const;

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

  QChar valueChar(const QString& name) const
  {
    return atools::strToChar(value(name).toString());
  }

  QChar valueChar(int i) const
  {
    return atools::strToChar(value(i).toString());
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

  QChar valueChar(const QString& name, const QChar& defaultValue) const
  {
    return hasField(name) ? valueChar(name) : defaultValue;
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

  void execAndClearBounds()
  {
    exec();
    clearBoundValues();
  }

  void execBatch(QSqlQuery::BatchExecutionMode mode = QSqlQuery::ValuesAsRows);
  void prepare(const QString& queryString);

  bool hasPlaceholder(const QString& name)
  {
    return placeholderSet.contains(name);
  }

  void bindValue(const QString& placeholder, const QVariant& val, QSql::ParamType type = QSql::In);
  void bindValue(int pos, const QVariant& val, QSql::ParamType type = QSql::In);
  void addBindValue(const QVariant& val, QSql::ParamType type = QSql::In);

  void bindNullStr(const QString& placeholder);
  void bindNullStr(int pos);
  void bindNullInt(const QString& placeholder);
  void bindNullInt(int pos);
  void bindNullFloat(const QString& placeholder);
  void bindNullFloat(int pos);

  void bindRecord(const atools::sql::SqlRecord& record, const QString& bindPrefix = QString());

  void bindAndExecRecords(const atools::sql::SqlRecordList& records, const QString& bindPrefix = QString());
  void bindAndExecRecord(const SqlRecord& record, const QString& bindPrefix = QString());

  QVariant boundValue(const QString& placeholder, bool ignoreInvalid = false) const;
  QVariant boundValue(int pos, bool ignoreInvalid = false) const;

  QString executedQuery() const;
  QVariant lastInsertId() const;
  void finish();
  bool nextResult();

  const QString& getQueryString() const
  {
    return queryString;
  }

  void bindValues(const QVector<std::pair<QString, QVariant> >& bindValues);
  void bindValues(const QVector<std::pair<int, QVariant> >& bindValues);

  /* Extract named bindings prefixed with colon ":placeholder" and unnamed bindings as question mark "?".
   * Throws exception if named and unamed are mixed. */
  static QStringList extractPlaceholders(const QString& query, bool& positional);

  /* Get a map of placeholders and associated values from query */
  QMap<QString, QVariant> boundPlaceholderAndValueMap() const;

  /* List of bound values for all placeholders */
  QVariantList boundValues() const;

  /* Get an ordered list of placeholders as found in the query string. Positional bindings are stored as numbers */
  const QStringList& getPlaceholderList() const
  {
    return placeholderList;
  }

  const QSet<QString>& getPlaceholderSet() const
  {
    return placeholderSet;
  }

  atools::sql::SqlDatabase *getDatabase() const
  {
    return db;
  }

  QString boundValuesAsString() const;

private:
  friend class SqlDatabase;

  void checkError(bool retval = true, const QString& msg = QString()) const;
  void checkPlaceholder(const QString& funcInfo, const QString& placeholder) const;
  void checkPos(const QString& funcInfo, int pos) const;
  void checkValues(const QString& funcInfo, const QVariantList& values) const;

  QSqlQuery query;
  QString queryString;
  QStringList placeholderList;
  QSet<QString> placeholderSet;
  bool positionalPlaceholders = false;

  atools::sql::SqlDatabase *db = nullptr;
};

} // namespace sql
} // namespace atools

#endif // ATOOLS_SQL_SQLQUERY_H
