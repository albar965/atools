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

#include "sql/sqlquery.h"
#include "sql/sqlexception.h"
#include "sql/sqldatabase.h"

#include "sql/sqlrecord.h"

#include <QSqlError>

namespace atools {

namespace sql {

SqlQuery::SqlQuery(QSqlResult *r)
{
  this->query = QSqlQuery(r);
  this->queryString = QString();
  this->db = new SqlDatabase();
}

SqlQuery::SqlQuery(const QString& queryStr, const SqlDatabase& sqlDb)
{
  this->query = QSqlQuery(queryStr, sqlDb.getQSqlDatabase());
  this->queryString = queryStr;
  this->db = new SqlDatabase(sqlDb);
}

SqlQuery::SqlQuery(const QString& queryStr, const SqlDatabase *sqlDb)
{
  this->query = QSqlQuery(queryStr, sqlDb->getQSqlDatabase());
  this->queryString = queryStr;
  this->db = new SqlDatabase(*sqlDb);
}

SqlQuery::SqlQuery(const SqlDatabase *sqlDb)
{
  this->query = QSqlQuery(sqlDb->getQSqlDatabase());
  this->queryString = QString();
  this->db = new SqlDatabase(*sqlDb);
}

SqlQuery::SqlQuery(const SqlDatabase& sqlDb)
{
  this->query = QSqlQuery(sqlDb.getQSqlDatabase());
  this->queryString = QString();
  this->db = new SqlDatabase(sqlDb);
}

SqlQuery::SqlQuery(const SqlQuery& other)
{
  this->query = other.query;
  this->queryString = other.queryString;
  this->db = new SqlDatabase(*other.db);

}

SqlQuery::SqlQuery(const QSqlQuery& otherQuery, QString queryStr)
{
  this->query = otherQuery;
  this->queryString = queryStr;
  this->db = new SqlDatabase();
}

SqlQuery& SqlQuery::operator=(const SqlQuery& other)
{
  this->query = other.query;
  this->queryString = other.queryString;

  delete db;
  this->db = new SqlDatabase(*other.db);

  return *this;
}

SqlQuery::~SqlQuery()
{
  delete db;
}

bool SqlQuery::isValid() const
{
  return query.isValid();
}

bool SqlQuery::isActive() const
{
  return query.isActive();
}

bool SqlQuery::isNull(int field) const
{
  checkError(isValid(), "SqlQuery::isNull() on invalid query");
  checkError(isActive(), "SqlQuery::isNull() on inactive query");

  if(field >= query.record().count())
    throw SqlException("SqlQuery::isNull(): Value index " +
                       QString::number(field) + " does not exist in query \"" + queryString + "\"");

  return query.isNull(field);
}

bool SqlQuery::isNull(const QString& name) const
{
  checkError(isValid(), "SqlQuery::isNull() on invalid query");
  checkError(isActive(), "SqlQuery::isNull() on inactive query");

  if(!query.record().contains(name))
    throw SqlException(
            "SqlQuery::isNull(): Value name \"" + name + "\" does not exist in query \"" + queryString + "\"");

  return query.isNull(name);
}

int SqlQuery::at() const
{
  return query.at();
}

QString SqlQuery::lastQuery() const
{
  return query.lastQuery();
}

int SqlQuery::numRowsAffected() const
{
  checkError(isActive(), "SqlQuery::numRowsAffected() on inactive query");
  return query.numRowsAffected();
}

QSqlError SqlQuery::lastError() const
{
  return query.lastError();
}

bool SqlQuery::isSelect() const
{
  return query.isSelect();
}

int SqlQuery::size() const
{
  checkError(isActive(), "SqlQuery::size() on inactive query");
  return query.size();
}

const QSqlDriver *SqlQuery::driver() const
{
  return query.driver();
}

const QSqlResult *SqlQuery::result() const
{
  return query.result();
}

bool SqlQuery::isForwardOnly() const
{
  return query.isForwardOnly();
}

QSqlRecord SqlQuery::sqlRecord() const
{
  checkError(isValid(), "SqlQuery::record() on invalid query");
  checkError(isActive(), "SqlQuery::record() on inactive query");
  return query.record();
}

SqlRecord SqlQuery::record(bool allowInvalidQuery) const
{
  if(!allowInvalidQuery)
    checkError(isValid(), "SqlQuery::record() on invalid query");
  checkError(isActive(), "SqlQuery::record() on inactive query");
  return SqlRecord(query.record(), queryString);
}

void SqlQuery::setForwardOnly(bool forward)
{
  query.setForwardOnly(forward);
}

void SqlQuery::exec(const QString& queryStr)
{
  this->queryString = queryStr;
  checkError(query.exec(queryStr), "SqlQuery::exec(): Error executing query");

  if(db->isAutocommit())
    db->commit();
}

QVariant SqlQuery::value(int i) const
{
  checkError(isValid(), "SqlQuery::value() on invalid query");
  checkError(isActive(), "SqlQuery::value() on inactive query");
  QVariant retval = query.value(i);
  if(!retval.isValid())
    throw SqlException("SqlQuery::value(): Value index " + QString::number(
                         i) + " does not exist in query \"" + queryString + "\"");
  return retval;
}

QVariant SqlQuery::value(const QString& name) const
{
  checkError(isValid(), "SqlQuery::value() on invalid query");
  checkError(isActive(), "SqlQuery::value() on inactive query");
  QVariant retval = query.value(name);
  if(!retval.isValid())
    throw SqlException(
            "SqlQuery::value(): Value name \"" + name + "\" does not exist in query \"" + queryString + "\"");
  return retval;
}

bool SqlQuery::hasField(const QString& name) const
{
  checkError(isValid(), "SqlQuery::hasField() on invalid query");
  checkError(isActive(), "SqlQuery::hasField() on inactive query");
  return query.record().contains(name);
}

void SqlQuery::setNumericalPrecisionPolicy(QSql::NumericalPrecisionPolicy precisionPolicy)
{
  query.setNumericalPrecisionPolicy(precisionPolicy);
}

QSql::NumericalPrecisionPolicy SqlQuery::numericalPrecisionPolicy() const
{
  return query.numericalPrecisionPolicy();
}

bool SqlQuery::seek(int i, bool relative)
{
  checkError(isSelect(), "SqlQuery::seek() on query which is not a select");
  checkError(isActive(), "SqlQuery::seek() on inactive query");
  return query.seek(i, relative);
}

bool SqlQuery::next()
{
  checkError(isSelect(), "SqlQuery::next() on query which is not a select");
  checkError(isActive(), "SqlQuery::next() on inactive query");
  return query.next();
}

bool SqlQuery::previous()
{
  checkError(isSelect(), "SqlQuery::previous() on query which is not a select");
  checkError(isActive(), "SqlQuery::previous() on inactive query");
  return query.previous();
}

bool SqlQuery::first()
{
  checkError(isSelect(), "SqlQuery::first() on query which is not a select");
  checkError(isActive(), "SqlQuery::first() on inactive query");
  return query.first();
}

bool SqlQuery::last()
{
  checkError(isSelect(), "SqlQuery::last() on query which is not a select");
  checkError(isActive(), "SqlQuery::last() on inactive query");
  return query.last();
}

void SqlQuery::clear()
{
  query.clear();
}

void SqlQuery::clearBoundValues()
{
  QMap<QString, QVariant> values = boundValues();

  for(const QString& key : values.keys())
  {
    const QVariant& value = values.value(key);
    if(value.isValid() && !value.isNull())
      bindValue(key, QVariant(value.type()));
  }
}

void SqlQuery::exec()
{
  checkError(query.exec(), "SqlQuery::exec(): Error executing query");
  if(db->isAutocommit())
    db->commit();
}

void SqlQuery::execBatch(QSqlQuery::BatchExecutionMode mode)
{
  checkError(query.execBatch(mode), "SqlQuery::execBatch(): Error executing query batch");

  if(db->isAutocommit())
    db->commit();
}

void SqlQuery::prepare(const QString& queryStr)
{
  this->queryString = queryStr;
  checkError(query.prepare(queryStr), "SqlQuery::prepare(): Error executing prepare");
}

void SqlQuery::bindValue(const QString& placeholder, const QVariant& val, QSql::ParamType type)
{
  query.bindValue(placeholder, val, type);
  boundValue(placeholder);
}

void SqlQuery::bindValue(int pos, const QVariant& val, QSql::ParamType type)
{
  query.bindValue(pos, val, type);
  boundValue(pos);
}

void SqlQuery::bindValues(const QVector<std::pair<QString, QVariant> >& bindValues)
{
  for(const std::pair<QString, QVariant>& bind : bindValues)
    bindValue(bind.first, bind.second);
}

void SqlQuery::bindValues(const QVector<std::pair<int, QVariant> >& bindValues)
{
  for(const std::pair<int, QVariant>& bind : bindValues)
    bindValue(bind.first, bind.second);
}

void SqlQuery::addBindValue(const QVariant& val, QSql::ParamType type)
{
  query.addBindValue(val, type);
}

void SqlQuery::bindNullStr(const QString& placeholder)
{
  query.bindValue(placeholder, QVariant(QVariant::String));
  boundValue(placeholder);
}

void SqlQuery::bindNullStr(int pos)
{
  query.bindValue(pos, QVariant(QVariant::String));
  boundValue(pos);
}

void SqlQuery::bindNullInt(const QString& placeholder)
{
  query.bindValue(placeholder, QVariant(QVariant::Int));
  boundValue(placeholder);
}

void SqlQuery::bindNullInt(int pos)
{
  query.bindValue(pos, QVariant(QVariant::Int));
  boundValue(pos);
}

void SqlQuery::bindNullFloat(const QString& placeholder)
{
  query.bindValue(placeholder, QVariant(QVariant::Double));
  boundValue(placeholder);
}

void SqlQuery::bindNullFloat(int pos)
{
  query.bindValue(pos, QVariant(QVariant::Double));
  boundValue(pos);
}

void SqlQuery::bindRecord(const SqlRecord& record)
{
  for(int i = 0; i < record.count(); i++)
    bindValue(record.fieldName(i), record.value(i));
}

void SqlQuery::bindAndExecRecords(const SqlRecordVector& records)
{
  for(const SqlRecord& record:records)
  {
    bindRecord(record);
    exec();
    clearBoundValues();
  }
}

void SqlQuery::bindAndExecRecord(const SqlRecord& record)
{
  bindRecord(record);
  exec();
  clearBoundValues();
}

QVariant SqlQuery::boundValue(const QString& placeholder, bool ignoreInvalid) const
{
  QVariant v = query.boundValue(placeholder);

  if(!ignoreInvalid && !v.isValid())
    throw SqlException(
            "SqlQuery::boundValue(): Bind name \"" + placeholder + "\" does not exist in query \"" +
            queryString +
            "\"");
  return v;
}

QVariant SqlQuery::boundValue(int pos, bool ignoreInvalid) const
{
  QVariant v = query.boundValue(pos);
  if(!ignoreInvalid && !v.isValid())
    throw SqlException("SqlQuery::boundValue(): Bind index " + QString::number(
                         pos) + " does not exist in query \"" + queryString + "\"");
  return v;
}

QMap<QString, QVariant> SqlQuery::boundValues() const
{
  return query.boundValues();
}

QString SqlQuery::executedQuery() const
{
  return query.executedQuery();
}

QVariant SqlQuery::lastInsertId() const
{
  return query.lastInsertId();
}

void SqlQuery::finish()
{
  query.finish();
}

bool SqlQuery::nextResult()
{
  return query.nextResult();
}

QString SqlQuery::boundValuesAsString() const
{
  QMap<QString, QVariant> boundValues = query.boundValues();

  QStringList values;
  for(QMap<QString, QVariant>::const_iterator i = boundValues.constBegin();
      i != boundValues.constEnd(); ++i)
    values.append("\"" + i.key() + "\"=\"" + i.value().toString() + "\"");
  return values.join(",");
}

void SqlQuery::checkError(bool retval, const QString& msg) const
{

  if(!retval || query.lastError().isValid())
  {
    if(queryString.isEmpty())
      throw SqlException(query.lastError(), msg);
    else
    {
      QString msg2("Query is \"" + queryString + "\".");

      QString boundValues = boundValuesAsString();
      if(!boundValues.isEmpty())
        msg2 += " Bound values are [" + boundValues + "]";

      throw SqlException(query.lastError(), msg, msg2);
    }
  }
}

const QSqlQuery& SqlQuery::getQSqlQuery() const
{
  return query;
}

QString SqlQuery::getFullQueryString() const
{
  QString retval = getQueryString();

  for(const QString& name : boundValues().keys())
  {
    QVariant val = boundValue(name);
    if(val.type() == QVariant::String)
      retval.replace(name, "'" + val.toString() + "' /* " + name + " */");
    else
      retval.replace(name, val.toString() + " /* " + name + " */");
  }
  return retval;
}

} // namespace sql

} // namespace atools
