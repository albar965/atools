/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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

#include "fs/db/meta/bglfilewriter.h"
#include "fs/db/datawriter.h"
#include "fs/db/meta/sceneryareawriter.h"

#include <QFileInfo>
#include <QDateTime>
#include <QDir>

namespace atools {
namespace fs {
namespace db {

using atools::fs::bgl::BglFile;
using atools::sql::SqlQuery;

void BglFileWriter::writeObject(const BglFile *type)
{
  currentFilenamePath = type->getFilename();
  currentFilename = QFileInfo(type->getFilename()).fileName();

  if(getOptions().isVerbose())
    qDebug() << "Writing BGL file " << type->getFilename();

  QFileInfo fi(type->getFilename());

  bind(":bgl_file_id", getNextId());
  bind(":scenery_area_id", getDataWriter().getSceneryAreaWriter()->getCurrentId());
  bind(":bgl_create_time", static_cast<int>(type->getHeader().getCreationTimestamp()));
  bind(":file_modification_time", static_cast<int>(fi.lastModified().toTime_t()));
  bind(":filepath", QDir::toNativeSeparators(currentFilenamePath));
  bind(":filename", QDir::toNativeSeparators(currentFilename));
  bind(":size", type->getFilesize());

  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
