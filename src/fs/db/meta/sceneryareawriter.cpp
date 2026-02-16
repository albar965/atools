/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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

#include "fs/db/meta/sceneryareawriter.h"
#include "atools.h"
#include "fs/navdatabaseoptions.h"

#include <QDebug>
#include <QDir>

namespace atools {
namespace fs {
namespace db {

using scenery::SceneryArea;

void SceneryAreaWriter::writeObject(const SceneryArea *type)
{
  currentArea = *type;
  currentSceneryLocalPath = type->getLocalPath();

  if(getOptions().isVerbose())
    qDebug() << "Writing SceneryArea layer " << type->getLayer() << " title " << type->getTitle();

  bind(QStringLiteral(":scenery_area_id"), getNextId());
  bind(QStringLiteral(":number"), type->getAreaNumber());
  bind(QStringLiteral(":layer"), type->getLayer());
  bind(QStringLiteral(":title"), type->getTitle());
  bind(QStringLiteral(":remote_path"), atools::nativeCleanPath(type->getRemotePath()));
  bind(QStringLiteral(":local_path"), atools::nativeCleanPath(type->getLocalPath()));
  bind(QStringLiteral(":active"), type->isActive());
  bind(QStringLiteral(":required"), type->isRequired());
  bind(QStringLiteral(":exclude"), type->getExclude());

  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
