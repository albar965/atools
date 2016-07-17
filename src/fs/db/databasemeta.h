/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_FS_DB_DATABASEMETA_H
#define ATOOLS_FS_DB_DATABASEMETA_H

#include <QDateTime>

namespace atools {
namespace sql {
class SqlDatabase;
}
namespace fs {
namespace db {

/*
 * Maintains versions and load time for a navdatabases
 */
class DatabaseMeta
{
public:
  DatabaseMeta(atools::sql::SqlDatabase *sqlDb);

  /*
   * @return Major version that is stored in the database schema
   */
  int getMajorVersion() const
  {
    return majorVersion;
  }

  /*
   * @return Minor version that is stored in the database schema
   */
  int getMinorVersion() const
  {
    return minorVersion;
  }

  /*
   * @return timestamp of the last database load
   */
  QDateTime getLastLoadTime() const
  {
    return lastLoadTime;
  }

  /*
   * @return true if the database metadata table was found
   */
  bool isValid() const
  {
    return valid;
  }

  /*
   * @return true if a schema was found (checked by looking for the most important airport table)
   */
  bool hasSchema();

  /*
   * @return true if a schema was found and contains data (checked by looking for the most important airport table)
   */
  bool hasData();

  /*
   * @return true if application major version and database major version are equal
   */
  bool isDatabaseCompatible();
  bool isDatabaseCompatible(int majorVersion);

  /* Update the version information in the database */
  void updateVersion(int majorVer, int minorVer);
  void updateVersion();

  /* Update the last loaded timestamp in the database and set it to now */
  void updateTimestamp();

  /* Set database version to application version and timestamp to current time */
  void updateAll();

  /* This defines the database schema version of the application and should be updated for every incompatible
   * schema or content change */
  static const int DB_VERSION_MAJOR = 3;

  /* Minor database version of the application. Minor version differences are compatible.
   * History:
   * 1 Removed unused database fields light_flags and pattern_flags.
   * 2 Increased ILS feather length to 9 nm
   * 3 Removed taxi path weight limit
   */
  static const int DB_VERSION_MINOR = 0;

private:
  atools::sql::SqlDatabase *db;
  int majorVersion = 0, minorVersion = 0;
  QDateTime lastLoadTime;
  bool valid = false;
};

} // namespace db
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_DB_DATABASEMETA_H
