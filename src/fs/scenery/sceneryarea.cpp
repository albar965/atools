/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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
#include <QDebug>

namespace atools {
namespace fs {
namespace scenery {

SceneryArea::SceneryArea()
{

}

SceneryArea::SceneryArea(int areaNum, int layerNum, const QString& sceneryTitle, const QString& sceneryLocalPath)
  : areaNumber(areaNum), layer(layerNum), active(true), title(sceneryTitle), localPath(sceneryLocalPath)
{
  fixTitle();
}

SceneryArea::SceneryArea(int num, const QString& sceneryTitle, const QString& sceneryLocalPath)
  : areaNumber(num), layer(num), active(true), title(sceneryTitle), localPath(sceneryLocalPath)
{
  fixTitle();
}

void SceneryArea::fixTitle()
{
  if(title.isEmpty())
    title = QString("Unnamed scenery number %1 layer %2").arg(areaNumber).arg(layer);
}

QDebug operator<<(QDebug out, const SceneryArea& area)
{
  QDebugStateSaver saver(out);

  out.nospace() << "SceneryArea["
                << "number " << area.areaNumber
                << ", title " << area.title
                << ", layer " << area.layer
                << ", active " << area.active
                << ", highPriority " << area.highPriority
                << ", required " << area.required
                << ", localPath " << area.localPath
                << ", textureId " << area.textureId
                << ", remotePath" << area.remotePath
                << ", exclude " << area.exclude
                << "]";
  return out;
}

} // namespace scenery
} // namespace fs
} // namespace atools
