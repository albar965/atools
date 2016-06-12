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

#ifndef ATOOLS_FS_LB_IMPORTER_H
#define ATOOLS_FS_LB_IMPORTER_H

#include "fs/fspaths.h"

#include <QString>

namespace atools {

namespace sql {
class SqlDatabase;
}

namespace fs {
namespace lb {

class LogbookEntryFilter;

/*
 * Reads the FSX Logbook.BIN file into a Sqlite database.
 */
class LogbookLoader
{
public:
  /*
   * @param sqlDb Database to use
   */
  LogbookLoader(atools::sql::SqlDatabase *sqlDb);

  /*
   * Creates all tables, views and indexes if they do not exist and
   * loads the given file into the database. Throws Exception in
   * case of failure.
   *
   * @param filename the Logbook.BIN file
   * @param filter Defines which entries should be omitted
   * @param append If true loads only new entries into the database, otherwise
   * creates new schema and loads all.
   */
  void loadLogbook(const QString& filename, atools::fs::FsPaths::SimulatorType type,
                   const atools::fs::lb::LogbookEntryFilter& filter, bool append);

  /*
   * @return Number of logbook entries loaded.
   */
  int getNumLoaded() const
  {
    return numLoaded;
  }

  /* Drops all tables and views */
  void dropDatabase();

private:
  atools::sql::SqlDatabase *db;
  int numLoaded = 0;
};

} // namespace lb
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_LB_IMPORTER_H
