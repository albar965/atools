/*****************************************************************************
* Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_FS_USERDATAMANAGER_H
#define ATOOLS_FS_USERDATAMANAGER_H

#include <QApplication>

namespace atools {

namespace sql {
class SqlDatabase;
class SqlRecord;
}

namespace geo {
class Pos;
}
namespace fs {

namespace common {
class MagDecReader;
}

namespace userdata {

enum Flag
{
  NONE,

  /* CSV import/export */
  CSV_HEADER = 1 << 0,

  /* All formats export expect BGL */
  APPEND = 1 << 1
};

Q_DECLARE_FLAGS(Flags, Flag);
Q_DECLARE_OPERATORS_FOR_FLAGS(atools::fs::userdata::Flags);

/*
 * Contains functionality around the userdata database which keeps user defined waypoints, bookmarks and others.
 */
class UserdataManager
{
  Q_DECLARE_TR_FUNCTIONS(UserdataManager)

public:
  UserdataManager(atools::sql::SqlDatabase *sqlDb);
  ~UserdataManager();

  /* True if table userdata is presend in database */
  bool hasSchema();

  /* True if table userdata is presend in database and is filled*/
  bool hasData();

  /* Create database schema. Drops current schema if tables already exist. */
  void createSchema();

  /* Remove all data from the table. */
  void clearData();

  /* Updates the coordinates of an user defined waypoint. Does not commit. */
  void updateCoordinates(int id, const atools::geo::Pos& position);

  /* Updates columns for all rows with the given ids. Does not commit. */
  void updateField(const QString& column, const QVector<int>& ids, const QVariant& value);

  /* Updates all columns found in the record for all rows with the given ids. Does not commit. */
  void updateByRecord(sql::SqlRecord getRecord, const QVector<int>& ids);

  /* Adds new record to database */
  void insertByRecord(sql::SqlRecord getRecord);

  /* Removes entries. Does not commit. */
  void removeRows(const QVector<int> ids);

  /* Get records with content for ids */
  void getRecords(QVector<atools::sql::SqlRecord>& getRecords, const QVector<int> ids);
  atools::sql::SqlRecord getRecord(int id);

  /* Empty records with schema populated */
  void getEmptyRecord(atools::sql::SqlRecord& getRecord);
  atools::sql::SqlRecord getEmptyRecord();

  void commit();

  /* Import and export from a predefined CSV format */
  int importCsv(const QString& filepath, atools::fs::userdata::Flags flags, QChar separator, QChar escape);
  int exportCsv(const QString& filepath, const QVector<int>& ids, atools::fs::userdata::Flags flags,
                QChar separator, QChar escape);

  /* Import and export user_fix.dat file from X-Plane */
  int importXplane(const QString& filepath);
  int exportXplane(const QString& filepath, const QVector<int>& ids, atools::fs::userdata::Flags flags);

  /* Import and export Garmin GTN user waypoint database */
  int importGarmin(const QString& filepath);
  int exportGarmin(const QString& filepath, const QVector<int>& ids, atools::fs::userdata::Flags flags);

  /* Export waypoints into a XML file for BGL compilation */
  int exportBgl(const QString& filepath, const QVector<int>& ids);

  atools::sql::SqlDatabase *getDatabase() const
  {
    return db;
  }

  void setMagDecReader(atools::fs::common::MagDecReader *value);

private:
  /* Prints a warning of colummn does not exist */
  QString at(const QStringList& line, int index);

  atools::sql::SqlDatabase *db;
  atools::fs::common::MagDecReader *magDec;
};

} // namespace userdata
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_USERDATAMANAGER_H
