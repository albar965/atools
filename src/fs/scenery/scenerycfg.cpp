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

#include "fs/scenery/scenerycfg.h"
#include "fs/scenery/sceneryarea.h"

#include <fstream>
#include <iostream>
#include <algorithm>

#include <QDebug>

namespace atools {
namespace fs {
namespace scenery {

SceneryCfg::SceneryCfg(const QString& textCodec)
  : AbstractIniReader(textCodec), cleanOnExit(false)
{
}

SceneryCfg::~SceneryCfg()
{
}

void SceneryCfg::appendArea(const SceneryArea& area)
{
  areaEntries.append(area);
}

void SceneryCfg::onStartDocument(const QString& filename)
{
  Q_UNUSED(filename);
  areaEntries.clear();
}

void SceneryCfg::onEndDocument(const QString& filename)
{
  Q_UNUSED(filename);

  if(areaEntries.isEmpty())
    throwException(tr("No valid scenery areas found"));
  sortAreas();
}

void SceneryCfg::sortAreas()
{
  // Sort areas by layer
  std::sort(areaEntries.begin(), areaEntries.end(), [](const SceneryArea& a1, const SceneryArea& a2) -> bool
        {
          if(a1.isHighPriority() == a2.isHighPriority())
            return a1.getLayer() < a2.getLayer();
          else
            return a1.isHighPriority() < a2.isHighPriority();
        });
}

void SceneryCfg::setAreaHighPriority(int index, bool value)
{
  areaEntries[index].setHighPriority(value);
}

void SceneryCfg::onStartSection(const QString& section, const QString& sectionSuffix)
{
  if(section == "area")
  {
    bool ok = false;
    currentArea.setAreaNumber(sectionSuffix.toInt(&ok));
    if(!ok)
    {
      qWarning() << "Area number" << sectionSuffix << "not valid in section" << section;
      currentArea.setAreaNumber(-1);
    }
  }
}

void SceneryCfg::onEndSection(const QString& section, const QString& sectionSuffix)
{
  Q_UNUSED(sectionSuffix);
  if(section == "area")
  {
    currentArea.fixTitle();
    if(!currentArea.getTitle().isEmpty() && currentArea.getAreaNumber() != -1 &&
       (!currentArea.getRemotePath().isEmpty() || !currentArea.getLocalPath().isEmpty()))
      appendArea(currentArea);
    else
      qWarning() << "Found empty area: number" << currentArea.getAreaNumber() << "in section" << section;

    currentArea = SceneryArea();
  }
}

void SceneryCfg::onKeyValue(const QString& section, const QString& sectionSuffix, const QString& key,
                            const QString& value)
{
  Q_UNUSED(sectionSuffix);
  if(section == "general")
  {
    if(key == "title")
      title = key;
    else if(key == "description")
      description = value;
    else if(key == "clean_on_exit")
      cleanOnExit = toBool(value);
    else
      qWarning() << "Unexpected key" << key << "in section" << section << "file" << filepath;
  }
  else if(section == "area")
  {
    if(key == "title")
      currentArea.setTitle(value);
    else if(key == "texture_id")
      currentArea.setTextureId(toInt(value));
    else if(key == "remote")
      currentArea.setRemotePath(value);
    else if(key == "local")
    {
#ifdef Q_OS_UNIX
      currentArea.setLocalPath(QString(value).replace("\\", "/"));
#else
      currentArea.setLocalPath(value);
#endif
    }
    else if(key == "layer")
      currentArea.setLayer(toInt(value));
    else if(key == "active")
      currentArea.setActive(toBool(value));
    else if(key == "required")
      currentArea.setRequired(toBool(value));
    else if(key == "exclude")
      currentArea.setExclude(value);
    else
      qWarning() << "Unexpected key" << key << "in section" << section << "file" << filepath;
  }
  else
    qWarning() << "Unexpected section" << section << "file" << filepath;
}

QDebug operator<<(QDebug out, const SceneryCfg& cfg)
{
  QDebugStateSaver saver(out);

  out.nospace() << "SceneryCfg["
                << "title " << cfg.title
                << ", description " << cfg.description
                << ", cleanOnExit " << cfg.cleanOnExit << endl;

  for(const SceneryArea& area : cfg.areaEntries)
    out.nospace().noquote() << area << endl;

  out.nospace().noquote() << endl << "]";
  return out;
}

} // namespace scenery
} // namespace fs
} // namespace atools
