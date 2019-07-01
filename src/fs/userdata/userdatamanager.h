/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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

#include "fs/userdata/datamanagerbase.h"

namespace atools {

namespace sql {
class SqlDatabase;
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
 * Uses SqlRecord as a base structure to exchange data.
 */
class UserdataManager :
  public DataManagerBase
{
public:
  UserdataManager(atools::sql::SqlDatabase *sqlDb);
  virtual ~UserdataManager() override;

  /* Remove all data from the table which has the temporary flag set. */
  void clearTemporary();

  /* Import and export from a predefined CSV format */
  int importCsv(const QString& filepath, atools::fs::userdata::Flags flags = atools::fs::userdata::NONE,
                QChar separator = ',', QChar escape = '"');
  int exportCsv(const QString& filepath, const QVector<int>& ids = QVector<int>(),
                atools::fs::userdata::Flags flags = atools::fs::userdata::NONE,
                QChar separator = ',', QChar escape = '"');

  /* Import and export user_fix.dat file from X-Plane */
  int importXplane(const QString& filepath);
  int exportXplane(const QString& filepath, const QVector<int>& ids = QVector<int>(),
                   atools::fs::userdata::Flags flags = atools::fs::userdata::NONE);

  /* Import and export Garmin GTN user waypoint database */
  int importGarmin(const QString& filepath);
  int exportGarmin(const QString& filepath, const QVector<int>& ids = QVector<int>(),
                   atools::fs::userdata::Flags flags = atools::fs::userdata::NONE);

  /* Export waypoints into a XML file for BGL compilation */
  int exportBgl(const QString& filepath, const QVector<int>& ids);

  /* Do any schema updates if needed */
  void updateSchema();

  /* Set later to avoid circular dependeny in database */
  void setMagDecReader(atools::fs::common::MagDecReader *reader)
  {
    magDec = reader;
  }

private:
  atools::fs::common::MagDecReader *magDec;
};

} // namespace userdata
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_USERDATAMANAGER_H
