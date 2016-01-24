/*
 * SceneryAreaWriter.cpp
 *
 *  Created on: 01.05.2015
 *      Author: alex
 */

#include "sceneryareawriter.h"
#include "../datawriter.h"
#include "fs/bglreaderoptions.h"

#include <QDebug>

namespace atools {
namespace fs {
namespace writer {

using scenery::SceneryArea;
using atools::sql::SqlQuery;

void SceneryAreaWriter::writeObject(const SceneryArea *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing SceneryArea layer " << type->getLayer() << " title " << type->getTitle();

  bind(":scenery_area_id", getNextId());
  bind(":number", type->getAreaNumber());
  bind(":layer", type->getLayer());
  bind(":title", type->getTitle());
  bind(":remote_path", type->getRemotePath());
  bind(":local_path", type->getLocalPath());
  bind(":active", type->isActive());
  bind(":required", type->isRequired());
  bind(":exclude", type->getExclude());

  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
