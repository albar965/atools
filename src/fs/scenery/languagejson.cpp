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

#include "atools.h"
#include "sql/sqldatabase.h"
#include "sql/sqlquery.h"
#include "sql/sqlutil.h"

#include <QFile>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>


namespace atools {
namespace fs {
namespace scenery {

/* Names that indicate no translation available in MSFS */
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
void LanguageJson::readFromFile(const QString& filename, const QStringList& keyPrefixes)
{
  clear();

  if(atools::checkFile(filename))
  {
    QFile file(filename);
    if(file.open(QIODevice::ReadOnly))
    {
      QJsonParseError error;
      QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
      if(error.error != QJsonParseError::NoError)
        qWarning() << Q_FUNC_INFO << "Error reading" << filename << error.errorString() << "at offset" << error.offset;

      QJsonObject package = doc.object().value("LocalisationPackage").toObject();

      language = package.value("Language").toString();
      adjustLanguage();
      QJsonObject strings = package.value("Strings").toObject();

      for(auto it = strings.begin(); it != strings.end(); ++it)
      {
        QString key = it.key();
        if(keyPrefixes.isEmpty() || atools::strStartsWith(keyPrefixes, key))
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
}

void LanguageJson::readFromDirToDb(sql::SqlDatabase *db, const QString& dirname, const QString& fileFilter,
                                   const QStringList& keyPrefixes)
{
  QDir dir(dirname);
  for(const QFileInfo& file: dir.entryInfoList({fileFilter}, QDir::Files))
  {
    readFromFile(file.filePath(), keyPrefixes);
    writeToDb(db);
  }
  clear();
}

/*
 *  create table translation
 *  (
 *    translation_id integer primary key,
 *    language varchar(50) not null,             -- Language like "en_US" or "de_DE"
 *    key varchar(250) not null collate nocase,  -- Key like "ATCCOM.AC_MODEL_TBM9.0.text"
 *    text varchar(250) not null collate nocase, -- Translated text
 *  );
 */
void LanguageJson::readFromDb(sql::SqlDatabase *db, const QString& languageParam, const QString& keyPrefix)
{
  clear();

  if(atools::sql::SqlUtil(db).hasTableAndRows("translation"))
  {
    language = languageParam;
    adjustLanguage();

    atools::sql::SqlQuery query(db);
    query.prepare("select key, text from translation where language = :lang and key like :key");
    query.bindValue(":lang", language);
    query.bindValue(":key", keyPrefix.isEmpty() ? "%" : keyPrefix);
    query.exec();
    while(query.next())
      names.insert(query.valueStr(0), query.valueStr(1));
  }
  else
    qWarning() << Q_FUNC_INFO << "Table translation not found in database or empty";
}

void LanguageJson::writeToDb(sql::SqlDatabase *db) const
{
  atools::sql::SqlQuery query(db);
  query.prepare("insert into translation (language, key, text) values(?, ?, ?)");
  for(auto it = names.begin(); it != names.end(); ++it)
  {
    query.bindValue(0, language);
    query.bindValue(1, it.key());
    query.bindValue(2, it.value());
    query.exec();
  }
  db->commit();
}

void LanguageJson::adjustLanguage()
{
  if(language.isEmpty())
    language = "en-US";

  language.replace('_', '-');
  language = language.section('-', 0, 0) + "-" + language.section('-', 1, 1).toUpper();
}

QString LanguageJson::getName(QString key) const
{
  key = key.trimmed();
  QString value;
  if(key.startsWith("TT:"))
  {
    // Translated string
    QString k = key.mid(3);
    value = names.value(k);
    if(value.isEmpty() && k.endsWith(".text"))
    {
      // Nothing found - try tts suffix again
      k.chop(4);
      value = names.value(k + "tts");
    }
    // Otherweise return empty
  }
  else if(key.startsWith("$$:"))
    value = key.mid(3);
  else
    value = key;

  return value;
}

} // namespace scenery
} // namespace fs
} // namespace atools
