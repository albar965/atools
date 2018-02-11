/*****************************************************************************
* Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
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
  : IniReader(textCodec), cleanOnExit(false)
{
}

SceneryCfg::~SceneryCfg()
{
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
  areaEntries[index].setHighPriority(true);
}

void SceneryCfg::onStartSection(const QString& section, const QString& sectionSuffix)
{
  if(section == "area")
  {
    bool ok = false;
    currentArea.areaNumber = sectionSuffix.toInt(&ok);
    if(!ok)
    {
      qWarning() << "Area number" << sectionSuffix << "not valid in section" << section;
      currentArea.areaNumber = -1;
    }
  }
}

void SceneryCfg::onEndSection(const QString& section, const QString& sectionSuffix)
{
  Q_UNUSED(sectionSuffix);
  if(section == "area")
  {
    currentArea.fixTitle();
    if(!currentArea.title.isEmpty() && currentArea.areaNumber != -1 &&
       (!currentArea.remotePath.isEmpty() || !currentArea.localPath.isEmpty()))
      areaEntries.append(currentArea);
    else
      qWarning() << "Found empty area: number" << currentArea.areaNumber << "in section" << section;

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
      qWarning() << "Unexpected key" << key << "in section" << section << "file" << filename;
  }
  else if(section == "area")
  {
    if(key == "title")
      currentArea.title = value;
    else if(key == "texture_id")
      currentArea.textureId = toInt(value);
    else if(key == "remote")
      currentArea.remotePath = value;
    else if(key == "local")
    {
#ifdef Q_OS_UNIX
      currentArea.localPath = QString(value).replace("\\", "/");
#else
      currentArea.localPath = value;
#endif
    }
    else if(key == "layer")
      currentArea.layer = toInt(value);
    else if(key == "active")
      currentArea.active = toBool(value);
    else if(key == "required")
      currentArea.required = toBool(value);
    else if(key == "exclude")
      currentArea.exclude = value;
    else
      qWarning() << "Unexpected key" << key << "in section" << section << "file" << filename;
  }
  else
    qWarning() << "Unexpected section" << section << "file" << filename;
}

bool SceneryCfg::toBool(const QString& str)
{
  QString tmp = str.toLower().trimmed();

  if(tmp == "true" || tmp == "t" || tmp == "y" || tmp == "yes" || tmp == "1")
    return true;
  else if(tmp == "false" || tmp == "f" || tmp == "n" || tmp == "no" || tmp == "0")
    return false;

  qWarning() << "Boolean value not valid in scenery area" << currentArea.title;
  return false;
}

int SceneryCfg::toInt(const QString& str)
{
  int retval = 0;
  bool ok = false;
  retval = str.toInt(&ok);
  if(!ok)
    qWarning() << "Int value not valid in scenery area" << currentArea.title;
  return retval;
}

} // namespace scenery
} // namespace fs
} // namespace atools
