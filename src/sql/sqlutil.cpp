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
#include "logging/loggingdefs.h"

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

  // QSqlDriver::WhereStatement	0	An SQL WHERE statement (e.g., WHERE f = 5).
  // QSqlDriver::SelectStatement	1	An SQL SELECT statement (e.g., SELECT f FROM t).
  // QSqlDriver::UpdateStatement	2	An SQL UPDATE statement (e.g., UPDATE TABLE t set f = 1).
  // QSqlDriver::InsertStatement	3	An SQL INSERT statement (e.g., INSERT INTO t (f) values (1)).
  // QSqlDriver::DeleteStatement	4	An SQL DELETE statement (e.g., DELETE FROM t).
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

void SqlUtil::printTableStats(QDebug& out)
{
  QDebugStateSaver saver(out);
  out.noquote().nospace();

  out << "Statistics for database (tables / rows):" << endl;

  SqlQuery query(db);

  int index = 1, totalCount = 0;

  for(QString name : db->tables())
  {
    query.exec("select count(1) as cnt from " + name);
    if(query.next())
    {
      int cnt = query.value("cnt").toInt();
      totalCount += cnt;
      out << "#" << (index++) << " " << name << ": " << cnt << " rows" << endl;
    }
  }
  out << "Total" << ": " << totalCount << " rows" << endl;
}

void SqlUtil::createColumnReport(QDebug& out, const QStringList& tables)
{
  QDebugStateSaver saver(out);
  out.noquote().nospace();

  out << "Column value report for database:" << endl;

  QStringList tableList;
  if(tables.isEmpty())
    tableList = db->tables();
  else
    tableList = tables;
  SqlQuery q(db);
  SqlQuery q2(db);

  for(QString name : db->tables())
  {
    QSqlRecord record = db->record(name);

    for(int i = 0; i < record.count(); ++i)
    {
      QString col = record.fieldName(i);
      q.exec("select count(distinct " + col + ") as cnt from " + name);
      if(q.next())
      {
        int cnt = q.value("cnt").toInt();
        if(cnt < 2)
        {
          out << name << "." << col;
          if(cnt == 0)
            out << " has no distinct values" << endl;
          else if(cnt == 1)
          {
            out << " has only 1 distinct value: ";
            q2.exec("select " + col + " from " + name + " group by " + col);
            while(q2.next())
              out << q2.value(0).toString();
            out << endl;
          }
        }
      }
    }
  }
}

void SqlUtil::reportDuplicates(QDebug& out,
                               const QString& table,
                               const QString& idColumn,
                               const QStringList& identityColumns)
{
  QDebugStateSaver saver(out);
  out.noquote().nospace();

  QStringList where;
  QStringList colList;
  for(QString ic : identityColumns)
  {
    where.push_back("t1." + ic + " = t2." + ic);
    colList.push_back("t1." + ic);
  }

  SqlQuery q(db);
  q.exec(
    "select distinct t1." + idColumn + ", " + colList.join(", ") +
    " from " + table + " t1 " +
    "join " + table + " t2 on " + where.join(" and ") +
    " where t1." + idColumn + " <> t2." + idColumn +
    " order by " + colList.join(", "));

  bool header = false;
  while(q.next())
  {
    if(!header)
    {
      out << "Table duplicates for " << table <<
      "(" << idColumn << "/" << identityColumns.join(",") << "):" << endl;
      header = true;
    }

    for(int i = 0; i < identityColumns.size() + 1; i++)
      out << q.value(i).toString() << ",";
    out << endl;
  }
  if(!header)
    out << "Table duplicates for " << table <<
    "(" << idColumn << "/" << identityColumns.join(",") << "): none found." << endl;
}

} // namespace sql

} // namespace atools
