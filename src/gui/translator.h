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

#ifndef ATOOLS_GUI_TRANSLATOR_H
#define ATOOLS_GUI_TRANSLATOR_H

#include <QVector>
#include <QString>

class QTranslator;

namespace atools {
namespace gui {

/*
 * Provides two methods to load and unload system and application translations
 * from various places.
 */
class Translator
{
public:
  /*
   * Will try to load application and system translation files.
   * Search order for application files is:
   * * Resources: ":/APPBASENAME/" where APPBASENAME is the base name of
   *   the executable.
   * * Resources: ":/translations/APPBASENAME/"
   * * File system: "APPDIRECTORY/" where APPDIRECTORY is the
   *   directory containing the app executable.
   *
   * Will not change the application locale if no translation files were found.
   *
   * Search order for system files is:
   * * "APPDIRECTORY/"
   * * QLibraryInfo::TranslationsPath
   *
   * @param language If given will override the system language.
   * Format is "language[_script][_country][.codeset][@modifier]".
   */
  static void load(const QString& language = QString());

  /* Unloads all translations and frees associated resources */
  static void unload();

private:
  Translator()
  {

  }

  static QVector<QTranslator *> translators;
  static bool loadAndInstall(const QString& name, const QString& dir, const QString& language);

  static bool loaded;

};

} // namespace gui
} // namespace atools

#endif // ATOOLS_GUI_TRANSLATOR_H
