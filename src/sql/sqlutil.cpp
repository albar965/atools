/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

#include <QDebug>
#include <QString>

namespace atools {

namespace sql {

SqlUtil::SqlUtil(SqlDatabase *sqlDb)
  : db(sqlDb)
{
}

SqlUtil::SqlUtil(SqlDatabase& sqlDb)
  : db(&sqlDb)
{

}

QString SqlUtil::buildInsertStatement(const QString& tablename, const QString& otherClause,
                                      const QStringList& excludeColumns, bool namedBindings)
{
  // TODO use QSqlDriver::sqlStatement()

  // QSqlDriver::WhereStatement	0	An SQL WHERE statement (e.g., WHERE f = 5).
  // QSqlDriver::SelectStatement	1	An SQL SELECT statement (e.g., SELECT f FROM t).
  // QSqlDriver::UpdateStatement	2	An SQL UPDATE statement (e.g., UPDATE TABLE t set f = 1).
  // QSqlDriver::InsertStatement	3	An SQL INSERT statement (e.g., INSERT INTO t (f) values (1)).
  // QSqlDriver::DeleteStatement	4	An SQL DELETE statement (e.g., DELETE FROM t).
  QString columnList, valueList;

  SqlRecord record = db->record(tablename);

  for(int i = 0; i < record.count(); i++)
  {
    QString name = record.fieldName(i);

    if(excludeColumns.contains(name))
      continue;

    if(!columnList.isEmpty())
      columnList += ", ";
    columnList += name;

    if(!valueList.isEmpty())
      valueList += ", ";

    if(namedBindings)
      valueList += ":" + name;
    else
      valueList += "?";
  }
  return "insert " + otherClause + " into " + tablename + " (" + columnList + ") values(" + valueList + ")";
}

QString SqlUtil::buildSelectStatement(const QString& tablename)
{
  // TODO use QSqlDriver::sqlStatement()
  QString columnList;

  SqlRecord record = db->record(tablename);

  for(int i = 0; i < record.count(); i++)
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
  return !db->record(tablename).isEmpty();
}

bool SqlUtil::hasTableAndRows(const QString& tablename)
{
  if(!db->record(tablename).isEmpty())
    return hasRows(tablename);

  return false;
}

int SqlUtil::rowCount(const QString& tablename, const QString& criteria)
{
  SqlQuery q(db);
  q.exec("select count(1) from " + tablename + (criteria.isEmpty() ? QString() : " where " + criteria));
  if(q.next())
    return q.value(0).toInt();

  return 0;
}

bool SqlUtil::hasRows(const QString& tablename, const QString& criteria)
{
  SqlQuery q(db);
  q.exec("select 1 from " + tablename + (criteria.isEmpty() ? QString() : " where " + criteria) + " limit 1");
  return q.next();
}

void SqlUtil::copyRowValues(const SqlQuery& from, SqlQuery& to)
{
  copyRowValuesInternal(from, to, from.record(), to.boundValues());
}

void SqlUtil::copyRowValuesInternal(const SqlQuery& from, SqlQuery& to,
                                    const SqlRecord& fromRec, const QMap<QString, QVariant>& bound)
{
  for(int i = 0; i < fromRec.count(); i++)
  {
    QString bind = ":" + fromRec.fieldName(i);
    if(bound.contains(bind))
      to.bindValue(bind, from.value(i));
  }
}

int SqlUtil::copyResultValues(SqlQuery& from, SqlQuery& to, std::function<bool(SqlQuery&, SqlQuery&)> func)
{
  int copied = 0;
  SqlRecord fromRec;
  QMap<QString, QVariant> bound = to.boundValues();

  while(from.next())
  {
    if(fromRec.isEmpty())
      fromRec = from.record();

    copyRowValuesInternal(from, to, fromRec, bound);

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

int SqlUtil::copyResultValues(SqlQuery& from, SqlQuery& to)
{
  int copied = 0;
  SqlRecord fromRec;
  QMap<QString, QVariant> bound = to.boundValues();
  while(from.next())
  {
    if(fromRec.isEmpty())
      fromRec = from.record();

    copyRowValuesInternal(from, to, fromRec, bound);
    to.exec();
    if(to.numRowsAffected() != 1)
      throw SqlException("Error executing statement in Utility::copyResultValues(). "
                         "Number of inserted rows not 1. "
                         "(SQL \"" + to.lastQuery() + "\")");
    copied++;
  }
  return copied;
}

void SqlUtil::printTableStats(QDebug& out, const QStringList& tables)
{
  QDebugStateSaver saver(out);
  out.noquote().nospace();

  out << "Statistics for database (tables / rows):" << endl;

  QStringList tableList = buildTableList(tables);

  SqlQuery query(db);

  int totalCount = 0;

  for(QString name : tableList)
  {
    if(hasTable(name))
    {
      if(hasTableAndRows(name))
      {
        query.exec("select count(1) as cnt from " + name);
        if(query.next())
        {
          int cnt = query.value("cnt").toInt();
          totalCount += cnt;
          out << name << ": " << cnt << " rows" << endl;
        }
      }
      else
        out << name << " is empty" << endl;
    }
    else
      out << name << " does not exist" << endl;
  }
  out << "Total" << ": " << totalCount << " rows" << endl;
}

void SqlUtil::createColumnReport(QDebug& out, const QStringList& tables)
{
  QDebugStateSaver saver(out);
  out.noquote().nospace();

  out << "Column value report for database:" << endl;

  QStringList tableList = buildTableList(tables);

  SqlQuery q(db);
  SqlQuery q2(db);

  for(QString name : tableList)
  {
    if(hasTable(name))
    {
      if(hasTableAndRows(name))
      {
        SqlRecord record = db->record(name);

        for(int i = 0; i < record.count(); i++)
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
      else
        out << name << " is empty" << endl;
    }
    else
      out << name << " does not exist" << endl;
  }
}

void SqlUtil::reportRangeViolations(QDebug& out,
                                    const QString& table,
                                    const QStringList& reportCols,
                                    const QString& column,
                                    const QVariant& minValue,
                                    const QVariant& maxValue)
{
  QDebugStateSaver saver(out);
  out.noquote().nospace();

  if(hasTable(table))
  {
    if(hasTableAndRows(table))
    {
      SqlQuery q(db);
      q.prepare("select " + reportCols.join(", ") + ", " + column + " from " + table +
                " where " + column + " not between :min and :max");

      q.bindValue(":min", minValue);
      q.bindValue(":max", maxValue);
      q.exec();

      bool header = false;
      while(q.next())
      {
        if(!header)
        {
          out << "Table range violations for " << table << " (" << column
              << " not between " << minValue.toString() << " and " << maxValue.toString() << "):" << endl;
          out << reportCols.join(", ") << endl;
          header = true;
        }
        out << buildResultList(q).join(", ") << endl;
      }
      if(!header)
        out << "Table range violations for " << table << " (" << column
            << " not between " << minValue.toString() << " and " << maxValue.toString()
            << "): none found" << endl;

    }
    else
      out << table << " is empty" << endl;
  }
  else
    out << table << " does not exist" << endl;
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
    where.append("t1." + ic + " = t2." + ic);
    colList.append("t1." + ic);
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
        " (" << idColumn << "/" << identityColumns.join(",") << "):" << endl;
      header = true;
    }
    out << buildResultList(q).join(", ") << endl;
  }
  if(!header)
    out << "Table duplicates for " << table <<
      " (" << idColumn << "/" << identityColumns.join(",") << "): none found." << endl;
}

int SqlUtil::bindAndExec(const QString& sql, const QString& bind, const QVariant& value)
{
  return bindAndExec(sql, {std::make_pair(bind, value)});
}

int SqlUtil::bindAndExec(const QString& sql, QVector<std::pair<QString, QVariant> > params)
{
  SqlQuery query(db);
  query.prepare(sql);

  for(const std::pair<QString, QVariant>& bind : params)
    query.bindValue(bind.first, bind.second);
  query.exec();
  return query.numRowsAffected();
}

QStringList SqlUtil::buildTableList(const QStringList& tables)
{
  QStringList tableList;
  if(tables.isEmpty())
    tableList = db->tables();
  else
    tableList = tables;

  qSort(tableList);
  return tableList;
}

QStringList SqlUtil::buildResultList(SqlQuery& query)
{
  QStringList retval;

  SqlRecord r = query.record();
  for(int i = 0; i < r.count(); i++)
    retval.append(r.valueStr(i));
  return retval;
}

} // namespace sql

} // namespace atools
