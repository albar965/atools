/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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

#include "fs/scenery/addoncfg.h"

#include <fstream>
#include <iostream>
#include <algorithm>

#include <QDebug>

namespace atools {
namespace fs {
namespace scenery {

AddOnCfg::AddOnCfg(const QString& textCodec)
  : AbstractIniReader(textCodec)
{
}

AddOnCfg::~AddOnCfg()
{
}

void AddOnCfg::onStartDocument(const QString& filename)
{
  Q_UNUSED(filename);
  entries.clear();
}

void AddOnCfg::onEndDocument(const QString& filename)
{
  Q_UNUSED(filename);
}

void AddOnCfg::onStartSection(const QString& section, const QString& sectionSuffix)
{
  if(section == "package")
  {
    bool ok = false;
    currentEntry.packageNum = sectionSuffix.toInt(&ok);
    if(!ok)
    {
      qWarning() << "Entry number" << sectionSuffix << "not valid in section" << section;
      currentEntry.packageNum = -1;
    }
  }
}

void AddOnCfg::onEndSection(const QString& section, const QString& sectionSuffix)
{
  Q_UNUSED(sectionSuffix);
  if(section == "package")
  {
    entries.append(currentEntry);
    currentEntry = AddOnCfgEntry();
  }
}

void AddOnCfg::onKeyValue(const QString& section, const QString& sectionSuffix, const QString& key,
                          const QString& value)
{
  Q_UNUSED(sectionSuffix);
  if(section == "package")
  {
    if(key == "title")
      currentEntry.title = value;
    else if(key == "path")
    {
#ifdef Q_OS_UNIX
      currentEntry.path = QString(value).replace("\\", "/");
#else
      currentEntry.path = value;
#endif
    }
    else if(key == "active")
      currentEntry.active = toBool(value);
    else if(key == "required")
      currentEntry.required = toBool(value);
    else
      qWarning() << "Unexpected key" << key << "in section" << section << "file" << filepath;
  }
  else if(section == "discoverypath")
  {
    // Ignore
  }
  else
    qWarning() << "Unexpected section" << section << "file" << filepath;
}

} // namespace scenery
} // namespace fs
} // namespace atools
