/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

namespace sql {
class SqlQuery;
}

namespace fs {

class NavDatabaseOptions;
class ProgressHandler;

namespace xp {

/*
 * Reads earth_nav.dat and writes to tables, vor, ndb, marker and ils.
 */
class XpAirportIndex;

class XpNavWriter :
  public atools::fs::xp::XpWriter
{
public:
  XpNavWriter(atools::sql::SqlDatabase& sqlDb, atools::fs::xp::XpAirportIndex *xpAirportIndex,
              const atools::fs::NavDatabaseOptions& opts, atools::fs::ProgressHandler *progressHandler);
  virtual ~XpNavWriter();

  virtual void write(const QStringList& line, const XpWriterContext& context) override;
  virtual void finish(const XpWriterContext& context) override;

private:
  void initQueries();
  void deInitQueries();
  void writeVor(const QStringList& line, int curFileId, bool dmeOnly);
  void writeNdb(const QStringList& line, int curFileId);
  void writeMarker(const QStringList& line, int curFileId, atools::fs::xp::NavRowCode rowCode);

  void bindIls(const QStringList& line, int curFileId);
  void bindIlsGlideslope(const QStringList& line);
  void bindIlsDme(const QStringList& line);
  void finishIls();

  bool writingIls = false;
  const int FEATHER_LEN_NM = 9;
  const float FEATHER_WIDTH = 4.f;

  int curVorId = 0, curNdbId = 0, curMarkerId = 0, curIlsId = 0;

  atools::sql::SqlQuery *insertVorQuery = nullptr, *insertNdbQuery = nullptr,
                        *insertMarkerQuery = nullptr, *insertIlsQuery = nullptr;
  atools::fs::xp::XpAirportIndex *airportIndex;

};

} // namespace xp
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_XP_NAVWRITER_H
