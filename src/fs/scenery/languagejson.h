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

#ifndef ATOOLS_LANGUAGEJSON_H
#define ATOOLS_LANGUAGEJSON_H

#include <QHash>
#include <QString>

namespace atools {
namespace sql {
class SqlDatabase;
}

namespace fs {
namespace scenery {

/*
 * Reads MSFS language files like
 * ".../Microsoft.FlightSimulator_8wekyb3d8bbwe/LocalCache/Packages/Official/OneStore/fs-base/en-US.locPak"
 * and creates a map for airport names like "TT:AIRPORTXX.MYNN.name" to the real localized name.
 *
 * The class can only store the translations for one language.
 */
class LanguageJson
{
public:
  /* Read translations for one language from JSON file.
   * Does not clear index before loading. */
  void readFromFile(const QString& filename, const QStringList& keyPrefixes = {});

  /* Read translations for all languages from directory using given file filter.
   * Stores translations for all languages to database.
   * Clears index after reading. */
  void readFromDirToDb(sql::SqlDatabase *db, const QString& dirname, const QString& fileFilter, const QStringList& keyPrefixes = {});

  /* Read translations for the given language from the database into the index.
   * Clears index before reading. */
  void readFromDb(atools::sql::SqlDatabase *db, const QString& languageParam, const QString& keyPrefix = QString());

  /* Write translations for the given language from the index to the database.
   * Clears index before reading. */
  void writeToDb(atools::sql::SqlDatabase *db) const;

  /* Clear in memory index */
  void clear()
  {
    names.clear();
    language.clear();
#ifdef DEBUG_MODEL_SUPPORT
    models.clear();
#endif
  }

  bool isEmpty() const
  {
    return names.isEmpty();
  }

  /* Get localized airport or other name from key found in BGL file like "TT:AIRPORTXX.MYNN.name"
   * Returns key if it does not start with the translation prefix "TT:" or list is empty */
  QString getName(QString key) const
  {
    return valueFromMap(names, key);
  }

#ifdef DEBUG_MODEL_SUPPORT
  /* Aircraft model "C172" from "ATCCOM.AC_MODEL_C172.0.text". */
  QString getModel(QString key) const;

#endif

  /* Get language as read from file or db. E.g. "en-US" */
  const QString& getLanguage() const
  {
    return language;
  }

private:
  void adjustLanguage();

  /* Maps key like "TT:AIRPORTXX.MYNN.name" to text */
  QHash<QString, QString> names;

#ifdef DEBUG_MODEL_SUPPORT
  void addModel(const QString& key, const QString& value);

  /* Maps "ATCCOM.AC_MODEL_C172.0.text" to "C172" */
  QHash<QString, QString> models;
#endif

  /* Loaded language */
  QString language;

  QString valueFromMap(const QHash<QString, QString>& hash, QString key) const;

};

} // namespace scenery
} // namespace fs
} // namespace atools

#endif // ATOOLS_LANGUAGEJSON_H
