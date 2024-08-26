/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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

#include "fs/xp/xpairspacereader.h"

#include "fs/progresshandler.h"
#include "fs/userdata/airspacereaderopenair.h"

namespace atools {
namespace fs {
namespace xp {

XpAirspaceReader::XpAirspaceReader(atools::sql::SqlDatabase& sqlDb,
                                   const NavDatabaseOptions& opts, ProgressHandler *progressHandler,
                                   atools::fs::NavDatabaseErrors *navdatabaseErrors)
  : XpReader(sqlDb, opts, progressHandler, navdatabaseErrors)
{
  airspaceReader = new atools::fs::userdata::AirspaceReaderOpenAir(&sqlDb);
}

XpAirspaceReader::~XpAirspaceReader()
{
  delete airspaceReader;
}

void XpAirspaceReader::read(const QStringList& line, const XpReaderContext& context)
{
  ctx = &context;
  airspaceReader->readLine(line, ctx->curFileId, ctx->filePath, ctx->lineNumber);
  postWrite();
}

void XpAirspaceReader::finish(const XpReaderContext& context)
{
  ctx = &context;
  airspaceReader->finish();
  postWrite();
}

void XpAirspaceReader::reset()
{
  airspaceReader->reset();
}

void XpAirspaceReader::postWrite()
{
  for(const userdata::AirspaceReaderOpenAir::AirspaceErr& err : airspaceReader->getErrors())
    errWarn(err.message);
  progress->incNumBoundaries(airspaceReader->getNumAirspacesRead());

  airspaceReader->resetErrors();
  airspaceReader->resetNumRead();
}

} // namespace xp
} // namespace fs
} // namespace atools
