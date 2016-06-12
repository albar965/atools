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

#ifndef ATOOLS_SETTINGS_SETTINGS_H
#define ATOOLS_SETTINGS_SETTINGS_H

#include <QString>
#include <QVariant>

class QSettings;

namespace atools {
namespace settings {

/*
 * Provides access to QSettings in a singleton as well as access to the
 * settings directory. The settings are always stored in a file ini format.
 * Directory will be QApplication::organizationName() with
 * spaces replaced by underscrores. Filename will be
 * QApplication::applicationName() in lowercase with
 * spaces replaced by underscrores.
 * If an error occurs Exception is thrown.
 */
class Settings
{
public:
  /* Get the singleton instance. The operator-> allows direct access to the
   * QSettings object. */
  static Settings& instance();

  /* Flush settings and release all resources */
  static void shutdown();

  /* Log relevant settings information into the qInfo channel.
   * That is currently the filename and the settings directory. */
  static void logSettingsInformation();

  /*
   * Get an application specific filename plus extension. The file will be
   * stored in the organization settings directory and have a name based on
   * the application name.
   *
   * @param extension Extension for the file including the dot.
   */
  static QString getConfigFilename(const QString& extension, const QString& subdir = QString());

  /* Get the organization specific settings directory. */
  static QString getPath();

  /* Returns the filename of the given path if the file exists in the settings
   * directory or the given path if the file exists. Otherwise throws Exception.
   *
   * @param filename
   * @return Path to existing file from settings or original filename.
   */
  static QString getOverloadedPath(const QString& filename);

  /* Returns the filename of the given path if the file exists in the current
   * directory or the given path if the file exists. Otherwise throws Exception.
   *
   * @param filename
   * @return Path to existing file from settings or original filename.
   */
  static QString getOverloadedLocalPath(const QString& filename);

  /* The same as QSettings.value() but ensures that the default is stored in the file
   * if not already present. Call syncSettings afterwards to write all defaults to the file. */
  QVariant getAndStoreValue(const QString& key, const QVariant& defaultValue = QVariant()) const;

  /* Write settings to file and reload all changes in settings file */
  static void syncSettings();

  /*
   * @return The single QSettings object.
   */
  static QSettings *getQSettings()
  {
    return instance().qSettings;
  }

  /*
   * Gives access to the included QSettings object.
   *
   * @return QSettings pointer
   */
  QSettings *operator->()
  {
    return qSettings;
  }

private:
  Settings();
  ~Settings();

  QSettings *qSettings;

  static QString getFilename();

  static Settings *settingsInstance;
  static QString appNameForFiles();
  static QString orgNameForDirs();

};

} // namespace atools
} // namespace settings

#endif // ATOOLS_SETTINGS_SETTINGS_H
