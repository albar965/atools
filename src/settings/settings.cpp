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

#include "settings/settings.h"
#include "exception.h"
#include "logging/loggingdefs.h"

#include <QSettings>
#include <QApplication>
#include <QFileInfo>
#include <QDir>

namespace atools {
namespace settings {

Settings *Settings::settingsInstance = nullptr;

Settings::Settings()
{
  qSettings = new QSettings(QSettings::IniFormat, QSettings::UserScope,
                            orgNameForDirs(),
                            appNameForFiles());

  QString p = QFileInfo(qSettings->fileName()).path();
  if(!QFileInfo::exists(p))
    // Create directory so getConfigFilename() does not fail
    if(!QDir().mkpath(p))
      throw Exception(QString("Cannot create settings path \"%1\"").arg(p));

  if(qSettings->status() != QSettings::NoError)
    throw Exception(QString("Error creating settings file \"%1\" reason %2").
                    arg(qSettings->fileName()).arg(qSettings->status()));
}

Settings::~Settings()
{
  delete qSettings;
}

Settings& Settings::instance()
{
  if(settingsInstance == nullptr)
    settingsInstance = new Settings();

  return *settingsInstance;
}

void Settings::shutdown()
{
  if(settingsInstance != nullptr)
  {
    delete settingsInstance;
    settingsInstance = nullptr;
  }
  else
    qWarning() << "Settings::shutdown called more than once";
}

void Settings::logSettingsInformation()
{
  qInfo() << "Settings path" << getPath() << "filename" << getFilename();
}

QString Settings::getFilename()
{
  return instance()->fileName();
}

QString Settings::getConfigFilename(const QString& extension)
{
  return getPath() + QDir::separator() + appNameForFiles() + extension;
}

QString Settings::getOverloadedPath(const QString& filename)
{
  QString configDirFile = getPath() + QDir::separator() + QFileInfo(filename).fileName();

  if(QFileInfo::exists(configDirFile))
    // User placed a copy of the file in the configuration directory - use the
    // overloaded one
    return configDirFile;
  else if(QFileInfo::exists(filename))
    // No overloading and file exists return the original path
    return filename;
  else
    throw Exception(QString("Settings::getOverloadedPath: cannot resolve path \"%1\"").arg(filename));
}

QVariant Settings::getAndStoreValue(const QString& key, const QVariant& defaultValue) const
{
  QSettings *settings = instance().getQSettings();

  if(settings->contains(key))
    return settings->value(key, defaultValue);
  else
  {
    settings->setValue(key, defaultValue);
    return defaultValue;
  }
}

void Settings::syncSettings()
{
  QSettings *qs = instance().getQSettings();
  qs->sync();

  if(qs->status() != QSettings::NoError)
    throw Exception(QString("Error creating settings file \"%1\" reason %2").
                    arg(qs->fileName()).arg(qs->status()));
}

QString Settings::getPath()
{
  return QFileInfo(instance()->fileName()).path();
}

QString Settings::orgNameForDirs()
{
  return QApplication::organizationName().replace(' ', '_');
}

QString Settings::appNameForFiles()
{
  return QApplication::applicationName().replace(' ', '_').toLower();
}

} // namespace atools
} // namespace settings
