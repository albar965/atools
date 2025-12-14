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

#include "sql/sqlexception.h"

#include "sql/sqldatabase.h"
#include "sql/sqlquery.h"
#include "sql/sqlrecord.h"

#include <QSqlError>
#include <QStringBuilder>
#include <QDebug>

namespace atools {

namespace sql {

SqlException::SqlException(const QString& message)
{
  if(!message.isEmpty())
    setMessage(message);
}

SqlException::SqlException(const SqlDatabase *db, const QString& message)
{
  QStringList msgList;
  messageDb(msgList, db);
  msgList.append(message);
  msgList.removeAll(QString());
  setMessage(msgList.join('\n'));
}

SqlException::SqlException(const SqlQuery *query, const QString& message)
{
  QStringList msgList;
  messageDb(msgList, query != nullptr ? query->getDatabase() : nullptr);
  messageQuery(msgList, query);
  msgList.append(message);
  msgList.removeAll(QString());
  setMessage(msgList.join('\n'));
}

SqlException::SqlException(const SqlRecord *record, const QString& message)
{
  QStringList msgList;
  messageRecord(msgList, record);
  msgList.append(message);
  msgList.removeAll(QString());
  setMessage(msgList.join('\n'));
}

void SqlException::messageDb(QStringList& msgList, const SqlDatabase *db)
{
  if(db != nullptr)
  {
    msgList.append(tr("Database name \"%1\", \"%2\"").arg(db->databaseName()).arg(db->getName()));
    messageErr(msgList, db->lastError());
  }
  else
    msgList.append(tr("Database is null"));
}

void SqlException::messageErr(QStringList& msgList, const QSqlError& err)
{
  if(!err.databaseText().isEmpty())
    msgList.append(tr("Database message \"%1\"").arg(err.databaseText()));

  if(!err.driverText().isEmpty())
    msgList.append(tr("Driver message \"%1\"").arg(err.driverText()));
}

void SqlException::messageQuery(QStringList& msgList, const SqlQuery *query)
{
  if(query != nullptr)
  {
    messageErr(msgList, query->lastError());

    if(!query->getQueryString().isEmpty())
      msgList.append(tr("Query \"%1\"").arg(query->getQueryString()));
    else
      msgList.append(tr("Query text is null"));

    qWarning() << Q_FUNC_INFO << ("Bound values [" % query->boundValuesAsString() % "]");
  }
  else
    msgList.append(tr("Query is null"));
}

void SqlException::messageRecord(QStringList& msgList, const SqlRecord *record)
{
  if(record != nullptr)
  {
    if(!record->getQueryString().isEmpty())
      msgList.append(record->getQueryString());
    else
      msgList.append(tr("Record query text is null"));

    qWarning() << Q_FUNC_INFO << *record;
  }
  else
    msgList.append(tr("Record is null"));
}

void SqlException::raise() const
{
  throw *this;
}

SqlException *SqlException::clone() const
{
  return new SqlException(*this);
}

} // namespace sql
} // namespace atools
