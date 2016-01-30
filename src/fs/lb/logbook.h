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

#ifndef ATOOLS_FS_LB_LOGBOOK_H
#define ATOOLS_FS_LB_LOGBOOK_H

#include "fs/fspaths.h"

#include <QString>

class QFile;

namespace atools {
namespace sql {
class SqlDatabase;
class SqlQuery;
}

namespace fs {
namespace lb {

class LogbookEntry;
class LogbookEntryFilter;

/*
 * Reads a FSX logbook file and writes the entries into the given database.
 */
class Logbook
{
public:
  /*
   * Creates a Logbook object.
   *
   * @param db Destination database
   * @return
   */
  Logbook(atools::sql::SqlDatabase *sqlDb, atools::fs::fstype::SimulatorType type);

  /*
   * Reads the whole Logbook.BIN file into the
   * database. Throws Exception in case of error.
   * Additional airport information is added to the tables, if the runway.xml
   * file was loaded before.
   *
   * @param file Logbook file to read.
   * @param filter Defines which entries should be omitted
   * @param append Read only the latest entries.
   */
  void read(QFile *file, const LogbookEntryFilter& filter, bool append);

  /*
   * @return number of loaded logbook entries
   */
  int getNumLoaded() const
  {
    return numLoaded;
  }

private:
  /* calculate distance in nautical miles */
  double calcDist(double startLon, double startLat, double destLon, double destLat) const;

  int numLoaded = 0;
  atools::sql::SqlDatabase *db;
  atools::fs::fstype::SimulatorType sim;
};

} /* namespace lb */
} /* namespace fs */
} // namespace atools

#endif /* ATOOLS_FS_LB_LOGBOOK_H */
