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

#include "fs/userdata/datamanagerbase.h"

#include "sql/sqlutil.h"
#include "sql/sqlscript.h"
#include "sql/sqltransaction.h"
#include "sql/sqlexport.h"
#include "geo/pos.h"
#include "settings/settings.h"
#include "io/fileroller.h"
#include "exception.h"
#include "sql/sqldatabase.h"

#include <QDir>

namespace atools {
namespace fs {
namespace userdata {

using atools::geo::Pos;
using atools::sql::SqlUtil;
using atools::sql::SqlScript;
using atools::sql::SqlQuery;
using atools::sql::SqlRecord;
using atools::sql::SqlExport;
using atools::sql::SqlTransaction;

DataManagerBase::DataManagerBase(sql::SqlDatabase *sqlDb, const QString& tableNameParam,
                                 const QString& idColumnNameParam, const QString& createSqlScript,
                                 const QString& dropSqlScript, const QString& backupFile)
  : db(sqlDb), tableName(tableNameParam), idColumnName(idColumnNameParam), createScript(createSqlScript),
  dropScript(dropSqlScript), backupFilename(backupFile)
{

}

DataManagerBase::~DataManagerBase()
{

}

bool DataManagerBase::hasSchema()
{
  return SqlUtil(db).hasTable(tableName);
}

bool DataManagerBase::hasData()
{
  return SqlUtil(db).hasTableAndRows(tableName);
}

void DataManagerBase::createSchema()
{
  qDebug() << Q_FUNC_INFO << tableName;

  SqlTransaction transaction(db);
  SqlScript script(db, true /*options->isVerbose()*/);

  script.executeScript(createScript);
  transaction.commit();
}

void DataManagerBase::updateSchema()
{
  //
}

void DataManagerBase::dropSchema()
{
  qDebug() << Q_FUNC_INFO << tableName;

  SqlTransaction transaction(db);
  SqlScript script(db, true /*options->isVerbose()*/);

  script.executeScript(dropScript);
  transaction.commit();
}

bool DataManagerBase::addColumnIf(const QString& colName, const QString& colType)
{
  qDebug() << Q_FUNC_INFO << colName << colType;
  SqlTransaction transaction(db);
  bool retval = SqlUtil(db).addColumnIf(tableName, colName, colType);
  transaction.commit();
  return retval;
}

void DataManagerBase::backup()
{
  qDebug() << Q_FUNC_INFO << tableName << backupFilename;

  if(hasData() && !backupFilename.isEmpty())
  {
    // Create a backup in the settings directory
    QString settingsPath = atools::settings::Settings::instance().getPath();
    QString filePath = settingsPath + QDir::separator() + backupFilename;

    atools::io::FileRoller roller(3);
    roller.rollFile(filePath);

    QFile file(filePath);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
      QTextStream out(&file);
      out.setCodec("UTF-8");
      SqlQuery query("select * from " + tableName, db);
      SqlExport sqlExport;
      sqlExport.printResultSet(query, out);
      file.close();
    }
    else
      throw atools::Exception(tr("Cannot open backup file %1. Reason: %2 (%3)").
                              arg(filePath).arg(file.errorString()).arg(file.error()));
  }
}

void DataManagerBase::updateCoordinates(int id, const geo::Pos& position)
{
  SqlQuery query(db);
  query.prepare("update " + tableName + " set lonx = ?, laty = ? where " + idColumnName + " = ?");
  query.bindValue(0, position.getLonX());
  query.bindValue(1, position.getLatY());
  query.bindValue(2, id);
  query.exec();
}

void DataManagerBase::updateField(const QString& column, const QVector<int>& ids, const QVariant& value)
{
  SqlQuery query(db);
  query.prepare("update " + tableName + " set " + column + " = ? where " + idColumnName + " = ?");

  for(int id : ids)
  {
    // Update field for all rows with the given id
    query.bindValue(0, value);
    query.bindValue(1, id);
    query.exec();
    if(query.numRowsAffected() != 1)
      qWarning() << Q_FUNC_INFO << "query.numRowsAffected() != 1";
  }
}

void DataManagerBase::insertByRecordId(const sql::SqlRecord& record)
{
  insertByRecordInternal(record, nullptr);
}

void DataManagerBase::insertRecords(const sql::SqlRecordVector& records)
{
  insertRecords(records, tableName);
}

void DataManagerBase::insertRecords(const sql::SqlRecordVector& records, const QString& table)
{
  SqlQuery query(db);
  QString insertStr = SqlUtil(db).buildInsertStatement(table, QString(), QStringList(), false);
  query.prepare(insertStr);

  for(const SqlRecord& record : records)
  {
    QVariantList vals = record.values();
    for(int i = 0; i < vals.size(); i++)
      query.bindValue(i, vals.at(i));
    query.exec();
    if(query.numRowsAffected() != 1)
      qWarning() << Q_FUNC_INFO << "query.numRowsAffected() != 1";
  }
}

void DataManagerBase::insertByRecord(sql::SqlRecord record, int *lastInsertedRowid)
{
  if(record.contains(idColumnName))
    // Get rid of id column - it is not needed here
    record.remove(record.indexOf(idColumnName));

  insertByRecordInternal(record, lastInsertedRowid);
}

void DataManagerBase::insertByRecordInternal(const sql::SqlRecord& record, int *lastInsertedRowid)
{
  QVariantList vals = record.values();
  SqlQuery query(db);
  query.prepare("insert into " + tableName + " (" + record.fieldNames().join(", ") + ") " +
                "values(" + QString("?, ").repeated(vals.size() - 1) + " ?)");
  for(int i = 0; i < vals.size(); i++)
    query.bindValue(i, vals.at(i));
  query.exec();
  if(query.numRowsAffected() != 1)
    qWarning() << Q_FUNC_INFO << "query.numRowsAffected() != 1";
  else if(lastInsertedRowid != nullptr)
  {
    SqlQuery rowidQuery(db);
    rowidQuery.exec("select max(rowid) from " + tableName);
    if(rowidQuery.next())
      *lastInsertedRowid = rowidQuery.valueInt(0);
  }
}

void DataManagerBase::updateByRecord(sql::SqlRecord record, const QVector<int>& ids)
{
  if(record.contains(idColumnName))
    // Get rid of id column - it is not needed here
    record.remove(record.indexOf(idColumnName));

  SqlQuery query(db);
  query.prepare("update " + tableName + " set " + record.fieldNames().join(
                  " = ?, ") + " = ? where " + idColumnName + " = ?");

  QVariantList vals = record.values();
  for(int id : ids)
  {
    // For each row with given id ...
    for(int i = 0; i < vals.size(); i++)
      // ... update all given columns with given values
      query.bindValue(i, vals.at(i));
    query.bindValue(vals.size(), id);
    query.exec();
    if(query.numRowsAffected() != 1)
      qWarning() << Q_FUNC_INFO << "query.numRowsAffected() != 1";
  }
}

void DataManagerBase::clearData()
{
  backup();

  SqlTransaction transaction(db);
  removeRows();
  transaction.commit();
}

void DataManagerBase::removeRows()
{
  removeRows(tableName);
}

void DataManagerBase::removeRows(const QVector<int> ids)
{
  removeRows(tableName, ids);
}

void DataManagerBase::removeRows(const QString& column, QVariant value)
{
  removeRows(tableName, column, value);
}

void DataManagerBase::removeRows(const QString& table)
{
  SqlQuery("delete from " + table, db).exec();
}

void DataManagerBase::removeRows(const QString& table, const QVector<int> ids)
{
  SqlQuery query(db);
  query.prepare("delete from " + table + " where " + idColumnName + " = ?");

  for(int id : ids)
  {
    query.bindValue(0, id);
    query.exec();
    if(query.numRowsAffected() != 1)
      qWarning() << Q_FUNC_INFO << "query.numRowsAffected() != 1";
  }
}

void DataManagerBase::removeRows(const QString& table, const QString& column, QVariant value)
{
  SqlQuery query(db);
  query.prepare("delete from " + table + " where " + column + " = ?");
  query.bindValue(0, value);
  query.exec();
}

void DataManagerBase::getValues(QVariantList& values, const QVector<int> ids, const QString& colName)
{
  SqlQuery query(db);
  query.prepare("select " + colName + " from " + tableName + " where " + idColumnName + " = ?");

  for(int id : ids)
  {
    query.bindValue(0, id);
    query.exec();
    if(query.next())
      values.append(query.value(0));
    else
      qWarning() << Q_FUNC_INFO << "nothing found for id" << id;
  }
}

QVariant DataManagerBase::getValue(int id, const QString& colName)
{
  QVariantList values;
  getValues(values, {id}, colName);

  // Nothing found
  return values.isEmpty() ? QVariant() : values.first();
}

void DataManagerBase::getRecords(QVector<SqlRecord>& records, const QVector<int> ids)
{
  SqlQuery query(db);
  query.prepare("select * from " + tableName + " where " + idColumnName + " = ?");

  for(int id : ids)
  {
    query.bindValue(0, id);
    query.exec();
    if(query.next())
      records.append(query.record());
    else
      qWarning() << Q_FUNC_INFO << "nothing found for id" << id;
  }
}

SqlRecord DataManagerBase::getRecord(int id)
{
  QVector<SqlRecord> recs;
  getRecords(recs, {id});

  // Nothing found
  return recs.isEmpty() ? SqlRecord() : recs.first();
}

void DataManagerBase::getEmptyRecord(SqlRecord& record)
{
  record = db->record(tableName);
}

SqlRecord DataManagerBase::getEmptyRecord()
{
  SqlRecord rec;
  getEmptyRecord(rec);
  return rec;
}

atools::geo::Pos DataManagerBase::validateCoordinates(const QString& line, const QString& lonx, const QString& laty)
{
  bool lonxOk = true, latyOk = true;
  Pos pos(lonx.toFloat(&lonxOk), laty.toFloat(&latyOk));

  if(!lonxOk)
    throw Exception(tr("Longitude is not a valid number in line\n\n\"%1\"\n\nImport stopped.").arg(line));

  if(!latyOk)
    throw Exception(tr("Latitude is not a valid number in line\n\n\"%1\"\n\nImport stopped.").arg(line));

  if(!pos.isValid())
    throw Exception(tr("Coordinates are not valid in line\n\n\"%1\"\n\nImport stopped.").arg(line));

  if(pos.isNull())
    throw Exception(tr("Coordinates are null in line\n\n\"%1\"\n\nImport stopped.").arg(line));

  if(!pos.isValidRange())
    throw Exception(tr("Coordinates are not in a valid range in line\n\n\"%1\"\n\nImport stopped.").arg(line));
  return pos;
}

QString DataManagerBase::at(const QStringList& line, int index, bool nowarn)
{
  if(index < line.size())
    return line.at(index);

  if(!nowarn)
    qWarning() << "Index" << index << "not found in file";
  return QString();
}

bool DataManagerBase::hasBlob(int id, const QString& colName)
{
  SqlQuery query(db);
  query.prepare("select 1 from " + tableName + " where " + idColumnName + " = ? and length(" + colName + ") > 0");
  query.bindValue(0, id);
  query.exec();
  return query.next();
}

} // namespace userdata
} // namespace fs
} // namespace atools
