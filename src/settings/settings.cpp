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

#include "settings/settings.h"
#include "exception.h"

#include <QDebug>
#include <QSettings>
#include <QApplication>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>

namespace atools {
namespace settings {

Settings *Settings::settingsInstance = nullptr;
QString Settings::overrideOrganisation;

Settings::Settings()
{
  qSettings = new QSettings(QSettings::IniFormat, QSettings::UserScope, orgNameForDirs(), appNameForFiles());

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

void Settings::clearAndShutdown()
{
  qDebug() << Q_FUNC_INFO;

  // Write current settings
  syncSettings();

  // Create a backup
  QFileInfo file = getFilename();
  QString newFile = file.path() + QDir::separator() + file.baseName() +
                    "_" + QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss") + ".ini";

  if(QFile::copy(file.filePath(), newFile))
    qInfo() << "Copied settings from" << file.filePath() << "to" << newFile;
  else
    qWarning() << "Copying from" << file.filePath() << "to" << newFile << "failed";

  // Remove all key/value pairs and continue shutdown
  clearSettings();
  shutdown();
}

void Settings::shutdown()
{
  qDebug() << Q_FUNC_INFO;
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
  return instance().getQSettings()->fileName();
}

QString Settings::getConfigFilename(const QString& extension, const QString& subdir)
{
  QString path;
  if(subdir.isEmpty())
    path = getPath();
  else
    path = getPath() + QDir::separator() + subdir;

  QDir(path).mkpath(path);

  return path + QDir::separator() + appNameForFiles() + extension;
}

QString Settings::getOverloadedLocalPath(const QString& filename)
{
  QString configDirFile = QFileInfo(filename).fileName();

  if(QFileInfo::exists(configDirFile))
    // User placed a copy of the file in the current directory - use the
    // overloaded one
    return configDirFile;
  else if(QFileInfo::exists(filename))
    // No overloading and file exists return the original path
    return filename;
  else
    throw Exception(QString("Settings::getOverloadedPath: cannot resolve path \"%1\"").arg(filename));
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
    throw Exception(QString("Error writing to settings file \"%1\" reason %2").
                    arg(qs->fileName()).arg(qs->status()));
}

void Settings::clearSettings()
{
  QSettings *qs = instance().getQSettings();
  qs->clear();

  if(qs->status() != QSettings::NoError)
    throw Exception(QString("Error clearing settings file \"%1\" reason %2").
                    arg(qs->fileName()).arg(qs->status()));
  syncSettings();
}

bool Settings::contains(const QString& key) const
{
  return qSettings->contains(key);
}

void Settings::remove(const QString& key)
{
  qSettings->remove(key);
}

QStringList Settings::valueStrList(const QString& key, const QStringList& defaultValue) const
{
  if(!contains(key))
    return defaultValue;

  QStringList list = qSettings->value(key, defaultValue).toStringList();

  if(list.isEmpty() || (list.size() == 1 && list.first().isEmpty()))
    return QStringList();
  else
    return list;
}

QString Settings::valueStr(const QString& key, const QString& defaultValue) const
{
  return qSettings->value(key, defaultValue).toString();
}

bool Settings::valueBool(const QString& key, bool defaultValue) const
{
  return qSettings->value(key, defaultValue).toBool();
}

int Settings::valueInt(const QString& key, int defaultValue) const
{
  return qSettings->value(key, defaultValue).toInt();
}

float Settings::valueFloat(const QString& key, float defaultValue) const
{
  return qSettings->value(key, defaultValue).toFloat();
}

double Settings::valueDouble(const QString& key, double defaultValue) const
{
  return qSettings->value(key, defaultValue).toDouble();
}

QVariant Settings::valueVar(const QString& key, QVariant defaultValue) const
{
  return qSettings->value(key, defaultValue);
}

void Settings::setValue(const QString& key, const QStringList& value)
{
  if(value.isEmpty())
    qSettings->setValue(key, QString());
  else
    qSettings->setValue(key, value);
}

void Settings::setValue(const QString& key, const QString& value)
{
  qSettings->setValue(key, value);
}

void Settings::setValue(const QString& key, bool value)
{
  qSettings->setValue(key, value);
}

void Settings::setValue(const QString& key, int value)
{
  qSettings->setValue(key, QString::number(value));
}

void Settings::setValue(const QString& key, float value)
{
  qSettings->setValue(key, QString::number(value, 'f', 10));
}

void Settings::setValue(const QString& key, double value)
{
  qSettings->setValue(key, QString::number(value, 'f', 18));
}

void Settings::setValueVar(const QString& key, const QVariant& value)
{
  qSettings->setValue(key, value);
}

QStringList Settings::childGroups() const
{
  return qSettings->childGroups();
}

QString Settings::getPath()
{
  return QFileInfo(instance().getQSettings()->fileName()).path();
}

QString Settings::orgNameForDirs()
{
  if(overrideOrganisation.isEmpty())
    return QApplication::organizationName().replace(' ', '_');
  else
    return overrideOrganisation.replace(' ', '_');
}

QString Settings::appNameForFiles()
{
  return QApplication::applicationName().replace(' ', '_').toLower();
}

} // namespace atools
} // namespace settings
