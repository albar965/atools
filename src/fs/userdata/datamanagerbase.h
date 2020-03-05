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

#ifndef ATOOLS_FS_DATAMANAGERBASE_H
#define ATOOLS_FS_DATAMANAGERBASE_H

#include <QApplication>
#include <QVector>

#include "sql/sqlquery.h"

namespace atools {

namespace sql {
class SqlDatabase;
class SqlRecord;
}

namespace geo {
class Pos;
}
namespace fs {
namespace userdata {

/*
 * Base class for general table manipulation like bulk column changes and more.
 * Useful for a single table schema like userdata.
 * Uses SqlRecord as a base structure to exchange data.
 */
class DataManagerBase
{
  Q_DECLARE_TR_FUNCTIONS(UserdataManager)

public:
  DataManagerBase(atools::sql::SqlDatabase *sqlDb, const QString& tableNameParam, const QString& idColumnNameParam,
                  const QString& createSqlScript, const QString& dropSqlScript, const QString& backupFile);
  virtual ~DataManagerBase();

  /* True if table is present in database */
  bool hasSchema();

  /* True if table is present in database and is filled */
  bool hasData();

  /* Create database schema. Drops current schema if tables already exist. */
  void createSchema();

  /* Remove all data from table. */
  void clearData();

  /* Updates the coordinates of an user defined waypoint. Does not commit. */
  void updateCoordinates(int id, const atools::geo::Pos& position);

  /* Updates columns for all rows with the given ids. Does not commit. */
  void updateField(const QString& column, const QVector<int>& ids, const QVariant& value);

  /* Updates all columns found in the record for all rows with the given ids. Does not commit. */
  void updateByRecord(sql::SqlRecord getRecord, const QVector<int>& ids);

  /* Adds new record to database */
  void insertByRecord(sql::SqlRecord getRecord, int *lastInsertedRowid = nullptr);

  /* Removes entries. Does not commit. */
  void removeRows(const QVector<int> ids);

  /* Get records with content for ids */
  void getRecords(QVector<atools::sql::SqlRecord>& getRecords, const QVector<int> ids);
  atools::sql::SqlRecord getRecord(int id);

  /* Empty records with schema populated */
  void getEmptyRecord(atools::sql::SqlRecord& getRecord);
  atools::sql::SqlRecord getEmptyRecord();

  atools::sql::SqlDatabase *getDatabase() const
  {
    return db;
  }

  /* Create a CSV backup in the settings directory and roll the files over. */
  void backup();

  /* Drops schema tables and indexes */
  void dropSchema();

protected:
  /*
   * Simple SqlQuery wrapper which can be used to export all rows or a list of rows by id
   */
  class QueryWrapper
  {
public:
    QueryWrapper(const QString& queryStr, const atools::sql::SqlDatabase *sqlDb, const QVector<int>& idList,
                 const QString& idColName)
      : q(sqlDb), ids(idList), hasIds(!ids.isEmpty())
    {
      q.prepare(queryStr + (hasIds ? " where " + idColName + " = :id" : QString()));
    }

    void exec()
    {
      if(!hasIds)
        q.exec();
    }

    bool next()
    {
      if(hasIds)
      {
        if(ids.isEmpty())
          return false;

        q.bindValue(":id", ids.takeFirst());
        q.exec();
      }
      return q.next();
    }

    atools::sql::SqlQuery q;

private:
    QVector<int> ids;
    bool hasIds = false;
  };

  /* Prints a warning of colummn does not exist */
  QString at(const QStringList& line, int index, bool nowarn = false);

  /* throws an exception if the coodinates are not valid */
  geo::Pos validateCoordinates(const QString& line, const QString& lonx, const QString& laty);

  atools::sql::SqlDatabase *db = nullptr;
  QString tableName, idColumnName, /* id column name */
          createScript, dropScript, backupFilename;
};

} // namespace userdata
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_DATAMANAGERBASE_H
