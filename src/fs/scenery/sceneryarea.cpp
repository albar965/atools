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

#include "fs/scenery/sceneryarea.h"

#include <QDir>
#include "logging/loggingdefs.h"

namespace atools {
namespace fs {
namespace scenery {

QDebug operator<<(QDebug out, const SceneryArea& area)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << "SceneryCfg["
  << "areaNumber " << area.areaNumber
  << ", title " << area.title
  << ", layer " << area.layer
  << ", active " << area.active
  << ", required " << area.required
  << ", localPath " << area.localPath
  << ", textureId " << area.textureId
  << ", remotePath" << area.remotePath
  << ", exclude " << area.exclude
  << "]";
  return out;
}

SceneryArea::SceneryArea()
  : areaNumber(0), textureId(0), layer(0), active(false), required(false)
{
}

SceneryArea::~SceneryArea()
{
}

} // namespace scenery
} // namespace fs
} // namespace atools
