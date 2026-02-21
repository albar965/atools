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
 * QCoreApplication::applicationName() in lowercase with
 * spaces replaced by underscrores.
 * If an error occurs sets error messsages.
 */
class Settings
{
public:
  Settings(const Settings& other) = delete;
  Settings& operator=(const Settings& other) = delete;

  /* Get the singleton instance. The operator-> allows direct access to the
   * QSettings object. Uses organization and application name from QCoreApplication if not set before. */
  static Settings& instance();

  /* Log all collected messages from instance() delayed and clear lists. Call this after initalizing the logging system. */
  static void logMessages();

  /* true if settings is not useable */
  static bool hasErrors()
  {
    return !errorMessages.isEmpty();
  }

  /* Flush settings and release all resources */
  static void shutdown();

  /* Clear all values and shutdown */
  static void clearAndShutdown();

  /* Overrides the whole path if not empty */
  static void setOverridePath(const QString& value)
  {
    overridePath = value;
  }

  /* Set this if instance() has to be used before creating an application object */
  static void setOrganizationName(const QString& value)
  {
    organizationName = value;
  }

  /* Set this if instance() has to be used before creating an application object */
  static void setApplicationName(const QString& value)
  {
    applicationName = value;
  }

  /*
   * Get an application specific filename plus extension. The file will be
   * stored in the organization settings directory and have a name based on
   * the application name.
   *
   * @param extension Extension for the file including the dot.
   */
  static QString getConfigFilename(const QString& extension, const QString& subdir = QString());

  /* Get the organization specific settings directory.
   * E.g. "C:\Users\YOURUSERNAME\AppData\Roaming\ABarthel" */
  static QString getPath();

  /* Get the organization specific settings directory.
   * E.g. "C:\Users\YOURUSERNAME\AppData\Roaming\ABarthel\little_navmap.ini" */
  static QString getFilename();

  /* Get the organization specific settings directory only last path part.
   * E.g. "ABarthel" for "C:\Users\YOURUSERNAME\AppData\Roaming\ABarthel" */
  static QString getDirName();

  /* Returns the filename of the given path if the file exists in the settings
   * directory or the given path if the file exists. Otherwise throws Exception.
   *
   * @param filename
   * @param ignoreMissing Throw an exception if false and file not found anywhere. Otherwise return empty string.
   * @return Path to existing file from settings or original filename.
   */
  static QString getOverloadedPath(const QString& filename, bool ignoreMissing = false);

  /* Returns the filename of the given path if the file exists in the current
   * directory or the given path if the file exists. Otherwise throws Exception.
   *
   * @param filename
   * @param ignoreMissing Throw an exception if false and file not found anywhere. Otherwise return empty string.
   * @return Path to existing file from settings or original filename.
   */
  static QString getOverloadedLocalPath(const QString& filename, bool ignoreMissing = false);

  /* The same as QSettings.value() but ensures that the default is stored in the file
   * if not already present. Call syncSettings afterwards to write all defaults to the file. */
  QVariant getAndStoreValue(const QString& key, const QVariant& defaultValue = QVariant()) const;

  /* Write settings to file and reload all changes in settings file */
  static void syncSettings();

  /* Remove all key/value pairs */
  static void clearSettings();

  /*
   * @return The single QSettings object.
   */
  static QSettings *getQSettings()
  {
    return instance().qSettings;
  }

  bool contains(const QString& key) const;
  void remove(const QString& key);

  QStringList valueStrList(const QString& key, const QStringList& defaultValue = QStringList()) const;
  QString valueStr(const QString& key, const QString& defaultValue = QString()) const;
  bool valueBool(const QString& key, bool defaultValue = false) const;
  int valueInt(const QString& key, int defaultValue = 0) const;
  int valueLongLong(const QString& key, long long defaultValue = 0LL) const;
  float valueFloat(const QString& key, float defaultValue = 0.f) const;
  double valueDouble(const QString& key, double defaultValue = 0.) const;
  QVariant valueVar(const QString& key, QVariant defaultValue = QVariant()) const;

  template<typename TYPE>
  TYPE valueEnum(const QString& key, TYPE defaultValue = TYPE()) const
  {
    /* Print error message at compile time if value is no enum */
    static_assert(std::is_enum<TYPE>(), "std::is_enum<TYPE>()");
    static_assert(std::is_convertible<TYPE, long long>(), "std::is_convertible<TYPE, long long>()");

    return static_cast<TYPE>(valueLongLong(key, static_cast<long long>(defaultValue)));
  }

  template<typename TYPE>
  void setValueEnum(const QString& key, TYPE value)
  {
    /* Print error message at compile time if value is no enum */
    static_assert(std::is_enum<TYPE>(), "std::is_enum<TYPE>()");
    static_assert(std::is_convertible<TYPE, long long>(), "std::is_convertible<TYPE, long long>()");

    setValue(key, static_cast<long long>(value));
  }

  void setValue(const QString& key, const QStringList& value);
  void setValue(const QString& key, const QString& value);
  void setValue(const QString& key, bool value);
  void setValue(const QString& key, int value);
  void setValue(const QString& key, long long value);
  void setValue(const QString& key, float value);
  void setValue(const QString& key, double value);
  void setValueVar(const QString& key, const QVariant& value);

  QStringList childGroups() const;

private:
  Settings();
  ~Settings();

  QSettings *qSettings;

  /* Create dirs relative to app dir or based on absolute path */
  static void createOverridePath();

  static QString overridePath, organizationName, applicationName;
  static QStringList infoMessages, errorMessages;
  static QString appNameForFiles();

  static Settings *settingsInstance;
  static QString orgNameForDirs();

};

} // namespace atools
} // namespace settings

#endif // ATOOLS_SETTINGS_SETTINGS_H
