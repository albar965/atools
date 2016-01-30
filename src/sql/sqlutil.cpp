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

#include "sql/sqlutil.h"

#include <QString>

namespace atools {

namespace sql {

SqlUtil::SqlUtil(SqlDatabase *sqlDb)
  : db(sqlDb)
{
}

QString SqlUtil::buildInsertStatement(const QString& tablename, const QString& otherClause)
{
  // TODO use QSqlDriver::sqlStatement()
  QString columnList, valueList;

  QSqlRecord record = db->record(tablename);

  for(int i = 0; i < record.count(); ++i)
  {
    QString name = record.fieldName(i);

    if(!columnList.isEmpty())
      columnList += ", ";
    columnList += name;

    if(!valueList.isEmpty())
      valueList += ", ";
    valueList += ":" + name;
  }
  return "insert " + otherClause + " into " + tablename + " (" + columnList + ") values(" + valueList + ")";
}

QString SqlUtil::buildSelectStatement(const QString& tablename)
{
  // TODO use QSqlDriver::sqlStatement()
  QString columnList;

  QSqlRecord record = db->record(tablename);

  for(int i = 0; i < record.count(); ++i)
  {
    QString name = record.fieldName(i);

    if(!columnList.isEmpty())
      columnList += ", ";
    columnList += name;
  }
  return "select " + columnList + " from " + tablename;
}

bool SqlUtil::hasTable(const QString& tablename)
{
  QSqlRecord rec = db->record(tablename);
  return !rec.isEmpty();
}

bool SqlUtil::hasTableAndRows(const QString& tablename)
{
  QSqlRecord rec = db->record(tablename);
  if(!rec.isEmpty())
  {
    SqlQuery q(db);
    q.exec("select count(1) from " + tablename);
    if(q.next())
      return q.value(0).toInt() > 0;
  }
  return false;
}

void SqlUtil::copyRowValues(const SqlQuery& from, SqlQuery& to)
{
  QSqlRecord fromRec = from.record();

  for(int i = 0; i < fromRec.count(); ++i)
    to.bindValue(i, from.value(i));
}

int SqlUtil::copyResultValues(SqlQuery& from, SqlQuery& to, std::function<bool(SqlQuery&, SqlQuery&)> func)
{
  int copied = 0;
  while(from.next())
  {
    copyRowValues(from, to);

    if(func(from, to))
    {
      to.exec();
      if(to.numRowsAffected() != 1)
        throw SqlException("Error executing statement in Utility::copyResultValues(). "
                           "Number of inserted rows not 1. "
                           "(SQL \"" + to.lastQuery() + "\")");
      copied++;
    }
  }
  return copied;
}

} // namespace sql

} // namespace atools
