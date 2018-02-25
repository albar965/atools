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
}

namespace geo {
class Pos;
}
namespace fs {
namespace userdata {

enum Flag
{
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

  /* Updates the coordinates of an user defined waypoint. Does not commit. */
  void updateCoordinates(int id, const atools::geo::Pos& position);

  /* Updates columns for all rows with the given ids. Does not commit. */
  void updateField(const QString& column, const QVector<int> ids, const QVariant& value);

  /* Removes entries. Does not commit. */
  void removeRows(const QVector<int> ids);

  /* Import and export from a predefined CSV format */
  void importCsv(const QString& filepath, atools::fs::userdata::Flags flags, QChar separator, QChar escape);
  void exportCsv(const QString& filepath, atools::fs::userdata::Flags flags, QChar separator, QChar escape);

  /* Import and export user_fix.dat file from X-Plane */
  void importXplane(const QString& filepath);
  void exportXplane(const QString& filepath, atools::fs::userdata::Flags flags);

  /* Import and export Garmin GTN user waypoint database */
  void importGarmin(const QString& filepath);
  void exportGarmin(const QString& filepath, atools::fs::userdata::Flags flags);

  /* Export waypoints into a XML file for BGL compilation */
  void exportBgl(const QString& filepath);

  atools::sql::SqlDatabase *getDatabase() const
  {
    return db;
  }

private:
  /* Prints a warning of colummn does not exist */
  QString at(const QStringList& line, int index);

  /* Limits ident to upper case characters and trims length to five */
  QString adjustIdent(QString ident);

  atools::sql::SqlDatabase *db;
};

} // namespace userdata
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_USERDATAMANAGER_H
