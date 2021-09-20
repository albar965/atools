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

#ifndef ATOOLS_FS_XP_MORAWRITER_H
#define ATOOLS_FS_XP_MORAWRITER_H

#include "fs/xp/xpwriter.h"

#include <QVector>

namespace atools {

namespace fs {

class NavDatabaseOptions;
class ProgressHandler;
class NavDatabaseErrors;

namespace xp {

/*
 * Reads earth_mora.dat and writes to mora_grid table.
 */
class XpMoraWriter :
  public atools::fs::xp::XpWriter
{
public:
  XpMoraWriter(atools::sql::SqlDatabase& sqlDb,
               const atools::fs::NavDatabaseOptions& opts, atools::fs::ProgressHandler *progressHandler,
               atools::fs::NavDatabaseErrors *navdatabaseErrors);
  virtual ~XpMoraWriter() override;

  XpMoraWriter(const XpMoraWriter& other) = delete;
  XpMoraWriter& operator=(const XpMoraWriter& other) = delete;

  virtual void write(const QStringList& line, const XpWriterContext& context) override;
  virtual void finish(const XpWriterContext& context) override;
  virtual void reset() override;

private:
  QVector<QStringList> lines;
};

} // namespace xp
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_XP_MORAWRITER_H
