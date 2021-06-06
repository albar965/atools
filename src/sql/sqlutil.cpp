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

#include "sql/sqlutil.h"
#include "sql/sqldatabase.h"
#include "sql/sqlrecord.h"
#include "sql/sqlquery.h"
#include "sql/sqlexception.h"

#include <QDebug>
#include <QString>
#include <QSqlDatabase>

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
  return "select " + buildColumnList(tablename).join(", ") + " from " + tablename;
}

QStringList SqlUtil::buildColumnList(const QString& tablename, const QStringList& excludeColumns)
{
  // TODO use QSqlDriver::sqlStatement()
  QStringList columnList;

  SqlRecord record = db->record(tablename);

  for(int i = 0; i < record.count(); i++)
  {
    QString name = record.fieldName(i);

    if(excludeColumns.contains(name))
      continue;

    columnList.append(name);
  }
  return columnList;
}

QString SqlUtil::buildSelectStatement(const QString& tablename, const QStringList& columns)
{
  // TODO use QSqlDriver::sqlStatement()
  return "select " + columns.join(",") + " from " + tablename;
}

bool SqlUtil::hasTable(const QString& tablename)
{
  return !db->record(tablename).isEmpty();
}

bool SqlUtil::hasTableAndColumn(const QString& tablename, const QString& columnname)
{
  return db->record(tablename).contains(columnname);
}

bool SqlUtil::hasTableAndRows(const QString& tablename)
{
  if(!db->record(tablename).isEmpty())
    return hasRows(tablename);

  return false;
}

int SqlUtil::getTableColumnAndDistinctRows(const QString& tablename, const QString& columnname)
{
  if(hasTableAndColumn(tablename, columnname))
  {
    SqlQuery q(db);
    q.exec("select count(distinct " + columnname + ") from " + tablename);
    if(q.next())
      return q.value(0).toInt();
  }
  return -1;
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
  prepareRowValuesInternal(from, to);
}

void SqlUtil::prepareRowValuesInternal(const SqlQuery& from, SqlQuery& to)
{
  SqlRecord fromRecord = from.record();
  SqlRecord toRecord = to.record();

  int fromCount = fromRecord.count();
  int toCount = toRecord.count();

  if(fromCount < toCount)                               // only as much as is in the smaller record can be copied
  {
    for(int i = 0; i < fromCount; i++)
    {
      QString bind = fromRecord.fieldName(i);
      if(toRecord.contains(bind))                       // NOTE: this only works if the placeholder == ":" + fieldname which "SQL" in general does not require afaik!
        to.bindValue(":" + bind, from.value(i));        // note the difference to below, Qt doc states this is faster
    }
  }
  else
  {
    for(int i = 0; i < toCount; i++)
    {
      QString bind = toRecord.fieldName(i);
      if(fromRecord.contains(bind))                     // NOTE: this only works if the placeholder == ":" + fieldname which "SQL" in general does not require afaik!
        to.bindValue(":" + bind, from.value(bind));
    }
  }
}

bool SqlUtil::returnTrue(SqlQuery& from, SqlQuery& to) {
  (void)from;   // circumvent compiler warning
  (void)to;     // circumvent compiler warning
  return true;
}

int SqlUtil::copyResultValues(SqlQuery& from, SqlQuery& to, std::function<bool(SqlQuery&, SqlQuery&)> func = SqlUtil::returnTrue)
{
  int copied = 0;

  if(!from.first())
    return copied;        // 0 !

  from.setForwardOnly(true);

  do
  {
    prepareRowValuesInternal(from, to);

    if(func(from, to))    // this can be optimised by explicitly duplicating this method without default parameter and omitting this call
    {
      to.exec();
      if(to.numRowsAffected() != 1)
        throw SqlException("Error executing statement in Utility::copyResultValues(). "
                           "Number of inserted rows not 1. "
                           "(SQL \"" + to.lastQuery() + "\")");
      copied++;
    }
  }
  while(from.next());

  return copied;
}

void SqlUtil::updateColumnInTable(const QString& table, const QString& idColum, const QStringList& queryColumns,
                                  const QStringList& insertcolumns, UpdateColFuncType func)
{
  updateColumnInTable(table, idColum, queryColumns, insertcolumns, QString(), func);
}

