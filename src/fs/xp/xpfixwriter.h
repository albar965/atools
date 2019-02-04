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

#ifndef ATOOLS_FS_XP_WAYPOINTWRITER_H
#define ATOOLS_FS_XP_WAYPOINTWRITER_H

#include "fs/xp/xpwriter.h"

namespace atools {

namespace sql {
class SqlDatabase;
class SqlQuery;
}

namespace fs {

class NavDatabaseOptions;
class ProgressHandler;
class NavDatabaseErrors;

namespace common {
class AirportIndex;
}

namespace xp {

/*
 * Reads earth_fix.dat and writes to waypoint table.
 */
class XpFixWriter :
  public atools::fs::xp::XpWriter
{
public:
  XpFixWriter(atools::sql::SqlDatabase& sqlDb, atools::fs::common::AirportIndex *airportIndexParam,
              const atools::fs::NavDatabaseOptions& opts, atools::fs::ProgressHandler *progressHandler,
              atools::fs::NavDatabaseErrors *navdatabaseErrors);
  virtual ~XpFixWriter();

  virtual void write(const QStringList& line, const XpWriterContext& context) override;
  virtual void finish(const XpWriterContext& context) override;
  virtual void reset() override;

private:
  void initQueries();
  void deInitQueries();

  int curFixId = 0;
  atools::sql::SqlQuery *insertWaypointQuery = nullptr;
  atools::fs::common::AirportIndex *airportIndex;

};

} // namespace xp
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_XP_WAYPOINTWRITER_H
