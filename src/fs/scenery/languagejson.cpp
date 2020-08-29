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

#include "fs/scenery/languagejson.h"

#include <QFile>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace atools {
namespace fs {
namespace scenery {

const static QSet<QString> NO_NAMES = {
  "KEINE STADT", "KEIN BUNDESSTAAT", "NO CITY", "NO STATE", "SIN CIUDAD", "SIN ESTADO", "AUCUNE VILLE", "AUCUN ÉTAT",
  "NESSUNA CITTÀ", "NESSUNO STATO", "BRAK MIEJSCOWOŚCI", "BRAK STANU", "SEM CIDADE", "SEM ESTADO"};

/*
 *  {
 *  "LocalisationPackage": {
 *   "Language": "de-DE",
 *   "Strings": {
 *     ...
 *     "AIRPORT0D.00AK.city": "Anchor Point",
 *     "AIRPORT0D.00AK.name": "Lowell Field",
 *     "AIRPORT0D.00AK.state": "KEIN BUNDESSTAAT",
 *     ...
 *   }
 *  }
 *  }
 */
LanguageJson::LanguageJson(const QString& filename)
{
  QFile file(filename);
  if(file.open(QIODevice::ReadOnly))
  {
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);

    QJsonObject strings = doc.object().value("LocalisationPackage").toObject().value("Strings").toObject();

    for(auto it = strings.begin(); it != strings.end(); ++it)
    {
      QString key = it.key();
      if(key.startsWith("AIRPORT"))
      {
        QString txt = it.value().toString();

        if(!NO_NAMES.contains(txt))
          names.insert(key, txt);
      }
    }
    file.close();
  }
  else
    qWarning() << Q_FUNC_INFO << "Cannot open file" << filename << file.errorString();
}

QString LanguageJson::getName(QString key) const
{
  return names.value(key.startsWith("TT:") ? key.remove(0, 3) : key);
}

} // namespace scenery
} // namespace fs
} // namespace atools
