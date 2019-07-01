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

#ifndef ATOOLS_FS_LOGDATAMANAGER_H
#define ATOOLS_FS_LOGDATAMANAGER_H

#include "fs/userdata/datamanagerbase.h"

namespace atools {

namespace sql {
class SqlDatabase;
}

namespace fs {
namespace userdata {

/*
 * Contains special functionality around the logbook database.
 */
class LogdataManager :
  public DataManagerBase
{
public:
  LogdataManager(atools::sql::SqlDatabase *sqlDb);
  virtual ~LogdataManager() override;

  /* Import and export from a custom CSV format which covers all fields in the logbook table. */
  int importCsv(const QString& filepath);
  int exportCsv(const QString& filepath);

  /* Import X-Plane logbook. Needs a function fetchAirport
   * that resolves airport ident to name and position. */
  int importXplane(const QString& filepath,
                   const std::function<void(atools::geo::Pos& pos, QString& name,
                                            const QString& ident)>& fetchAirport);

  /* Update schema to latest. Checks for new columns and tables. */
  void updateSchema();

private:
};

} // namespace userdata
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_LOGDATAMANAGER_H
