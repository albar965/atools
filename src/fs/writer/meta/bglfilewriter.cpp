/*
 * BglFileWriter.cpp
 *
 *  Created on: 01.05.2015
 *      Author: alex
 */

#include "fs/writer/meta/bglfilewriter.h"
#include "fs/writer/datawriter.h"
#include "fs/writer/meta/sceneryareawriter.h"

#include <QFileInfo>
#include <QDateTime>

namespace atools {
namespace fs {
namespace writer {

using atools::fs::bgl::BglFile;
using atools::sql::SqlQuery;

void BglFileWriter::writeObject(const BglFile *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing BGL file " << type->getFilename();

  QFileInfo fi(type->getFilename());

  bind(":bgl_file_id", getNextId());
  bind(":scenery_area_id", getDataWriter().getSceneryAreaWriter()->getCurrentId());
  bind(":bgl_create_time", static_cast<int>(type->getHeader().getCreationTimestamp()));
  bind(":file_modification_time", static_cast<int>(fi.lastModified().toTime_t()));
  bind(":filename", type->getFilename());
  bind(":size", type->getFilesize());

  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
