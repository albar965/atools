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

#include "fs/scenery/layoutjson.h"

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
 *  "content": [
 *   {
 *     "path": "scenery/0000/ATX00000.bgl",
 *     "size": 15516,
 *     "date": 132408483334580637
 *   },
 *  ...
 *   {
 *     "path": "scenery/world/BoundariesTraining.bgl",
 *     "size": 365420,
 *     "date": 132408485372037503
 *   },
 *   {
 *     "path": "scenery/world/BoundariesWarning.bgl",
 *     "size": 122536,
 *     "date": 132408485375667376
 *   }
 *  ]
 *  }
 */
void LayoutJson::read(const QString& filename)
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

      QJsonArray arr = doc.object().value("content").toArray();
      for(int i = 0; i < arr.count(); i++)
      {
        QString path = arr.at(i).toObject().value("path").toString();
        if(path.endsWith(".bgl", Qt::CaseInsensitive))
          bglPaths.append(path);
        else if(path.endsWith("Library.xml", Qt::CaseInsensitive))
          materialPaths.append(path);
      }
      file.close();
    }
    else
      qWarning() << Q_FUNC_INFO << "Cannot open file" << filename << file.errorString();
  }
}

} // namespace scenery
} // namespace fs
} // namespace atools
