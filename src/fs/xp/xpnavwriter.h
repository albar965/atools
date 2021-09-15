/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_FS_XP_NAVWRITER_H
#define ATOOLS_FS_XP_NAVWRITER_H

#include "fs/xp/xpconstants.h"
#include "fs/xp/xpwriter.h"

namespace atools {

namespace geo {
class Pos;
}
namespace sql {
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
 * Reads earth_nav.dat and writes to tables, vor, ndb, marker and ils.
 */

class XpNavWriter :
  public atools::fs::xp::XpWriter
{
public:
  XpNavWriter(atools::sql::SqlDatabase& sqlDb, atools::fs::common::AirportIndex *airportIndexParam,
              const atools::fs::NavDatabaseOptions& opts, atools::fs::ProgressHandler *progressHandler,
              atools::fs::NavDatabaseErrors *navdatabaseErrors);
  virtual ~XpNavWriter() override;

  XpNavWriter(const XpNavWriter& other) = delete;
  XpNavWriter& operator=(const XpNavWriter& other) = delete;

  virtual void write(const QStringList& line, const XpWriterContext& context) override;
  virtual void finish(const XpWriterContext& context) override;
  virtual void reset() override;

private:
  void initQueries();
  void deInitQueries();
  void writeVor(const QStringList& line, int curFileId, bool dmeOnly);
  void writeNdb(const QStringList& line, int curFileId, const XpWriterContext& context);
  void writeMarker(const QStringList& line, int curFileId, atools::fs::xp::NavRowCode rowCode);

  void writeIlsSbasGbas(const QStringList& line, atools::fs::xp::NavRowCode rowCode, const XpWriterContext& context);
  void updateIlsGlideslope(const QStringList& line);
  void updateIlsDme(const QStringList& line);
  void updateSbasGbasThreshold(const QStringList& line);
  void assignIlsGeometry(atools::sql::SqlQuery *query, const atools::geo::Pos& pos, float heading);

  QChar ilsType(const QString& name, bool glideslope);

  const int FEATHER_LEN_NM = 9;
  const float FEATHER_WIDTH = 4.f;

  int curVorId = 0, curNdbId = 0, curMarkerId = 0, curIlsId = 0;

  QString ilsName;

  atools::sql::SqlQuery *insertVorQuery = nullptr, *insertNdbQuery = nullptr,
                        *insertMarkerQuery = nullptr, *insertIlsQuery = nullptr,
                        *updateIlsGsTypeQuery = nullptr, *updateIlsDmeQuery = nullptr,
                        *updateSbasGbasThresholdQuery = nullptr;
  atools::fs::common::AirportIndex *airportIndex;

};

} // namespace xp
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_XP_NAVWRITER_H
