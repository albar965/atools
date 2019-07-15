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

#include "fs/xp/xpairspacewriter.h"

#include "fs/progresshandler.h"
#include "fs/userdata/airspacereaderopenair.h"

namespace atools {
namespace fs {
namespace xp {

XpAirspaceWriter::XpAirspaceWriter(atools::sql::SqlDatabase& sqlDb,
                                   const NavDatabaseOptions& opts, ProgressHandler *progressHandler,
                                   atools::fs::NavDatabaseErrors *navdatabaseErrors)
  : XpWriter(sqlDb, opts, progressHandler, navdatabaseErrors)
{
  airspaceWriter = new atools::fs::userdata::AirspaceReaderOpenAir(&sqlDb);
}

XpAirspaceWriter::~XpAirspaceWriter()
{
  delete airspaceWriter;
}

void XpAirspaceWriter::write(const QStringList& line, const XpWriterContext& context)
{
  ctx = &context;
  airspaceWriter->readLine(line, ctx->curFileId, ctx->filePath, ctx->lineNumber);
  postWrite();
}

void XpAirspaceWriter::finish(const XpWriterContext& context)
{
  ctx = &context;
  airspaceWriter->finish();
  postWrite();
}

void XpAirspaceWriter::reset()
{
  airspaceWriter->reset();
}

void XpAirspaceWriter::postWrite()
{
  for(const userdata::AirspaceReaderOpenAir::AirspaceErr& err : airspaceWriter->getErrors())
    errWarn(err.message);
  progress->incNumBoundaries(airspaceWriter->getNumAirspacesRead());

  airspaceWriter->resetErrors();
  airspaceWriter->resetNumRead();
}

} // namespace xp
} // namespace fs
} // namespace atools
