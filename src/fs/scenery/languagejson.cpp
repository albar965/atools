/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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
  if(atools::checkFile(Q_FUNC_INFO, filename))
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

      for(auto it = strings.constBegin(); it != strings.constEnd(); ++it)
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
    {
      names.insert(query.valueStr(0), query.valueStr(1));
#ifdef DEBUG_MODEL_SUPPORT
      addModel(query.valueStr(0), query.valueStr(1));
#endif
    }
  }
  else
    qWarning() << Q_FUNC_INFO << "Table translation not found in database or empty";
}

void LanguageJson::writeToDb(sql::SqlDatabase *db) const
{
  atools::sql::SqlQuery query(db);
  query.prepare("insert into translation (language, key, text) values(?, ?, ?)");
  for(auto it = names.constBegin(); it != names.constEnd(); ++it)
  {
    QString key = it.key();

    query.bindValue(0, language);
    query.bindValue(1, key);
    query.bindValue(2, it.value());
    query.exec();

    // Key change in SU12 - add duplicate entries in old and new format
    if(key.startsWith("ATCCOM.ATC_NAME "))
    {
      query.bindValue(1, key.replace("ATCCOM.ATC_NAME ", "ATCCOM.ATC_NAME_"));
      query.exec();
    }
    else if(key.startsWith("ATCCOM.AC_MODEL "))
    {
      query.bindValue(1, key.replace("ATCCOM.AC_MODEL ", "ATCCOM.AC_MODEL_"));
      query.exec();
    }

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

#ifdef DEBUG_MODEL_SUPPORT
QString LanguageJson::getModel(QString key) const
{
  return valueFromMap(models, key);
}

#endif

QString LanguageJson::valueFromMap(const QHash<QString, QString>& hash, QString key) const
{
  key = key.trimmed();
  QString value;
  if(key.startsWith("TT:"))
  {
    // Translated string
    key.remove(0, 3);
    value = hash.value(key);
    if(value.isEmpty() && key.endsWith(".text"))
      // Nothing found - try tts suffix again
      value = hash.value(key.chopped(4) + "tts");
    // else return empty
  }
  else if(key.startsWith("$$:"))
    value = key.mid(3);
  else if(key.startsWith("AIRPORT"))
    // Return empty if airport name or admin names not found
    value = hash.value(key);
  else if(key.startsWith("ATCCOM."))
    // Return empty if aircraft names not found
    value = hash.value(key);
  else
  {
    value = hash.value(key);
    if(value.isEmpty() && key.endsWith(".text"))
      // Nothing found - try tts suffix again
      value = hash.value(key.chopped(4) + "tts");
    if(value.isEmpty())
      value = key;
  }

  return value;
}

#ifdef DEBUG_MODEL_SUPPORT

void LanguageJson::addModel(const QString& key, const QString& value)
{
  const static QRegularExpression MODEL_REGEXP("^ATCCOM.AC_MODEL[_ ]?([A-Z0-9]+)\\.(text|tts)?");

  // "ATCCOM.AC_MODEL_C172.0.text" to "C172"
  // "ATCCOM.AC_MODEL BE58.0.text": "Baron",
  // "ATCCOM.AC_MODEL_BE58.0.text": "BE58",
  // "ATCCOM.AC_MODEL BE58.0.tts": "Baron",
  // "ATCCOM.AC_MODEL_BE58.0.tts": "BE58",

  if(key.startsWith("ATCCOM.AC_MODEL"))
  {
    QRegularExpressionMatch match = MODEL_REGEXP.match(key);
    if(match.hasMatch())
      models.insert(key, match.captured(1));
  }
}

#endif

} // namespace scenery
} // namespace fs
} // namespace atools
