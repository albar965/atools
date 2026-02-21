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

#include "settings/settings.h"
#include "atools.h"

#include <QSettings>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QCoreApplication>

namespace atools {
namespace settings {

Settings *Settings::settingsInstance = nullptr;
QString Settings::overridePath;
QString Settings::organizationName;
QString Settings::applicationName;
QStringList Settings::infoMessages;
QStringList Settings::errorMessages;

Settings::Settings()
{
  // Create dirs relative to app dir or based on absolute path if set
  createOverridePath();

  if(!overridePath.isEmpty())
    // qSettings object is used to determine paths
    qSettings = new QSettings(overridePath + QDir::separator() + appNameForFiles() + ".ini", QSettings::IniFormat);
  else
    // Default settings path in roaming or other well known paths
    qSettings = new QSettings(QSettings::IniFormat, QSettings::UserScope, orgNameForDirs(), appNameForFiles());

  QString path = QFileInfo(qSettings->fileName()).path();
  if(!QFileInfo::exists(path))
  {
    // Create directory so getConfigFilename() does not fail
    if(QDir().mkpath(path))
      infoMessages.append(QStringLiteral("Created settings path \"%1\"").arg(path));
    else
      errorMessages.append(QStringLiteral("Cannot create settings path \"%1\"").arg(path));
  }

  if(qSettings->status() != QSettings::NoError)
    errorMessages.append(QStringLiteral("Error creating settings file \"%1\" reason %2").arg(qSettings->fileName()).arg(
                           qSettings->status()));

  if(!qSettings->isWritable())
    errorMessages.append(QStringLiteral("Settings file \"%1\" not writeable").arg(qSettings->fileName()));

  infoMessages.append(QStringLiteral("Using settings file \"%1\"").arg(qSettings->fileName()));
}

Settings::~Settings()
{
  delete qSettings;
}

void Settings::createOverridePath()
{
  if(!overridePath.isEmpty())
  {
    // Override full path
    QDir dir(overridePath);
    if(dir.isAbsolute())
    {
      // Create all paths for absolute
      if(!QFile::exists(overridePath))
      {
        if(QDir().mkpath(overridePath))
          infoMessages.append(QStringLiteral("Created absolute settings path \"%1\"").arg(overridePath));
        else
          errorMessages.append(QStringLiteral("Cannot create settings path \"%1\"").arg(overridePath));
      }
    }
    else
    {
      // Create path based on relative path to application
      QDir appDir(QCoreApplication::applicationDirPath());
      if(!appDir.exists(overridePath))
      {
        if(appDir.mkpath(overridePath))
          infoMessages.append(QStringLiteral("Created relative settings path \"%1\"").arg(overridePath));
        else
          errorMessages.append(QStringLiteral("Cannot create settings path \"%1\"").arg(overridePath));
      }
    }
  }
}

Settings& Settings::instance()
{
  if(settingsInstance == nullptr)
  {
    if(applicationName.isEmpty())
      applicationName = QCoreApplication::applicationName();

    if(organizationName.isEmpty())
      organizationName = QCoreApplication::organizationName();

    settingsInstance = new Settings();
  }

  return *settingsInstance;
}

void Settings::logMessages()
{
  for(const QString& message : std::as_const(infoMessages))
    qInfo().noquote() << Q_FUNC_INFO << message;

  for(const QString& message : std::as_const(errorMessages))
    qCritical().noquote() << Q_FUNC_INFO << message;

  infoMessages.clear();
  errorMessages.clear();
}

void Settings::clearAndShutdown()
{
  qDebug() << Q_FUNC_INFO;

  // Write current settings
  syncSettings();

  // Create a backup
  QFileInfo file(getFilename());
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

QString Settings::getFilename()
{
  return getQSettings()->fileName();
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

QString Settings::getOverloadedLocalPath(const QString& filename, bool ignoreMissing)
{
  QString configDirFile = QFileInfo(filename).fileName();

  if(QFileInfo::exists(configDirFile))
    // User placed a copy of the file in the current directory - use the
    // overloaded one
    return configDirFile;
  else if(QFileInfo::exists(filename))
    // No overloading and file exists return the original path
    return filename;
  else if(!ignoreMissing)
    errorMessages.append(QStringLiteral("Settings::getOverloadedPath: cannot resolve path \"%1\"").arg(filename));

  return QString();
}

QString Settings::getOverloadedPath(const QString& filename, bool ignoreMissing)
{
  QString configDirFile = getPath() + QDir::separator() + QFileInfo(filename).fileName();

  if(QFileInfo::exists(configDirFile))
    // User placed a copy of the file in the configuration directory - use the
    // overloaded one
    return configDirFile;
  else if(QFileInfo::exists(filename))
    // No overloading and file exists return the original path
    return filename;
  else if(!ignoreMissing)
    errorMessages.append(QStringLiteral("Settings::getOverloadedPath: cannot resolve path \"%1\"").arg(filename));

  return QString();
}

QVariant Settings::getAndStoreValue(const QString& key, const QVariant& defaultValue) const
{
  QSettings *settings = getQSettings();

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
  QSettings *qs = getQSettings();
  qs->sync();

  if(qs->status() != QSettings::NoError)
    errorMessages.append(QStringLiteral("Error writing to settings file \"%1\" reason %2").arg(qs->fileName()).arg(qs->status()));
}

void Settings::clearSettings()
{
  QSettings *qs = getQSettings();
  qs->clear();

  if(qs->status() != QSettings::NoError)
    errorMessages.append(QStringLiteral("Error clearing settings file \"%1\" reason %2").arg(qs->fileName()).arg(qs->status()));
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

  if(list.isEmpty() || (list.size() == 1 && list.constFirst().isEmpty()))
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

int Settings::valueLongLong(const QString& key, long long defaultValue) const
{
  return qSettings->value(key, defaultValue).toLongLong();
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

void Settings::setValue(const QString& key, long long value)
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
  return QFileInfo(getQSettings()->fileName()).path();
}

QString Settings::getDirName()
{
  return QFileInfo(getPath()).fileName();
}

QString Settings::orgNameForDirs()
{
  return organizationName.replace(' ', '_');
}

QString Settings::appNameForFiles()
{
  return applicationName.replace(' ', '_').toLower();
}

} // namespace atools
} // namespace settings
