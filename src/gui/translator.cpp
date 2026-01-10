/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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
#include "atools.h"

#include <QDebug>
#include <QFileInfo>
#include <QCoreApplication>
#include <QTranslator>
#include <QLibraryInfo>
#include <QDir>
#include <QLocale>
#include <QCoreApplication>
#include <QRegularExpression>

namespace atools {

namespace gui {

QList<QTranslator *> Translator::translators;
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
    qDebug() << "Translations for appPath" << appPath << "lang" << language << "loadDefault" << loadDefault;

    if(loadDefault)
    {
      // Load atools translations if main app language was found
      loadApp("atools", appPath, language);

      // Load the Qt translations only if a language was found for the application to avoid mixed language dialogs
      QString translationsPath = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
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
  {
    // try resources translations second
    if(!loadAndInstall(appBaseName, ":/" + appBaseName + "/translations", language))
    {
      // Executable directory
      if(!loadAndInstall(appBaseName, appPath, language))
      {
        // Last try in executable directory + "translations"
        if(!loadAndInstall(appBaseName, appPath + QDir::separator() + "translations", language))
        {
          // No translations for this application found - force English to avoid mixed language in dialogs
          return false;
        }
      }
    }
  }
  return true;
}

void Translator::unload()
{
  if(loaded)
  {
    for(QTranslator *trans : std::as_const(translators))
      QCoreApplication::removeTranslator(trans);

    qDeleteAll(translators);
    translators.clear();
    loaded = false;
  }
  else
    qWarning() << "Translator::unload called more than once";
}

QList<QLocale> Translator::findTranslationFiles()
{
  // Get files from current application path (macOS)
  QList<QLocale> retval = findTranslationFilesInternal(QString());

  // Get files from translations folder (Linux and Windows)
  retval.append(findTranslationFilesInternal("translations"));

  // Always add English
  retval.append(QLocale("en"));

  // Sort by language and country ==================
  std::sort(retval.begin(), retval.end(), [](const QLocale& l1, const QLocale& l2) -> bool {
        if(l1.language() != l2.language())
          return l1.language() < l2.language();
        else
          return l1.country() < l2.country();
      });
  // Remove consecutive duplicates in the sorted list ==================
  retval.erase(std::unique(retval.begin(), retval.end(), [](const QLocale& l1, const QLocale& l2) -> bool {
        return l1.language() == l2.language() && l1.country() == l2.country();
      }), retval.end());

  // Now sort list by native language and country name ==============
  std::sort(retval.begin(), retval.end(), [](const QLocale& l1, const QLocale& l2) -> bool {
        int cmp = l1.nativeLanguageName().compare(l2.nativeLanguageName());
        if(cmp == 0)
          return l1.nativeCountryName().compare(l2.nativeCountryName()) < 0;
        else
          return cmp < 0;
      });
  return retval;
}

QList<QLocale> Translator::findTranslationFilesInternal(const QString& path)
{
  static const QString APP_NAME = QFileInfo(QCoreApplication::applicationFilePath()).baseName();
  static const QString FILTER = QStringLiteral("%1_*.qm").arg(APP_NAME);
  static const QRegularExpression QM_FILE_LANG(APP_NAME + "_(.+).qm");
  static const QRegularExpression QM_FILE_LANG_REGION(APP_NAME + "_(.+)_(.+).qm");

  // Setup directory for file detection =====================================
  QDir dir(QCoreApplication::applicationDirPath() + QDir::separator() + path);
  dir.setFilter(QDir::Files | QDir::Hidden);
  dir.setNameFilters({FILTER});

  QList<QLocale> retval;
  for(const QFileInfo& fi : dir.entryInfoList())
  {
    atools::checkFile(Q_FUNC_INFO, fi, true /* warn */);

    // Use C locale as invalid
    QLocale locale(QLocale::C);

    // Try language/region match ========
    QRegularExpressionMatch match = QM_FILE_LANG_REGION.match(fi.fileName());
    if(match.hasMatch())
      locale = QLocale(match.captured(1) + "_" + match.captured(2));
    else
    {
      // Try language match ========
      QRegularExpressionMatch match2 = QM_FILE_LANG.match(fi.fileName());
      if(match2.hasMatch())
        locale = QLocale(match2.captured(1));
    }

    if(locale.language() != QLocale::C)
    {
      qInfo() << Q_FUNC_INFO << fi.filePath() << "name" << locale.name() << "bcp47Name" << locale.bcp47Name()
              << "country" << QLocale::countryToString(locale.country()) << "native" << locale.nativeCountryName()
              << "language" << QLocale::languageToString(locale.language()) << "native" << locale.nativeLanguageName();
      retval.append(locale);
    }
    else
      qWarning() << Q_FUNC_INFO << "No locale found for" << fi.filePath();
  }
  return retval;
}

bool Translator::loadAndInstall(const QString& name, const QString& dir, const QString& language)
{
  QLocale locale;
  if(language.isEmpty())
    // Use only one language here since the translation API will try to load second and third languages
    locale = QLocale(QLocale().uiLanguages().isEmpty() ? "en" : QLocale().uiLanguages().value(0));
  else
    // Override system language for translations only
    locale = QLocale(language);

  QTranslator *t = new QTranslator();
  if(!t->load(locale, name, "_", dir))
    qDebug() << "Qt translation file" << name << "not loaded from dir" << dir << "locale" << locale.name();
  else if(QCoreApplication::installTranslator(t))
  {
    qInfo() << "Qt translation file" << name << "from dir" << dir << "installed" << "locale" << locale.name();
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
