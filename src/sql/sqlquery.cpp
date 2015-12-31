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

#include "sql/sqlquery.h"
#include "sql/sqldatabase.h"
#include "sql/sqlexception.h"

#include <QSqlError>
#include <QSqlRecord>

namespace atools {

namespace sql {

SqlQuery::SqlQuery(QSqlResult *r)
{
  this->query = QSqlQuery(r);
  this->queryString = QString();
}

SqlQuery::SqlQuery(const QString& queryStr, const QSqlDatabase& sqlDb)
{
  this->query = QSqlQuery(queryStr, sqlDb);
  this->queryString = queryStr;

  if(!queryStr.isEmpty())
    checkError(!lastError().isValid(), "SqlQuery caused error after construction");
}

SqlQuery::SqlQuery(const SqlDatabase *db)
{
  this->query = QSqlQuery(db->getQSqlDatabase());
  this->queryString = QString();
}

SqlQuery::SqlQuery(const SqlDatabase& db)
{
  this->query = QSqlQuery(db.getQSqlDatabase());
  this->queryString = QString();
}

SqlQuery::SqlQuery(const SqlQuery& other)
{
  this->query = other.query;
  this->queryString = other.queryString;
}

SqlQuery::SqlQuery(const QSqlQuery& otherQuery, QString queryStr)
{
  this->query = otherQuery;
  this->queryString = queryStr;
}

SqlQuery& SqlQuery::operator=(const SqlQuery& other)
{
  this->query = other.query;
  this->queryString = other.queryString;
  return *this;
}

SqlQuery::~SqlQuery()
{
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

QSqlRecord SqlQuery::record() const
{
  checkError(isValid(), "SqlQuery::record() on invalid query");
  checkError(isActive(), "SqlQuery::record() on inactive query");
  return query.record();
}

void SqlQuery::setForwardOnly(bool forward)
{
  query.setForwardOnly(forward);
}

void SqlQuery::exec(const QString& queryStr)
{
  this->queryString = queryStr;
  checkError(query.exec(queryStr), "SqlQuery::exec(): Error executing query");
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

void SqlQuery::exec()
{
  checkError(query.exec(), "SqlQuery::exec(): Error executing query");
}

void SqlQuery::execBatch(QSqlQuery::BatchExecutionMode mode)
{
  checkError(query.execBatch(mode), "SqlQuery::execBatch(): Error executing query batch");
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

void SqlQuery::addBindValue(const QVariant& val, QSql::ParamType type)
{
  query.addBindValue(val, type);
}

QVariant SqlQuery::boundValue(const QString& placeholder) const
{
  QVariant v = query.boundValue(placeholder);
  if(!v.isValid())
    throw SqlException(
            "SqlQuery::boundValue(): Bind name \"" + placeholder + "\" does not exist in query \"" +
            queryString +
            "\"");
  return v;
}

QVariant SqlQuery::boundValue(int pos) const
{
  QVariant v = query.boundValue(pos);
  if(!v.isValid())
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

void SqlQuery::checkError(bool retval, const QString& msg) const
{
  if(!retval || query.lastError().isValid())
  {
    if(queryString.isEmpty())
      throw SqlException(query.lastError(), msg);
    else
      throw SqlException(query.lastError(), msg, "Query is \"" + queryString + "\"");
  }
}

const QSqlQuery& SqlQuery::getQSqlQuery() const
{
  return query;
}

} // namespace sql

} // namespace atools
