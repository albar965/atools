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

#include "sql/datamanagerbase.h"

#include "sql/sqlutil.h"
#include "sql/sqlscript.h"
#include "sql/sqltransaction.h"
#include "sql/sqlexport.h"
#include "geo/pos.h"
#include "atools.h"
#include "settings/settings.h"
#include "io/fileroller.h"
#include "exception.h"
#include "sql/sqldatabase.h"

#include <QDir>
#include <QStringBuilder>
#include <QAction>

using atools::geo::Pos;

namespace atools {
namespace sql {

DataManagerBase::DataManagerBase(sql::SqlDatabase *sqlDb, const QString& tableNameParam,
                                 const QString& idColumnNameParam, const QString& createSqlScript, const QString& createUndoSqlScript,
                                 const QString& dropSqlScript)
  : db(sqlDb), tableName(tableNameParam), idColumnName(idColumnNameParam), createScript(createSqlScript),
  createUndoScript(createUndoSqlScript), dropScript(dropSqlScript)
{
  util = new SqlUtil(db);
  undoActive = hasUndoSchema();

  initQueries();
  initCurrentId();
  initCurrentUndoIds();
  updateUndoRedoActions();
}

DataManagerBase::~DataManagerBase()
{
  deInitQueries();
  delete util;
}

bool DataManagerBase::hasSchema() const
{
  return util->hasTable(tableName);
}

bool DataManagerBase::hasData() const
{
  return util->hasTableAndRows(tableName);
}

int DataManagerBase::rowCount() const
{
  return hasData() ? util->rowCount(tableName) : 0;
}

void DataManagerBase::createSchema()
{
  qDebug() << Q_FUNC_INFO << tableName;

  deInitQueries();

  SqlTransaction transaction(db);
  SqlScript script(db, true);

  script.executeScript(createScript);

  if(!createUndoScript.isEmpty())
    script.executeScript(createUndoScript);

  initQueries();
  initCurrentId();
  initCurrentUndoIds();
  transaction.commit();

  updateUndoRedoActions();
}

void DataManagerBase::updateSchema()
{
  // No-op
}

void DataManagerBase::updateUndoSchema()
{
  if(!hasUndoSchema() && !createUndoScript.isEmpty())
  {
    qDebug() << Q_FUNC_INFO << "Adding undo schema for" << tableName;

    deInitQueries();
    SqlTransaction transaction(db);
    atools::sql::SqlScript script(db, true);
    script.executeScript(createUndoScript);
    transaction.commit();
    initQueries();
    undoActive = hasUndoSchema();
  }

  updateUndoRedoActions();
}

void DataManagerBase::dropSchema()
{
  qDebug() << Q_FUNC_INFO << tableName;

  deInitQueries();
  SqlTransaction transaction(db);
  SqlScript script(db, true);
  script.executeScript(dropScript);
  transaction.commit();
}

void DataManagerBase::initCurrentId()
{
  if(util->hasTable(tableName))
    currentId = util->getMaxId(tableName, idColumnName);
  undoActive = hasUndoSchema();
}

void DataManagerBase::initCurrentUndoIds()
{
  undoActive = hasUndoSchema();

  if(undoActive)
  {
    // Get current id for undo table
    currentUndoId = util->getMaxId("undo_data", "undo_data_id");

    if(!util->hasRows("undo_current"))
    {
      // Initialize current undo step to top of table and write to undo_current table
      currentUndoGroupId = util->getMaxId("undo_data", "undo_group_id");
      syncCurrentUndoGroupToDb();
    }
    else
      // Load state from undo_current table
      syncCurrentUndoGroupFromDb();
  }
}

bool DataManagerBase::addColumnIf(const QString& colName, const QString& colType)
{
  qDebug() << Q_FUNC_INFO << colName << colType;
  SqlTransaction transaction(db);
  bool retval = util->addColumnIf(tableName, colName, colType);

  // Also add column to undo table if present
  bool retvalUndo = false;
  if(util->hasTable("undo_data"))
    retvalUndo = util->addColumnIf("undo_data", colName, colType);
  transaction.commit();

  qDebug() << Q_FUNC_INFO << colName << colType << "added" << retval << "added undo" << retvalUndo;
  return retval;
}

void DataManagerBase::setActions(QAction *undoActionParam, QAction *redoActionParam)
{
  undoAction = undoActionParam;
  redoAction = redoActionParam;
  updateUndoRedoActions();
}

void DataManagerBase::insertOneRecord(SqlRecord record, int id)
{
  if(id == -1)
    // Use generated id if not given
    id = getNextId();

  // Insert id in record if id column is 0 or missing
  updateIdColumn(record, id);
  preUndoInsert({record});

  queryInsertRecords->bindAndExecRecord(record, ":");
  postUndo();
}

void DataManagerBase::insertRecords(sql::SqlRecordList records)
{
  // Insert id in records if id column is 0 or missing
  for(SqlRecord& record : records)
    updateIdColumn(record, getNextId());

  preUndoInsert(records);
  queryInsertRecords->bindAndExecRecords(records, ":");
  postUndo();
}

void DataManagerBase::insertRecords(const SqlRecordList& records, const QString& table)
{
  // No undo support for this method
  checkUndoTable(table);

  SqlQuery insert(db);
  insert.prepare(util->buildInsertStatement(table));
  insert.bindAndExecRecords(records, ":");
}

void DataManagerBase::updateField(const QString& column, const QVector<int>& ids, const QVariant& value)
{
  if(!ids.isEmpty())
  {
    preUndoUpdate(ids);
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
    postUndo();
  }
}

void DataManagerBase::updateOneRecord(SqlRecord record, int id)
{
  if(id == -1)
    // Use id from record if not given as parameter - updateRecords() removes the id column
    id = record.valueInt(idColumnName);

  updateRecords(record, {id});
}

void DataManagerBase::updateRecords(const sql::SqlRecord& record, const QVector<int>& ids)
{
  if(!ids.isEmpty())
  {
    preUndoUpdate(ids);
    updateRecordsInternal(record, ids);
    postUndo();
  }
}

void DataManagerBase::updateRecordsInternal(sql::SqlRecord record, const QVector<int>& ids)
{
  if(!ids.isEmpty())
  {
    // Remove id column since ids are used in the where clause
    if(record.contains(idColumnName))
      record.remove(record.indexOf(idColumnName));

    // Create list of bindings extracted from record
    QStringList binds;
    for(const QString& field : record.fieldNames())
      binds.append(field % " = :" % field);

    SqlQuery query(db);
    query.prepare("update " % tableName % " set " % binds.join(", ") % " where " % idColumnName % " = :id");

    // Bind all record values
    query.bindRecord(record, ":");

    // Now update table columns for all given ids
    for(int id : ids)
    {
      query.bindValue(":id", id);
      query.exec();
      if(query.numRowsAffected() != 1)
        qWarning() << Q_FUNC_INFO << "query.numRowsAffected() != 1. id " << id;
    }
  }
}

void DataManagerBase::deleteAllRows()
{
  preUndoDeleteAll();
  SqlQuery("delete from " % tableName, db).exec();
  postUndo();
}

void DataManagerBase::deleteOneRow(int id)
{
  preUndoDelete({id});
  deleteRowsInternal({id});
  postUndo();
}

void DataManagerBase::deleteRows(const QVector<int>& ids)
{
  if(!ids.isEmpty())
  {
    preUndoDelete(ids);
    deleteRowsInternal(ids);
    postUndo();
  }
}

void DataManagerBase::deleteAllRows(const QString& table)
{
  // Undo not supported for this method
  checkUndoTable(table);
  SqlQuery("delete from " % table, db).exec();
}

void DataManagerBase::deleteRowsInternal(const QVector<int>& ids)
{
  for(int id : ids)
  {
    queryDeleteRowById->bindValue(0, id);
    queryDeleteRowById->exec();
    if(queryDeleteRowById->numRowsAffected() != 1)
      qWarning() << Q_FUNC_INFO << "query.numRowsAffected() != 1. id " << id;
  }
}

void DataManagerBase::deleteRows(const QString& column, const QVariant& value)
{
  // Collect all ids for rows to delete
  QVector<int> ids;
  SqlQuery query(db);
  query.prepare("select " % idColumnName % " from " % tableName % " where " % column % " = ?");
  query.bindValue(0, value);
  query.exec();
  while(query.next())
    ids.append(query.valueInt(0));

  // Also takes care about undo
  deleteRows(ids);
}

void DataManagerBase::deleteRows(const QString& table, const QString& column, const QVariant& value)
{
  // Undo not supported for this method
  checkUndoTable(table);

  SqlQuery query(db);
  query.prepare("delete from " + table + " where " + column + " = ?");
  query.bindValue(0, value);
  query.exec();
}

void DataManagerBase::getValues(QVariantList& values, const QVector<int>& ids, const QString& colName) const
{
  if(!ids.isEmpty())
  {
    SqlQuery query(db);
    query.prepare("select " % colName % " from " % tableName % " where " % idColumnName % " = ?");

    // Fetch column value for all ids
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
}

QVariant DataManagerBase::getValue(int id, const QString& colName) const
{
  QVariantList values;
  getValues(values, {id}, colName);

  // Nothing found
  return values.isEmpty() ? QVariant() : values.first();
}

void DataManagerBase::getRecords(QVector<SqlRecord>& records, const QVector<int> ids) const
{
  for(int id : ids)
  {
    querySelectById->bindValue(0, id);
    querySelectById->exec();
    if(querySelectById->next())
      records.append(querySelectById->record());
    else
      qWarning() << Q_FUNC_INFO << "nothing found for id" << id;
  }
}

SqlRecord DataManagerBase::getRecord(int id) const
{
  QVector<SqlRecord> recs;
  getRecords(recs, {id});

  // Nothing found
  return recs.isEmpty() ? SqlRecord() : recs.first();
}

bool DataManagerBase::hasRecord(int id) const
{
  return util->hasRows(tableName, QString("%1 = %2").arg(idColumnName).arg(id));
}

void DataManagerBase::getEmptyRecord(SqlRecord& record) const
{
  record = db->record(tableName);
}

SqlRecord DataManagerBase::getEmptyRecord() const
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

bool DataManagerBase::hasBlob(int id, const QString& colName) const
{
  SqlQuery query(db);
  query.prepare("select 1 from " % tableName % " where " % idColumnName % " = ? and length(" % colName % ") > 0");
  query.bindValue(0, id);
  query.exec();
  return query.next();
}

void DataManagerBase::updateIdColumn(SqlRecord& record, int id)
{
  if(id != -1)
  {
    if(!record.contains(idColumnName))
      // No column - add field and value
      record.insertFieldAndValue(0, idColumnName, id);
    else if(record.valueInt(idColumnName) == 0)
      // Has id column but it is null - set
      record.setValue(idColumnName, id);
  }
}

bool DataManagerBase::hasUndoSchema() const
{
  return util->hasTable("undo_data") && util->hasTable("undo_current");
}

void DataManagerBase::preUndoInsert(sql::SqlRecordList records)
{
  if(undoActive)
  {
    truncateUndoIf();
    currentUndoGroupId++;

    // Add all given records to the undo table - this is one undo group
    for(SqlRecord& rec : records)
    {
      rec.insertFieldAndValue(0, "undo_data_id", ++currentUndoId);
      rec.insertFieldAndValue(1, "undo_group_id", currentUndoGroupId);
      rec.insertFieldAndValue(2, "undo_type", atools::charToStr(UNDO_INSERT));
    }

    queryInsertUndoData->bindAndExecRecords(records, ":");
  }
}

void DataManagerBase::preUndoBulkInsert()
{
  if(undoActive)
  {
    truncateUndoIf();

    // Remember current id
    preBulkInsertId = currentId;
  }
}

void DataManagerBase::postUndoBulkInsert()
{
  if(undoActive)
  {
    if(preBulkInsertId == -1)
      throw atools::Exception(tr("preUndoBulkInsert() not called"));

    currentUndoGroupId++;

    // get current (max) id from table
    initCurrentId();

    // Fetch all ids that were inserted in the bulk import
    SqlQuery select(db);
    select.prepare("select * from " % tableName % " where " % idColumnName % " between ? and ?");
    select.bindValue(0, preBulkInsertId);
    select.bindValue(1, currentId);
    select.exec();

    // Add all  records to the undo table - this is one undo group
    while(select.next())
    {
      SqlRecord rec = select.record();
      rec.insertFieldAndValue(0, "undo_data_id", ++currentUndoId);
      rec.insertFieldAndValue(1, "undo_group_id", currentUndoGroupId);
      rec.insertFieldAndValue(2, "undo_type", atools::charToStr(UNDO_INSERT));
      queryInsertUndoData->bindRecord(rec, ":");
      queryInsertUndoData->exec();
    }

    // Update current id
    preBulkInsertId = -1;

    syncCurrentUndoGroupToDb();
    updateUndoRedoActions();
  }
}

void DataManagerBase::preUndoDeleteAll()
{
  if(undoActive)
  {
    truncateUndoIf();
    currentUndoGroupId++;

    // Add all records to delete to the undo table
    SqlQuery selectQuery("select * from " % tableName, db);
    selectQuery.exec();
    while(selectQuery.next())
    {
      util->copyRowValues(selectQuery, *queryInsertUndoData);

      queryInsertUndoData->bindValue(":undo_data_id", ++currentUndoId);
      queryInsertUndoData->bindValue(":undo_group_id", currentUndoGroupId);
      queryInsertUndoData->bindValue(":undo_type", atools::charToStr(UNDO_DELETE));
      queryInsertUndoData->exec();
    }
  }
}

void DataManagerBase::preUndoUpdate(const QVector<int>& ids)
{
  preUndoCopyInternal(ids, UNDO_UPDATE);
}

void DataManagerBase::preUndoDelete(const QVector<int>& ids)
{
  preUndoCopyInternal(ids, UNDO_DELETE);
}

void DataManagerBase::preUndoCopyInternal(const QVector<int>& ids, UndoAction action)
{
  if(undoActive)
  {
    truncateUndoIf();
    currentUndoGroupId++;

    QueryWrapper wrapped("select * from " % tableName, db, ids, idColumnName);
    wrapped.exec();
    while(wrapped.next())
    {
      util->copyRowValues(wrapped.query, *queryInsertUndoData);

      queryInsertUndoData->bindValue(":undo_data_id", ++currentUndoId);
      queryInsertUndoData->bindValue(":undo_group_id", currentUndoGroupId);
      queryInsertUndoData->bindValue(":undo_type", atools::charToStr(action));
      queryInsertUndoData->exec();
    }
  }
}

void DataManagerBase::postUndo()
{
  // Write current undo state to database
  syncCurrentUndoGroupToDb();

  // Update action state and text
  updateUndoRedoActions();
}

void DataManagerBase::checkUndoTable(const QString& table)
{
  if(undoActive && table == tableName)
    // Undo is enable and user is trying to modifiy main table
    throw atools::Exception(tr("Attempt to modify table \"%1\" bypassing active undo function.").arg(table));
}

void DataManagerBase::undoRedo(bool undo)
{
  if(undoActive)
  {
    if(!undo)
      // Increment before for redo
      currentUndoGroupId++;

    // Get all records from the undo table for the current step to undo/redo
    selectUndoByGroup->bindValue(":id", currentUndoGroupId);
    selectUndoByGroup->exec();

    while(selectUndoByGroup->next())
    {
      SqlRecord undoRec = selectUndoByGroup->record();

      // Get all undo table values before removing the fields
      UndoAction action = static_cast<UndoAction>(atools::strToChar(undoRec.valueStr("undo_type")));
      int id = undoRec.valueInt(idColumnName);
      int undoDataId = undoRec.valueInt("undo_data_id");

      // Get rid if fields except the ones from the main table
      undoRec.remove({"undo_data_id", "undo_group_id", "undo_type"});

      switch(action)
      {
        case atools::sql::DataManagerBase::UNDO_INSERT:
          if(undo)
            // Revert insert - delete value and keep copy in undo table for redo
            deleteRowsInternal({id});
          else
          {
            // Insert again - keep copy in undo table for undo
            updateIdColumn(undoRec, id);
            queryInsertRecords->bindAndExecRecord(undoRec, ":");
          }
          break;

        case atools::sql::DataManagerBase::UNDO_UPDATE:
          {
            // Undo: Revert update - swap values
            // Redo: Apply update again - swap values

            // Get temporary values from main table
            SqlRecord tempRecord = getRecord(id);

            // Copy from undo table back to original table
            updateRecordsInternal(undoRec, {id});

            // Update undo table with temp value from original table
            updateUndoById->bindRecord(tempRecord, ":");
            updateUndoById->bindValue(":id", undoDataId);
            updateUndoById->exec();
          }
          break;

        case atools::sql::DataManagerBase::UNDO_DELETE:
          if(undo)
          {
            // Revert delete - insert values from undo table and keep copy in undo table for redo
            updateIdColumn(undoRec, id);
            queryInsertRecords->bindAndExecRecord(undoRec, ":");
          }
          else
            // Delete again - keep copy in undo table for undo
            deleteRowsInternal({id});

          break;
      }
    }
    if(undo)
      currentUndoGroupId--;
    syncCurrentUndoGroupToDb();
    updateUndoRedoActions();
  }
}

void DataManagerBase::updateUndoRedoActions()
{
  if(undoAction != nullptr || redoAction != nullptr)
  {
    bool undo, redo;
    canUndoRedo(undo, redo);

    if(undoAction != nullptr)
    {
      undoAction->setEnabled(undo);
      undoAction->setText(undo ? tr("&%1").arg(undoStepName()) : tr("&Undo%1").arg(textSuffixSingular));
    }

    if(redoAction != nullptr)
    {
      redoAction->setEnabled(redo);
      redoAction->setText(redo ? tr("&%1").arg(redoStepName()) : tr("&Redo%1").arg(textSuffixSingular));
    }
  }
}

bool DataManagerBase::canUndo() const
{
  if(undoActive)
  {
    bool undo, dummy;
    canUndoRedo(undo, dummy);
    return undo;
  }
  return false;
}

bool DataManagerBase::canRedo() const
{
  if(undoActive)
  {
    bool dummy, redo;
    canUndoRedo(dummy, redo);
    return redo;
  }
  return false;
}

void DataManagerBase::canUndoRedo(bool& undo, bool& redo) const
{
  undo = redo = false;

  selectMinMax->exec();
  if(selectMinMax->next())
  {
    int minGroupId = selectMinMax->valueInt(0);
    int maxGroupId = selectMinMax->valueInt(1);
    int count = selectMinMax->valueInt(2);

    if(count > 0)
    {
      // Undo id points to the step which will be reverted when calling undo()
      undo = currentUndoGroupId >= minGroupId;
      redo = currentUndoGroupId < maxGroupId;
    }
  }
}

QString DataManagerBase::undoRedoStepName(bool undo) const
{
  QString text;
  if(undoActive)
  {
    // Get row count for current undo/redo step
    querySelectUndoByGroup->bindValue(0, undo ? currentUndoGroupId : currentUndoGroupId + 1);
    querySelectUndoByGroup->exec();

    if(querySelectUndoByGroup->next())
    {
      // Use number of modified/added/deleted rows in text
      int num = querySelectUndoByGroup->valueInt(1);

      // Build number text
      QString numText;
      if(num == 0)
        numText = tr("no %1").arg(textSuffixSingular);
      else if(num == 1)
        numText = tr("one %1").arg(textSuffixSingular);
      else if(num == 2)
        numText = tr("two %1").arg(textSuffixPlural);
      else
        numText = tr("%1 %2").arg(num).arg(textSuffixPlural);

      QString undoRedoText(undo ? tr("Undo") : tr("Redo"));

      UndoAction action = static_cast<UndoAction>(atools::strToChar(querySelectUndoByGroup->valueStr(0)));
      switch(action)
      {
        case atools::sql::DataManagerBase::UNDO_INSERT:
          text = tr("%1 adding of %2").arg(undoRedoText).arg(numText);
          break;
        case atools::sql::DataManagerBase::UNDO_UPDATE:
          text = tr("%1 editing of %2").arg(undoRedoText).arg(numText);
          break;
        case atools::sql::DataManagerBase::UNDO_DELETE:
          text = tr("%1 deleting of %2").arg(undoRedoText).arg(numText);
          break;
      }
    }
  }
  return text;
}

void DataManagerBase::truncateUndoIf()
{
  // Delete all undo rows above the current one to truncate the redo stack in case of change after doing undo
  queryTruncateUndoDataCurrent->bindValue(0, currentUndoGroupId);
  queryTruncateUndoDataCurrent->exec();

  // Now check if maximum undo steps are exceeded and remove all rows beginning from the first one (older steps)
  selectMinMax->exec();
  if(selectMinMax->next())
  {
    int maxUndoGroupId = selectMinMax->valueInt(1);
    int count = selectMinMax->valueInt(2);

    if(count > maximumUndoSteps + 10)
    {
      int id = maxUndoGroupId - maximumUndoSteps;
      queryTruncateUndoData->bindValue(0, id);
      queryTruncateUndoData->exec();
    }
  }
}

void DataManagerBase::syncCurrentUndoGroupToDb()
{
  if(undoActive)
  {
    // Copy from field to table undo_current
    queryUndoCurrent->bindValue(0, currentUndoGroupId);
    queryUndoCurrent->exec();

    if(queryUndoCurrent->numRowsAffected() != 1)
      throw Exception(tr("Table undo_current is empty."));
  }
}

void DataManagerBase::syncCurrentUndoGroupFromDb()
{
  if(undoActive)
  {
    // Copy from table undo_current to field
    currentUndoGroupId = util->getValueInt("select undo_group_id from undo_current", -1);

    if(currentUndoGroupId == -1)
      throw Exception(tr("Table undo_current is empty."));
  }
}

void DataManagerBase::initQueries()
{
  deInitQueries();

  undoActive = hasUndoSchema();

  if(undoActive)
  {
    queryUndoCurrent = new SqlQuery(db);
    queryUndoCurrent->prepare("update undo_current set undo_group_id = ?");

    queryTruncateUndoData = new SqlQuery(db);
    queryTruncateUndoData->prepare("delete from undo_data where undo_group_id < ?");

    selectMinMax = new SqlQuery("select min(undo_group_id), max(undo_group_id), count(distinct undo_group_id) from undo_data", db);

    queryTruncateUndoDataCurrent = new SqlQuery(db);
    queryTruncateUndoDataCurrent->prepare("delete from undo_data where undo_group_id > ?");

    querySelectUndoByGroup = new SqlQuery(db);
    querySelectUndoByGroup->prepare("select undo_type, count(1) from undo_data where undo_group_id = ? group by undo_group_id");

    updateUndoById = new SqlQuery(db);
    updateUndoById->prepare(util->buildUpdateStatement("undo_data", "undo_data_id = :id", {"undo_data_id", "undo_group_id", "undo_type"}));

    selectUndoByGroup = new SqlQuery(db);
    selectUndoByGroup->prepare("select * from undo_data where undo_group_id = :id");

    queryInsertUndoData = new SqlQuery(db);
    queryInsertUndoData->prepare(util->buildInsertStatement("undo_data"));
  }

  if(hasSchema())
  {
    queryDeleteRowById = new SqlQuery(db);
    queryDeleteRowById->prepare("delete from " % tableName % " where " % idColumnName % " = ?");

    queryInsertRecords = new SqlQuery(db);
    queryInsertRecords->prepare(util->buildInsertStatement(tableName));

    querySelectById = new SqlQuery(db);
    querySelectById->prepare("select * from " % tableName % " where " % idColumnName % " = ?");
  }
}

void DataManagerBase::deInitQueries()
{
  delete queryUndoCurrent;
  queryUndoCurrent = nullptr;

  delete queryTruncateUndoData;
  queryTruncateUndoData = nullptr;

  delete selectMinMax;
  selectMinMax = nullptr;

  delete queryTruncateUndoDataCurrent;
  queryTruncateUndoDataCurrent = nullptr;

  delete querySelectUndoByGroup;
  querySelectUndoByGroup = nullptr;

  delete updateUndoById;
  updateUndoById = nullptr;

  delete selectUndoByGroup;
  selectUndoByGroup = nullptr;

  delete queryInsertUndoData;
  queryInsertUndoData = nullptr;

  delete queryDeleteRowById;
  queryDeleteRowById = nullptr;

  delete queryInsertRecords;
  queryInsertRecords = nullptr;

  delete querySelectById;
  querySelectById = nullptr;
}

} // namespace sql
} // namespace atools
