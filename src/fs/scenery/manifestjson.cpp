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

#include "fs/scenery/manifestjson.h"

#include "exception.h"
#include "atools.h"

#include <QFile>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace atools {
namespace fs {
namespace scenery {

/*
 *  {
 *  "dependencies": [],
 *  "content_type": "SCENERY",
 *  "title": "Navigraph Navdata Cycle 2010-revision.10",
 *  "manufacturer": "",
 *  "creator": "Navigraph",
 *  "package_version": "0.1.0",
 *  "minimum_game_version": "1.8.3",
 *  "release_notes": {
 *   "neutral": {
 *     "LastUpdate": "",
 *     "OlderHistory": ""
 *   }
 *  }
 *  } */
void ManifestJson::read(const QString& filename)
{
  if(atools::checkFile(filename))
  {
    QFile file(filename);
    if(file.open(QIODevice::ReadOnly))
    {
      QJsonParseError error;
      QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
      if(error.error != QJsonParseError::NoError)
        qWarning() << Q_FUNC_INFO << "Error reading" << filename << error.errorString() << "at offset" << error.offset;

      QJsonObject obj = doc.object();

      contentType = obj.value("content_type").toString();
      title = obj.value("title").toString();
      manufacturer = obj.value("manufacturer").toString();
      creator = obj.value("creator").toString();
      packageVersion = obj.value("package_version").toString();
      minGameVersion = obj.value("minimum_game_version").toString();
      file.close();
      valid = true;
    }
    else
      qWarning() << Q_FUNC_INFO << "Cannot open file" << filename << file.errorString();
  }
}

bool ManifestJson::isScenery() const
{
  // "content_type": "AIRCRAFT"
  // "content_type": "CORE"
  // "content_type": "INSTRUMENTS"
  // "content_type": "LIVERY"
  // "content_type": "MISC"
  // "content_type": "MISSION"
  // "content_type": "SCENERY"

  // Cannot discriminate by values since add-on developers use often wrong types
  // Instead exclude obvious known types
  return contentType.compare("AIRCRAFT", Qt::CaseInsensitive) != 0 &&
         contentType.compare("INSTRUMENTS", Qt::CaseInsensitive) != 0 &&
         contentType.compare("LIVERY", Qt::CaseInsensitive) != 0 &&
         contentType.compare("MISSION", Qt::CaseInsensitive) != 0;
}

void ManifestJson::clear()
{
  contentType.clear();
  title.clear();
  manufacturer.clear();
  creator.clear();
  packageVersion.clear();
  minGameVersion.clear();
  valid = false;
}

} // namespace scenery
} // namespace fs
} // namespace atools
