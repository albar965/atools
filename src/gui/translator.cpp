/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

#include "gui/translator.h"

#include <QDebug>
#include <QFileInfo>
#include <QCoreApplication>
#include <QTranslator>
#include <QLibraryInfo>
#include <QDir>

namespace atools {

namespace gui {

QVector<QTranslator *> Translator::translators;
bool Translator::loaded = false;

void Translator::load(const QString& language)
{
  if(!loaded)
  {
    QFileInfo appFilePath(QCoreApplication::applicationFilePath());
    QString appPath = appFilePath.absolutePath();
    QString appBaseName = appFilePath.baseName();

    // Load application files - will not be loaded if the files does not exist or is empty
    bool loadDefault = loadApp(appBaseName, appPath, language);
    qDebug() << "Translations for" << appPath << language << "found";

    // Load atools translations - not required
    loadApp("atools", appPath, language);

    if(loadDefault)
    {
      // Load the Qt translations only if a language was found for the application to avoid mixed language dialogs
      QString translationsPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
      // First application path
      if(!loadAndInstall("qt", appPath, language))
        // second official translations path
        loadAndInstall("qt", translationsPath, language);

      if(!loadAndInstall("qtbase", appPath, language))
        loadAndInstall("qtbase", translationsPath, language);
    }
    loaded = true;

    qInfo() << "Locale" << QLocale();
  }
  else
    qWarning() << "Translator::load called more than once";
}

bool Translator::loadApp(const QString& appBaseName, const QString& appPath, const QString& language)
{
  // try resources first
  if(!loadAndInstall(appBaseName, ":/" + appBaseName, language))
    // try resources translations second
    if(!loadAndInstall(appBaseName, ":/" + appBaseName + "/translations", language))
      // Last try in executable directory
      if(!loadAndInstall(appBaseName, appPath, language))
      {
        // No translations for this application found - force English to avoid mixed language in dialogs
        return false;
      }
  return true;
}

void Translator::unload()
{
  if(loaded)
  {
    qDeleteAll(translators);
    translators.clear();
    loaded = false;
  }
  else
    qWarning() << "Translator::unload called more than once";
}

bool Translator::loadAndInstall(const QString& name, const QString& dir, const QString& language)
{
  QLocale locale;
  if(language.isEmpty())
    locale = QLocale::system();
  else
    // Override system language for translations only
    locale = QLocale(language);

  QTranslator *t = new QTranslator();
  if(!t->load(locale, name, "_", dir))
    qInfo() << "Qt translation file" << name << "not loaded from dir" << dir << "locale" << locale.name();
  else if(QCoreApplication::instance()->installTranslator(t))
  {
    qDebug() << "Qt translation file" << name << "from dir" << dir << "installed" << "locale" << locale.name();
    translators.append(t);
    return true;
  }
  else
    qDebug() << "Qt translation file" << name << "not installed from dir" << dir << "locale" << locale.name();

  delete t;
  return false;
}

} // namespace gui
} // namespace atools
