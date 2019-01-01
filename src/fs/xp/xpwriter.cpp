/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel albar965@mailbox.org
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

#include "fs/xp/xpwriter.h"
#include "fs/navdatabaseerrors.h"

#include <QDebug>

namespace atools {
namespace fs {
namespace xp {

XpWriter::XpWriter(sql::SqlDatabase& sqlDb, const NavDatabaseOptions& opts, ProgressHandler *progressHandler,
                   atools::fs::NavDatabaseErrors *navdatabaseErrors)
  : db(sqlDb), options(opts), progress(progressHandler), errors(navdatabaseErrors)
{

}

XpWriter::~XpWriter()
{

}

void XpWriter::err(const QString& msg)
{
  if(ctx != nullptr)
    qWarning() << ctx->messagePrefix() << msg;
  else
    qWarning() << msg;

  if(errors != nullptr)
  {
    if(!errors->sceneryErrors.isEmpty())
    {
      atools::fs::NavDatabaseErrors::SceneryFileError err;
      if(ctx != nullptr)
      {
        err.errorMessage = ctx->messagePrefix() + " " + msg;
        err.filepath = ctx->filePath;
      }
      else
        err.errorMessage = msg;
      err.lineNum = 0;

      errors->sceneryErrors.first().fileErrors.append(err);
    }
  }
}

void XpWriter::errWarn(const QString& msg)
{
  if(ctx != nullptr)
    qWarning() << ctx->messagePrefix() << msg;
  else
    qWarning() << msg;
}

} // namespace xp
} // namespace fs
} // namespace atools