void SqlUtil::updateColumnInTable(const QString& table, const QString& idColum, const QStringList& queryColumns,
                                  const QStringList& insertcolumns, const QString& whereClause, UpdateColFuncType func)
{
  SqlUtil util(db);

  QStringList queryCols(queryColumns);
  queryCols.append(idColum);
  SqlQuery select(util.buildSelectStatement(table, queryCols), db);

  QStringList insertSet;
  for(const QString& ic : insertcolumns)
    insertSet.append(ic + " = :" + ic);

  SqlQuery insert(db);
  QString queryStr = "update " + table + " set " + insertSet.join(", ") + " where " + idColum + " = :" + idColum;

  if(!whereClause.isEmpty())
    queryStr += " and (" + whereClause + ")";

  insert.prepare(queryStr);

  select.exec();

  while(select.next())
  {
    if(func(select, insert))
    {
      insert.bindValue(":" + idColum, select.value(idColum));
      insert.exec();
    }
  }

}

void SqlUtil::printTableStats(QDebug& out, const QStringList& tables)
{
  QDebugStateSaver saver(out);
  out.noquote().nospace();

  out << "Statistics for database (tables / rows):" << Qt::endl;

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
          out << name << ": " << cnt << " rows" << Qt::endl;
        }
      }
      else
        out << name << " is empty" << Qt::endl;
    }
    else
      out << name << " does not exist" << Qt::endl;
  }
  out << "Total" << ": " << totalCount << " rows" << Qt::endl;
}

void SqlUtil::createColumnReport(QDebug& out, const QStringList& tables)
{
  QDebugStateSaver saver(out);
  out.noquote().nospace();

  out << "Column value report for database:" << Qt::endl;

  QStringList tableList = buildTableList(tables);

  SqlQuery querySelCount(db);
  SqlQuery queryGroup(db);

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
          querySelCount.exec("select count(distinct " + col + ") as cnt from " + name);
          if(querySelCount.next())
          {
            int cnt = querySelCount.value("cnt").toInt();
            if(cnt < 2)
            {
              out << name << "." << col;
              if(cnt == 0)
                out << " has no distinct values" << Qt::endl;
              else if(cnt == 1)
              {
                out << " has only 1 distinct value: ";
                queryGroup.exec("select " + col + " from " + name + " group by " + col);
                while(queryGroup.next())
                {
                  QVariant val = queryGroup.value(0);
                  if(val.metaType().id() != QMetaType::QByteArray && QMetaType::canConvert(val.metaType(), QMetaType(QMetaType::QString)))
                    out << val.toString();
                  else
                    out << "[" << val.typeName() << "]";
                }
                out << Qt::endl;
              }
            }
          }
        }
      }
      else
        out << name << " is empty" << Qt::endl;
    }
    else
      out << name << " does not exist" << Qt::endl;
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
              << " not between " << minValue.toString() << " and " << maxValue.toString() << "):" << Qt::endl;
          out << reportCols.join(", ") << Qt::endl;
          header = true;
        }
        out << buildResultList(q).join(", ") << Qt::endl;
      }
      if(!header)
        out << "Table range violations for " << table << " (" << column
            << " not between " << minValue.toString() << " and " << maxValue.toString()
            << "): none found" << Qt::endl;

    }
    else
      out << table << " is empty" << Qt::endl;
  }
  else
    out << table << " does not exist" << Qt::endl;
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
        " (" << idColumn << "/" << identityColumns.join(",") << "):" << Qt::endl;
      header = true;
    }
    out << buildResultList(q).join(", ") << Qt::endl;
  }
  if(!header)
    out << "Table duplicates for " << table <<
      " (" << idColumn << "/" << identityColumns.join(",") << "): none found." << Qt::endl;
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

  //qsort(tableList);         // TODO: should it be sorted?
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

bool SqlUtil::addColumnIf(const QString& table, const QString& column, const QString& type, const QString& suffix)
{
  if(!db->record(table).contains(column))
  {
    // Add missing column
    db->exec("alter table " + table + " add column " + column + " " + type + " " + suffix);
    return true;
  }
  return false;
}

} // namespace sql

} // namespace atools
