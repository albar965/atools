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

#ifndef ATOOLS_SQL_DATAMANAGERBASE_H
#define ATOOLS_SQL_DATAMANAGERBASE_H

#include "sql/sqlquery.h"

#include <QCoreApplication>
#include <QList>

class QAction;

namespace atools {

namespace geo {
class Pos;
}

namespace sql {

class SqlDatabase;
class SqlRecord;
class SqlUtil;

/*
 * Base class for general table manipulation like bulk column changes and more.
 * Useful for a single table schema like userdata.
 * Uses SqlRecord as a base structure to exchange data.
 * Creates an additional layer above atools::sql classes.
 * Database ids are tracked in this instance.
 *
 * Provides an optional undo framework. This supports only changes on the main table as provided in the constructor.
 * Do not mix undo and non-undo changes in database. This will result in id conflicts.
 */
class DataManagerBase :
  public QObject
{
  Q_OBJECT

public:
  /*
   * createSqlScript is required.
   * createUndoSqlScript can be empty to leave undo system empty.
   * dropSqlScript Drops all tables including undo.
   */
  DataManagerBase(atools::sql::SqlDatabase *sqlDb, const QString& tableNameParam, const QString& idColumnNameParam,
                  const QStringList& createSqlScripts, const QString& createUndoSqlScript, const QString& dropSqlScript);
  virtual ~DataManagerBase() override;

  /* True if table is present in database */
  bool hasSchema() const;

  /* True if table is present in database and is filled */
  bool hasData() const;

  /* Row count for default table */
  int rowCount() const;

  /* Create database schema. Drops current schema if tables already exist. Initalizes ids and uses a transaction.
   * Also creates the undo schema if the script is given in the constructor. */
  void createSchema(bool verboseLogging);

  /* Drops schema tables and indexes */
  void dropSchema();

  /* true if all two undo tables are present */
  bool hasUndoSchema() const;

  /* Undo last change. Call canUndo() before running this to check if steps are available. */
  void undo()
  {
    undoRedo(true);
  }

  /* Redo last change. Call canRedo() before running this to check if steps are available. */
  void redo()
  {
    undoRedo(false);
  }

  /* Removes all undo/redo steps if undo is enabled */
  void clearUndoRedoData();

  /* Updates all columns found in the record for all rows with the given ids. Does not commit.
   * Both methods support undo. */
  void updateOneRecord(atools::sql::SqlRecord record, int id = -1);
  void updateRecords(const SqlRecord& record, const QSet<int>& ids);

  /* Updates columns for all rows with the given ids. Does not commit. Supports undo. */
  void updateField(const QString& column, const QSet<int>& ids, const QVariant& value);

  /* Adds new record to database. Creates record id automatically if id is -1.
   * Column order in record does not have to match column order in table.
   * Supports undo.
   * If id is not -1 given id will be used. Id column will be set or added on demand.
   * Included id in record is used if id is -1.
   */
  void insertOneRecord(SqlRecord record, int id = -1);

  /* Add all records from vector into database. Ids are assigned automatically.
   * Column order in record does not have to match column order in table.
   *
   * If id column is not given or id is 0 a generated id will be used.
   * Id column will be set or added on demand.
   */
  void insertRecords(SqlRecordList records); /* Supports undo. */

  void insertRecords(const atools::sql::SqlRecordList& records, const QString& table); /* No undo support */

  /* Removes all data. Supports undo. */
  void deleteAllRows();

  /* Removes one entry. Does not commit. Supports undo.*/
  void deleteOneRow(int id);

  /* Removes entries. Does not commit. Supports undo. */
  void deleteRows(const QSet<int>& ids);

  /* Removes all data */
  void deleteAllRows(const QString& table); /* No undo support */

  /* Removes entries where column equals value. Does not commit. Supports undo. */
  void deleteRows(const QString& column, const QVariant& value);

  /* Removes entries where column equals value. Does not commit. */
  void deleteRows(const QString& table, const QString& column, const QVariant& value); /* No undo support */

  /* Get records with content for ids */
  bool hasRecord(int id) const;
  atools::sql::SqlRecord getRecord(int id) const;
  void getRecords(QList<atools::sql::SqlRecord>& records, const QSet<int>& ids) const;

  /* Get values with content for ids and one column */
  QVariant getValue(int id, const QString& colName) const;
  void getValues(QVariantList& values, const QSet<int>& ids, const QString& colName) const;

  /* Empty records with schema populated */
  atools::sql::SqlRecord getEmptyRecord() const;
  void getEmptyRecord(atools::sql::SqlRecord& getRecord) const;

  atools::sql::SqlDatabase *getDatabase() const
  {
    return db;
  }

  /* true if row with the given id and column name has a BLOB larger than 0 */
  bool hasBlob(int id, const QString& colName) const;

  /* Update schema if needed. Empty body here. */
  virtual void updateSchema();

  /* Update or create undo schema if needed. Also initalizes ids and uses a transaction. */
  virtual void updateUndoSchema();

  /* Get next usable database id for a new row in the main table. This is max(rowid) + 1 */
  int getNextId()
  {
    return ++currentId;
  }

  /* Get currently used database id (i.e. from last insert). This is max(rowid). */
  int getCurrentId() const
  {
    return currentId;
  }

  /* Set actions which texts and state will be updated after undo steps accordingly. */
  void setActions(QAction *undoActionParam, QAction *redoActionParam);

  void updateUndoRedoActions();

  /* true if undo steps are available */
  bool canUndo() const;

  /* true if redo steps are available */
  bool canRedo() const;

  /* Text suffix to add to action text for single and multiple values like "Userpoint" / "Userpoints" */
  void setTextSuffix(const QString& singular, const QString& plural)
  {
    textSuffixSingular = singular;
    textSuffixPlural = plural;
  }

  /* Text for undo button or action like "Undo deleting of 54 Userpoints". Set suffices before using setTextSuffix(). */
  QString undoStepName() const
  {
    return undoRedoStepName(true);
  }

  /* Text for redo button or action like "Redo deleting of three Log Entries". Set suffices before using setTextSuffix(). */
  QString redoStepName() const
  {
    return undoRedoStepName(false);
  }

  /* true if undo tables are present and undo framework is active. */
  bool isUndoActive() const
  {
    return undoActive;
  }

  /* Activat undo system. Does not create the database schema */
  void setUndoActive(bool value)
  {
    undoActive = value;
  }

  /* Call this before doing a bulk insert like CSV import. The method will remember the currently used rowid. */
  void preUndoBulkInsert(int idFrom);

  /* Call this after doing a bulk insert like CSV import. The method will store all rows in the undo
   * table between previously remembered and current rowid. */
  void postUndoBulkInsert();

  /* Reset ids after exception */
  void abortUndoBulkInsert();

  /* Maximum undo steps. Not number of changed rows */
  void setMaximumUndoSteps(int value)
  {
    maximumUndoSteps = value;
  }

  void initQueries();
  void deInitQueries();

  /* Get number of available undo steps or 0 if none */
  int getUndoStepCount() const
  {
    return undoRedoStepCount(true /* undo */, nullptr);
  }

  /* Get number of available redo steps or 0 if none */
  int getRedoStepCount() const
  {
    return undoRedoStepCount(false /* undo */, nullptr);
  }

  /* return false to stop calculation. */
  typedef std::function<bool (int totalNumber, int currentNumber)> UndoRedoCallbackType;

  void setProgressCallback(UndoRedoCallbackType progressCallback)
  {
    callback = progressCallback;
  }

protected:
  /*
   * Simple SqlQuery wrapper which can be used to export all rows or a list of rows by id
   */
  struct QueryWrapper
  {
    /* Returns records in order of id vector */
    QueryWrapper(const QString& queryStr, const atools::sql::SqlDatabase *sqlDb, const QList<int>& idVector, const QString& idColName)
      : query(sqlDb), ids(idVector), hasIds(!ids.isEmpty())
    {
      query.prepare(queryStr + (hasIds ? QStringLiteral(" where ") + idColName + QStringLiteral(" = :id") : QString()));
    }

    /* Returns records in arbitrary order */
    QueryWrapper(const QString& queryStr, const atools::sql::SqlDatabase *sqlDb, const QSet<int>& idSet, const QString& idColName)
      : query(sqlDb), ids(idSet.constBegin(), idSet.constEnd()), hasIds(!ids.isEmpty())
    {
      query.prepare(queryStr + (hasIds ? QStringLiteral(" where ") + idColName + QStringLiteral(" = :id") : QString()));
    }

    void exec()
    {
      if(!hasIds)
        query.exec();
    }

    bool next()
    {
      if(hasIds)
      {
        if(ids.isEmpty())
          return false;

        query.bindValue(QStringLiteral(":id"), ids.takeFirst());
        query.exec();
      }
      return query.next();
    }

    atools::sql::SqlQuery query;

private:
    QList<int> ids;
    bool hasIds = false;
  };

  /* Used to remember the change type in the table "undo_data.undo_type" */
  enum UndoAction
  {
    UNDO_INVALID = '\0',
    UNDO_INSERT = 'I',
    UNDO_UPDATE = 'U',
    UNDO_DELETE = 'D',
  };

  /* Run this to replace null values with empty strings to allow queries in cleanupUserdata() and getCleanupPreview().
   * Clean up - set null string columns empty to allow join - hidden compatibility change, no need to undo */
  void preCleanup(const QStringList& columns);

  /* Reverse preCleanup() action.
   * Clean up - set empty string columns back to null - no need to undo. */
  void postCleanup(const QStringList& columns);

  /* Prints a warning of colummn does not exist */
  QString at(const QStringList& line, int index, bool nowarn = false);

  /* throws an exception if the coodinates are not valid */
  geo::Pos validateCoordinates(const QString& line, const QString& lonx, const QString& laty, int lineNum, bool checkNull);

  /* Add column with given name and type to table and undo table if not already present.
   * Returns true if table was changed. */
  bool addColumnIf(const QString& colName, const QString& colType);

  atools::sql::SqlDatabase *db = nullptr;

  QString tableName, idColumnName;

  void deleteRowsInternal(const QSet<int>& ids);

  /* Update all fields in the record given for given ids */
  void updateRecordsInternal(sql::SqlRecord record, const QSet<int>& ids);

  /* Fetch current id from max primary key of main table */
  void initCurrentId();

  /* Initialize or fetch current ids for undo tables */
  void initCurrentUndoIds();

  /* pre methods create a copy of main table rows in the table undo_data. These have to be called before any table change. */
  void preUndoInsert(SqlRecordList records);
  void preUndoUpdate(const QSet<int>& ids);
  void preUndoDelete(const QSet<int>& ids);
  void preUndoCopyInternal(const QSet<int>& ids, UndoAction undoAction);
  void preUndoDeleteAll();

  /* Copies current undo_group_id to table undo_current */
  void postUndo();

  void undoRedo(bool undo);

  QString undoRedoStepName(bool undo) const;

  /* Check if user tries to modify main table bypassing undo with undo system enabled. Throws exception. */
  void checkUndoTable(const QString& table);

  /* Truncate undo stack if maximum size is exceeded or if user changes data after doing undo steps */
  void truncateUndoIf();

  void canUndoRedo(bool& undo, bool& redo) const;

  /* Add or update id column in record */
  void updateIdColumn(atools::sql::SqlRecord& record, int id);

  /* Copy currentUndoGroupId from or to table undo_current */
  void syncCurrentUndoGroupFromDb();
  void syncCurrentUndoGroupToDb();

  int undoRedoStepCount(bool undo, UndoAction *action) const;
  bool invokeCallback(int totalNumber, int currentNumber);

  QString createUndoScript, dropScript, textSuffixSingular, textSuffixPlural;
  QStringList createScripts;

  bool undoActive = false;
  int currentId = 0, currentUndoId = 0, preBulkInsertId = -1, maximumUndoSteps = 1000;

  /* Undo id points to the step which will be reverted when calling undo(). Can be min(undo_group_id) - 1 to max(undo_group_id) */
  int currentUndoGroupId = 0;

  atools::sql::SqlUtil *util = nullptr;
  QAction *undoAction = nullptr, *redoAction = nullptr;

  atools::sql::SqlQuery *queryUndoCurrent = nullptr, *queryTruncateUndoData = nullptr, *selectMinMax = nullptr,
                        *queryTruncateUndoDataCurrent = nullptr, *querySelectUndoByGroup = nullptr, *updateUndoById = nullptr,
                        *selectUndoByGroup = nullptr, *queryInsertUndoData = nullptr, *queryDeleteRowById = nullptr,
                        *queryInsertRecords = nullptr, *querySelectById = nullptr;

  UndoRedoCallbackType callback;
  qint64 callbackTime = 0L;
};

/* Creates bulk before insert. Aborts bulk insert in destructor. */
class DataManagerUndoHandler
{
public:
  /* Prepares undo operation */
  explicit DataManagerUndoHandler(DataManagerBase *datamanagerParam, int id)
    : datamanager(datamanagerParam), numInserted(0)
  {
    datamanager->preUndoBulkInsert(id);
  }

  /* Abort insert assuming that transaction will not be commited in case of exception. */
  ~DataManagerUndoHandler()
  {
    if(!finished)
      datamanager->abortUndoBulkInsert();
  }

  /* Increment number of inserted columns. */
  void inserted()
  {
    numInserted++;
  }

  /* Finishes undo operation assuming transaction will be commited. */
  void finish()
  {
    if(numInserted > 0)
      datamanager->postUndoBulkInsert();
    finished = true;
  }

private:
  DataManagerBase *datamanager;
  int numInserted;
  bool finished = false;
};

} // namespace sql
} // namespace atools

#endif // ATOOLS_SQL_DATAMANAGERBASE_H
