/*****************************************************************************
* Copyright 2015-2021 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_FS_XP_AIRPORTMSAWRITER_H
#define ATOOLS_FS_XP_AIRPORTMSAWRITER_H

#include "fs/xp/xpwriter.h"

namespace atools {

namespace geo {
class Pos;
}
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
 * Reads earth_msa.dat, creates MSA geometry and writes to airport_msa table.
 */
class XpAirportMsaWriter :
  public atools::fs::xp::XpWriter
{
public:
  XpAirportMsaWriter(atools::sql::SqlDatabase& sqlDb, atools::fs::common::AirportIndex *airportIndexParam,
                     const atools::fs::NavDatabaseOptions& opts, atools::fs::ProgressHandler *progressHandler,
                     atools::fs::NavDatabaseErrors *navdatabaseErrors);
  virtual ~XpAirportMsaWriter() override;

  XpAirportMsaWriter(const XpAirportMsaWriter& other) = delete;
  XpAirportMsaWriter& operator=(const XpAirportMsaWriter& other) = delete;

  virtual void write(const QStringList& line, const XpWriterContext& context) override;
  virtual void finish(const XpWriterContext& context) override;
  virtual void reset() override;

private:
  void initQueries();
  void deInitQueries();

  // Fetch id, magvar and coordinates for a navaid by ident and region
  void fetchNavaid(sql::SqlQuery *query, const QString& ident, const QString& region, int& id, float& magvar,
                   atools::geo::Pos& pos);

  int curMsaId = 0;
  atools::sql::SqlQuery *insertQuery = nullptr, *waypointQuery = nullptr, *ndbQuery = nullptr, *vorQuery = nullptr;
  atools::fs::common::AirportIndex *airportIndex;
};

} // namespace xp
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_XP_AIRPORTMSAWRITER_H
