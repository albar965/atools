/*****************************************************************************
* Copyright 2015-2023 Alexander Barthel alex@littlenavmap.org
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
#include <QStringBuilder>

namespace atools {
namespace sql {

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
using Qt::endl;
#endif

SqlUtil::SqlUtil(const SqlDatabase *sqlDb)
  : db(sqlDb)
{
}

SqlUtil::SqlUtil(const SqlDatabase& sqlDb)
  : db(&sqlDb)
{

}

QString SqlUtil::buildInsertStatement(const QString& tablename, const QString& otherClause,
                                      const QStringList& excludeColumns, bool namedBindings) const
{
  QStringList columnList, valueList;

  SqlRecord record = db->record(tablename);

  for(int i = 0; i < record.count(); i++)
  {
    QString name = record.fieldName(i);

    if(excludeColumns.contains(name))
      continue;

    columnList.append(name);
    valueList.append(namedBindings ? QString(":" % name) : "?");
  }
  return "insert " % otherClause % " into " % tablename % " (" % columnList.join(", ") % ") values(" % valueList.join(", ") % ")";
}

QString SqlUtil::buildUpdateStatement(const QString& tablename, const QString& whereClause,
                                      const QStringList& excludeColumns, bool namedBindings) const
{
  QStringList columnList;

  SqlRecord record = db->record(tablename);

  for(int i = 0; i < record.count(); i++)
  {
    QString name = record.fieldName(i);

    if(excludeColumns.contains(name))
      continue;

    columnList.append(name % " = " % (namedBindings ? QString(":" % name) : "?"));
  }
  return "update " % tablename % " set " % columnList.join(", ") % (whereClause.isEmpty() ? QString() : " where " % whereClause);
}

QString SqlUtil::buildSelectStatement(const QString& tablename) const
{
  return "select " % buildColumnList(tablename).join(", ") % " from " % tablename;
}

QStringList SqlUtil::buildColumnList(const QString& tablename, const QStringList& excludeColumns) const
{
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

QStringList SqlUtil::buildColumnListIf(const QString& tablename, const QStringList& columns) const
{
  QStringList columList;
  SqlRecord record = db->record(tablename);

  for(const QString& column : columns)
  {
    if(record.contains(column))
      columList.append(column);
  }
  return columList;
}

QString SqlUtil::buildSelectStatement(const QString& tablename, const QStringList& columns) const
{
  return "select " % columns.join(", ") % " from " % tablename;
}

bool SqlUtil::hasTable(const QString& tablename) const
{
  return !db->record(tablename).isEmpty();
}

bool SqlUtil::hasTableAndColumn(const QString& tablename, const QString& columnname) const
{
  return db->record(tablename).contains(columnname);
}

bool SqlUtil::hasTableAndRows(const QString& tablename) const
{
  if(!db->record(tablename).isEmpty())
    return hasRows(tablename);

  return false;
}

SqlDatabase *SqlUtil::getDbWithTableAndRows(const QString& tablename, QVector<SqlDatabase *> databases)
{
  for(SqlDatabase *database : databases)
  {
    if(database != nullptr && SqlUtil(database).hasTableAndRows(tablename))
      return database;
  }
  return nullptr;
}

SqlDatabase *SqlUtil::getDbWithTableAndRows(const QString& tablename, SqlDatabase *db1, SqlDatabase *db2)
{
  return getDbWithTableAndRows(tablename, {db1, db2});
}

int SqlUtil::getTableColumnAndDistinctRows(const QString& tablename, const QString& columnname) const
{
  if(hasTableAndColumn(tablename, columnname))
  {
    SqlQuery q(db);
    q.exec("select count(distinct " % columnname % ") from " % tablename);
    if(q.next())
      return q.value(0).toInt();
  }
  return -1;
}

int SqlUtil::rowCount(const QString& tablename, const QString& criteria) const
{
  SqlQuery q(db);
  q.exec("select count(1) from " % tablename % (criteria.isEmpty() ? QString() : " where " % criteria));
  if(q.next())
    return q.value(0).toInt();

  return 0;
}

bool SqlUtil::hasRows(const QString& tablename, const QString& criteria) const
{
  SqlQuery q(db);
  q.exec("select 1 from " % tablename % (criteria.isEmpty() ? QString() : " where " % criteria) % " limit 1");
  return q.next();
}

void SqlUtil::copyRowValues(const SqlQuery& from, SqlQuery& to)
{
  copyRowValuesInternal(from, to, from.record(), to.boundPlaceholderAndValueMap());
}

void SqlUtil::copyRowValuesInternal(const SqlQuery& from, SqlQuery& to, const SqlRecord& fromRec, const QMap<QString, QVariant>& bound)
{
  for(int i = 0; i < fromRec.count(); i++)
  {
    QString bind = ":" % fromRec.fieldName(i);
    if(bound.contains(bind))
      to.bindValue(bind, from.value(i));
  }
}

int SqlUtil::copyResultValues(SqlQuery& from, SqlQuery& to, std::function<bool(SqlQuery&, SqlQuery&)> func)
{
  int copied = 0;
  SqlRecord fromRec;
  QMap<QString, QVariant> bound = to.boundPlaceholderAndValueMap();

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
                           "(SQL \"" % to.lastQuery() % "\")");
      copied++;
    }
  }
  return copied;
}

int SqlUtil::copyResultValues(SqlQuery& from, SqlQuery& to)
{
  int copied = 0;
  SqlRecord fromRec;
  QMap<QString, QVariant> bound = to.boundPlaceholderAndValueMap();
  while(from.next())
  {
    if(fromRec.isEmpty())
      fromRec = from.record();

    copyRowValuesInternal(from, to, fromRec, bound);
    to.exec();
    if(to.numRowsAffected() != 1)
      throw SqlException("Error executing statement in Utility::copyResultValues(). "
                         "Number of inserted rows not 1. "
                         "(SQL \"" % to.lastQuery() % "\")");
    copied++;
  }
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
    insertSet.append(ic % " = :" % ic);

  SqlQuery insert(db);
  QString queryStr = "update " % table % " set " % insertSet.join(", ") % " where " % idColum % " = :" % idColum;

  if(!whereClause.isEmpty())
    queryStr += " and (" % whereClause % ")";

  insert.prepare(queryStr);

  select.exec();

  while(select.next())
  {
    if(func(select, insert))
    {
      insert.bindValue(":" % idColum, select.value(idColum));
      insert.exec();
    }
  }
}

void SqlUtil::printTableStats(QDebug& out, const QStringList& tables, bool brief) const
{
  QDebugStateSaver saver(out);
  out.noquote().nospace();

  if(!brief)
    out << "Statistics for database (tables / rows):" << endl;

  const QStringList tableList = buildTableList(tables);

  SqlQuery query(db);

  int totalCount = 0;

  for(const QString& name : tableList)
  {
    if(hasTable(name))
    {
      if(hasTableAndRows(name))
      {
        query.exec("select count(1) as cnt from " % name);
        if(query.next())
        {
          int cnt = query.value("cnt").toInt();
          totalCount += cnt;

          if(brief)
            out << name << " " << cnt << " ";
          else
            out << name << ": " << cnt << " rows" << endl;
        }
      }
      else
      {
        out << name << " is empty";
        if(!brief)
          out << endl;
      }
    }
    else
    {
      out << name << " does not exist";
      if(!brief)
        out << endl;
    }
  }

  if(!brief)
    out << "Total" << ": " << totalCount << " rows" << endl;
}

void SqlUtil::createColumnReport(QDebug& out, const QStringList& tables) const
{
  QDebugStateSaver saver(out);
  out.noquote().nospace();

  out << "Column value report for database:" << endl;

  const QStringList tableList = buildTableList(tables);

  SqlQuery querySelCount(db);
  SqlQuery queryGroup(db);

  for(const QString& name : tableList)
  {
    if(hasTable(name))
    {
      if(hasTableAndRows(name))
      {
        SqlRecord record = db->record(name);

        for(int i = 0; i < record.count(); i++)
        {
          QString col = record.fieldName(i);
          querySelCount.exec("select count(distinct " % col % ") as cnt from " % name);
          if(querySelCount.next())
          {
            int cnt = querySelCount.value("cnt").toInt();
            if(cnt < 2)
            {
              out << name << "." << col;
              if(cnt == 0)
                out << " has no distinct values" << endl;
              else if(cnt == 1)
              {
                out << " has only 1 distinct value: ";
                queryGroup.exec("select " % col % " from " % name % " group by " % col);
                while(queryGroup.next())
                {
                  QVariant val = queryGroup.value(0);
                  if(val.type() != QVariant::ByteArray && val.canConvert(QVariant::String))
                    out << val.toString();
                  else
                    out << "[" << val.typeName() << "]";
                }
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
                                    const QVariant& maxValue) const
{
  QDebugStateSaver saver(out);
  out.noquote().nospace();

  if(hasTable(table))
  {
    if(hasTableAndRows(table))
    {
      SqlQuery q(db);
      q.prepare("select " % reportCols.join(", ") % ", " % column % " from " % table %
                " where " % column % " not between :min and :max");

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

void SqlUtil::reportDuplicates(QDebug& out, const QString& table, const QString& idColumn, const QStringList& identityColumns) const
{
  QDebugStateSaver saver(out);
  out.noquote().nospace();

  QStringList where;
  QStringList colList;
  for(const QString& icol : identityColumns)
  {
    where.append("t1." % icol % " = t2." % icol);
    colList.append("t1." % icol);
  }

  SqlQuery q(db);
  q.exec(
    "select distinct t1." % idColumn % ", " % colList.join(", ") %
    " from " % table % " t1 " %
    "join " % table % " t2 on " % where.join(" and ") %
    " where t1." % idColumn % " <> t2." % idColumn %
    " order by " % colList.join(", "));

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

const QStringList SqlUtil::buildTableList(const QStringList& tables) const
{
  QStringList tableList;
  if(tables.isEmpty())
    tableList = db->tables();
  else
    tableList = tables;

  tableList.sort();
  return tableList;
}

const QStringList SqlUtil::buildResultList(SqlQuery& query) const
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
    db->exec("alter table " % table % " add column " % column % " " % type % " " % suffix);
    return true;
  }
  return false;
}

int SqlUtil::getMaxId(const QString& table, const QString& idColumn)
{
  int id = 0;
  SqlQuery query("select max(" % (idColumn.isEmpty() ? "rowid" : idColumn) % ") from " % table, db);
  query.exec();
  if(query.next())
    id = query.valueInt(0);
  else
    qWarning() << Q_FUNC_INFO << "Nothing found in" << table << idColumn;
  query.finish();
  return id;
}

void SqlUtil::getIds(QSet<int>& ids, const QString& table, const QString& idColumn, const QString& where)
{
  SqlQuery query("select " % idColumn % " from " % table % " " % (where.isEmpty() ? QString() : " where " % where), db);
  query.exec();
  while(query.next())
    ids.insert(query.valueInt(0));
}

void SqlUtil::getIds(QSet<int>& ids, const QString& queryString)
{
  SqlQuery query(queryString, db);
  query.exec();
  while(query.next())
    ids.insert(query.valueInt(0));
}

int SqlUtil::getValueInt(const QString& queryStr, int defaultValue)
{
  return getValueVar(queryStr, defaultValue).toInt();
}

float SqlUtil::getValueFloat(const QString& queryStr, float defaultValue)
{
  return getValueVar(queryStr, defaultValue).toFloat();
}

QString SqlUtil::getValueStr(const QString& queryStr, const QString& defaultValue)
{
  return getValueVar(queryStr, defaultValue).toString();
}

QVariant SqlUtil::getValueVar(const QString& queryStr, const QVariant& defaultValue)
{
  QVariant var;
  SqlQuery query(queryStr, db);
  query.exec(queryStr);
  if(query.next())
    var = query.value(0);
  else
    var = defaultValue;
  query.finish();
  return var;
}

} // namespace sql

} // namespace atools
