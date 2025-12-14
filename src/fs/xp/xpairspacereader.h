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

#ifndef ATOOLS_FS_XP_AIRSPACEREADER_h
#define ATOOLS_FS_XP_AIRSPACEREADER_h

#include "fs/xp/xpreader.h"

#include "geo/linestring.h"

namespace atools {

namespace sql {
class SqlDatabase;
class SqlQuery;
}

namespace fs {

class NavDatabaseOptions;
class ProgressHandler;
class NavDatabaseErrors;

namespace userdata {
class AirspaceReaderOpenAir;
}

namespace xp {

/*
 * Reads OpenAir files containing airspaces and writes them to the boundary table.
 */
class XpAirportIndex;

class XpAirspaceReader
  : public atools::fs::xp::XpReader
{
public:
  XpAirspaceReader(atools::sql::SqlDatabase& sqlDb, const atools::fs::NavDatabaseOptions& opts,
                   atools::fs::ProgressHandler *progressHandler,
                   atools::fs::NavDatabaseErrors *navdatabaseErrors);
  virtual ~XpAirspaceReader() override;

  XpAirspaceReader(const XpAirspaceReader& other) = delete;
  XpAirspaceReader& operator=(const XpAirspaceReader& other) = delete;

  virtual void read(const QStringList& line, const XpReaderContext& context) override;
  virtual void finish(const XpReaderContext& context) override;
  virtual void reset() override;

private:
  atools::fs::userdata::AirspaceReaderOpenAir *airspaceReader;
  void postWrite();

};

} // namespace xp
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_XP_AIRSPACEREADER_h
