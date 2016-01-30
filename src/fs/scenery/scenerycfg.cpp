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

#include "fs/scenery/scenerycfg.h"
#include "fs/scenery/sceneryarea.h"

#include <fstream>
#include <iostream>
#include <algorithm>

namespace atools {
namespace fs {
namespace scenery {

class SceneryAreaComparator
{
public:
  bool operator()(const SceneryArea& a1, const SceneryArea& a2)
  {
    return a1.getLayer() < a2.getLayer();
  }

};

SceneryCfg::SceneryCfg()
  : IniReader(), cleanOnExit(false)
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
  std::sort(areaEntries.begin(), areaEntries.end(), SceneryAreaComparator());
}

void SceneryCfg::onStartSection(const QString& section, const QString& sectionSuffix)
{
  if(section == "area")
  {
    bool ok = false;
    currentArea.areaNumber = sectionSuffix.toInt(&ok);
    if(!ok)
      throwException("Area number not valid");
  }
}

void SceneryCfg::onEndSection(const QString& section, const QString& sectionSuffix)
{
  Q_UNUSED(sectionSuffix);
  if(section == "area")
    areaEntries.push_back(currentArea);
}

void SceneryCfg::onKeyValue(const QString& section,
                            const QString& sectionSuffix,
                            const QString& key,
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
      throwException("Unexpected key \"" + key + "\"");
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
      throwException("Unexpected key \"" + key + "\"");
  }
}

bool SceneryCfg::toBool(const QString& str)
{
  QString tmp = str.toLower();

  if(tmp == "true" || tmp == "t" || tmp == "y" || tmp == "yes")
    return true;
  else if(tmp == "false" || tmp == "f" || tmp == "n" || tmp == "no")
    return false;

  throwException("Boolean value not valid");
}

int SceneryCfg::toInt(const QString& str)
{
  int retval = 0;
  bool ok = false;
  retval = str.toInt(&ok);
  if(!ok)
    throwException("Int value not valid");
  return retval;
}

} // namespace scenery
} // namespace fs
} // namespace atools
