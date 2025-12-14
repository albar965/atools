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

#ifndef ATOOLS_FS_XP_HOLDINGREADER_h
#define ATOOLS_FS_XP_HOLDINGREADER_h

#include "fs/xp/xpreader.h"

#include <QSet>

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
 * Reads earth_hold.dat and writes to holding table.
 */
class XpHoldingReader :
  public atools::fs::xp::XpReader
{
public:
  XpHoldingReader(atools::sql::SqlDatabase& sqlDb, atools::fs::common::AirportIndex *airportIndexParam,
                  const atools::fs::NavDatabaseOptions& opts, atools::fs::ProgressHandler *progressHandler,
                  atools::fs::NavDatabaseErrors *navdatabaseErrors);
  virtual ~XpHoldingReader() override;

  XpHoldingReader(const XpHoldingReader& other) = delete;
  XpHoldingReader& operator=(const XpHoldingReader& other) = delete;

  virtual void read(const QStringList& line, const XpReaderContext& context) override;
  virtual void finish(const XpReaderContext& context) override;
  virtual void reset() override;

private:
  void initQueries();
  void deInitQueries();

  QSet<QString> holdingSet;
  int curHoldId = 0;
  atools::sql::SqlQuery *insertQuery = nullptr;
  atools::fs::common::AirportIndex *airportIndex;
};

} // namespace xp
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_XP_HOLDINGREADER_h
