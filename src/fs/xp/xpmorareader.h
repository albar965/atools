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

#ifndef ATOOLS_FS_XP_MORAREADER_h
#define ATOOLS_FS_XP_MORAREADER_h

#include "fs/xp/xpreader.h"

#include <QList>

namespace atools {

namespace fs {

class NavDatabaseOptions;
class ProgressHandler;
class NavDatabaseErrors;

namespace xp {

/*
 * Reads earth_mora.dat and writes to mora_grid table.
 */
class XpMoraReader :
  public atools::fs::xp::XpReader
{
public:
  XpMoraReader(atools::sql::SqlDatabase& sqlDb,
               const atools::fs::NavDatabaseOptions& opts, atools::fs::ProgressHandler *progressHandler,
               atools::fs::NavDatabaseErrors *navdatabaseErrors);
  virtual ~XpMoraReader() override;

  XpMoraReader(const XpMoraReader& other) = delete;
  XpMoraReader& operator=(const XpMoraReader& other) = delete;

  virtual void read(const QStringList& line, const XpReaderContext& context) override;
  virtual void finish(const XpReaderContext& context) override;
  virtual void reset() override;

private:
  QList<QStringList> lines;
};

} // namespace xp
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_XP_MORAREADER_h
