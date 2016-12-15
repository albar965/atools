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

#ifndef ATOOLS_SQL_SQLQUERY_H
#define ATOOLS_SQL_SQLQUERY_H

#include "sql/sqlrecord.h"

#include <QSqlQuery>
#include <QSqlResult>
#include <QString>

class QSqlResult;

namespace atools {
namespace sql {

class SqlDatabase;

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
  SqlRecord record() const;
  QSqlRecord sqlRecord() const;

  void setForwardOnly(bool forward);
  void exec(const QString& queryString);
  QVariant value(int i) const;
  QVariant value(const QString& name) const;

  void setNumericalPrecisionPolicy(QSql::NumericalPrecisionPolicy precisionPolicy);
  QSql::NumericalPrecisionPolicy numericalPrecisionPolicy() const;

  bool seek(int i, bool relative = false);
  bool next();
  bool previous();
  bool first();
  bool last();

  void clear();

  void exec();

  void execBatch(QSqlQuery::BatchExecutionMode mode = QSqlQuery::ValuesAsRows);
  void prepare(const QString& queryString);
  void bindValue(const QString& placeholder, const QVariant& val, QSql::ParamType type = QSql::In);
  void bindValue(int pos, const QVariant& val, QSql::ParamType type = QSql::In);
  void addBindValue(const QVariant& val, QSql::ParamType type = QSql::In);
  QVariant boundValue(const QString& placeholder) const;
  QVariant boundValue(int pos) const;

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
