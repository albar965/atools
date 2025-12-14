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

#ifndef ATOOLS_FS_XP_WAYPOINTREADER_h
#define ATOOLS_FS_XP_WAYPOINTREADER_h

#include "fs/xp/xpreader.h"

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
class XpFixReader :
  public atools::fs::xp::XpReader
{
public:
  XpFixReader(atools::sql::SqlDatabase& sqlDb, atools::fs::common::AirportIndex *airportIndexParam,
              const atools::fs::NavDatabaseOptions& opts, atools::fs::ProgressHandler *progressHandler,
              atools::fs::NavDatabaseErrors *navdatabaseErrors);
  virtual ~XpFixReader() override;

  XpFixReader(const XpFixReader& other) = delete;
  XpFixReader& operator=(const XpFixReader& other) = delete;

  virtual void read(const QStringList& line, const XpReaderContext& context) override;
  virtual void finish(const XpReaderContext& context) override;
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

#endif // ATOOLS_FS_XP_WAYPOINTREADER_h
