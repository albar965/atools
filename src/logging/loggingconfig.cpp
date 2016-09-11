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

#include "logging/loggingconfig.h"
#include "settings/settings.h"
#include "io/fileroller.h"

#include <QDebug>
#include <QDir>
#include <QSettings>
#include <QApplication>

namespace atools {
namespace logging {
namespace internal {

LoggingConfig::LoggingConfig(const QString& logConfiguration, const QString& logDirectory,
                             const QString& logFilePrefix)
  : logConfig(logConfiguration), logDir(logDirectory), logPrefix(logFilePrefix)
{
  QSettings *settings = nullptr;

  if(!logConfig.isEmpty())
  {
    QFileInfo logFile(logConfig);

    QString logFileInConfig = atools::settings::Settings::getPath() + QDir::separator() + logFile.fileName();

    if(QFile::exists(logFileInConfig))
      // Try in the configuration directory
      settings = new QSettings(logFileInConfig, QSettings::IniFormat);
    else if(QFile::exists(logConfig))
      // Try resource or full path
      settings = new QSettings(logConfig, QSettings::IniFormat);
  }
  else
    // Use default configuration file
    settings = atools::settings::Settings::getQSettings();

  if(settings->status() != QSettings::NoError)
    qWarning() << "Error reading log configuration file" << settings->fileName() << ":" << settings->status();

  // Read general parameters
  readConfigurationSection(settings);

  QHash<QString, QTextStream *> channelMap;
  // Create all file streams and add them to the channelMap
  readChannels(settings, channelMap);

  // Assign log levels to channels
  readLevels(settings, channelMap);

  delete settings;
}

LoggingConfig::~LoggingConfig()
{
  debugStreams.clear();
  infoStreams.clear();
  warningStreams.clear();
  criticalStreams.clear();
  fatalStreams.clear();

  for(QTextStream *str : streams)
    str->flush();
  qDeleteAll(streams);
  streams.clear();

  for(QFile *file : files)
    file->close();
  qDeleteAll(files);
  files.clear();
}

QStringList LoggingConfig::getLogFiles()
{
  QStringList retval;

  for(QFile *f : files)
    retval.append(QFileInfo(f->fileName()).absoluteFilePath());

  return retval;
}

QVector<QTextStream *> LoggingConfig::getStream(QtMsgType type)
{
  switch(type)
  {
    case QtDebugMsg:
      return debugStreams;

    case QtInfoMsg:
      return infoStreams;

    case QtWarningMsg:
      return warningStreams;

    case QtCriticalMsg:
      return criticalStreams;

    case QtFatalMsg:
      return fatalStreams;
  }
  return emptyStreams;
}

QHash<QString, QVector<QTextStream *> > LoggingConfig::getCatStream(QtMsgType type)
{
  switch(type)
  {
    case QtDebugMsg:
      return debugStreamsCat;

    case QtInfoMsg:
      return infoStreamsCat;

    case QtWarningMsg:
      return warningStreamsCat;

    case QtCriticalMsg:
      return criticalStreamsCat;

    case QtFatalMsg:
      return fatalStreamsCat;
  }
  return emptyStreamsCat;
}

void LoggingConfig::addDefaultChannels(const QStringList& channelsForLevel,
                                       const QHash<QString, QTextStream *>& channelMap,
                                       QVector<QTextStream *>& streamList)
{
  for(QString name : channelsForLevel)
    if(channelMap.contains(name))
      streamList.append(channelMap.value(name));
}

void LoggingConfig::addCatChannels(const QString& category, const QStringList& channelsForLevel,
                                   const QHash<QString, QTextStream *>& channelMap,
                                   QHash<QString, QVector<QTextStream *> >& streamList)
{
  for(QString name : channelsForLevel)
    if(channelMap.contains(name))
    {
      if(streamList.contains(category))
        streamList[category].append(channelMap.value(name));
      else
      {
        QVector<QTextStream *> catstr;
        catstr.append(channelMap.value(name));
        streamList.insert(category, catstr);
      }
    }
}

void LoggingConfig::readConfigurationSection(QSettings *settings)
{
  QString defaultPattern("[%{time yyyy-MM-dd h:mm:ss.zzz} %{category} "
                         "%{if-debug}DEBUG%{endif}"
                         "%{if-info}INFO %{endif}"
                         "%{if-warning}WARN %{endif}"
                         "%{if-critical}CRIT %{endif}"
                         "%{if-fatal}FATAL%{endif}]: %{message}");

  // Set the configured message pattern
  qSetMessagePattern(settings->value("configuration/messagepattern", QVariant(defaultPattern)).toString());

  QString filesParameter = settings->value("configuration/files").toString();
  if(filesParameter == "truncate" || filesParameter == "roll")
    mode = QIODevice::WriteOnly | QIODevice::Text;
  else if(filesParameter == "append")
    mode = QIODevice::Append | QIODevice::Text;
  else
  {
    qWarning() << "Invalid value for configuration/files:" << filesParameter
               << "use either truncate, append or roll.";
    mode = QIODevice::WriteOnly | QIODevice::Text;
  }

  rolling = settings->value("configuration/files").toString() == "roll";
  maxFiles = settings->value("configuration/maxfiles").toInt();

  QString abortOn = settings->value("configuration/abort", QVariant("fatal")).toString();
  if(abortOn == "warning")
    abortType = QtWarningMsg;
  else if(abortOn == "critical")
    abortType = QtCriticalMsg;
  else if(abortOn == "fatal")
    abortType = QtFatalMsg;
  else
    qWarning() << "Invalid value for configuration/abort:" << abortOn
               << "use either warning, critical or fatal (default).";
}

void LoggingConfig::readChannels(QSettings *settings, QHash<QString, QTextStream *>& channelMap)
{
  io::FileRoller fr(maxFiles);

  settings->beginGroup("channels");
  for(QString key : settings->allKeys())
  {
    QString channelName = settings->value(key).toString();

    if(channelName == "stdio")
    {
      QTextStream *io = new QTextStream(stdout);
      // Most terminals can deal with utf-8
      io->setCodec("UTF-8");
      channelMap.insert(key, io);
      streams.append(io);
    }
    else if(channelName == "stderr")
    {
      QTextStream *err = new QTextStream(stderr);
      // Most terminals can deal with utf-8
      err->setCodec("UTF-8");
      channelMap.insert(key, err);
      streams.append(err);
    }
    else
    {
      // Create a stream for the channel
      QString filename = channelName;
      if(filename.isEmpty())
      {
        // No filename
        if(logPrefix.isEmpty())
          filename = QApplication::organizationName().replace(" ", "_").toLower() + "-" +
                     QApplication::applicationName().replace(" ", "_").toLower();
        else
          filename = logPrefix;
      }
      else if(!logPrefix.isEmpty())
        filename = logPrefix + filename;

      if(!filename.endsWith(".log", Qt::CaseInsensitive))
        filename += ".log";

      if(!logDir.isEmpty())
        filename = logDir + QDir::separator() + filename;

      if(rolling)
        // Create log file backups
        fr.rollFile(filename);

      QFile *file = new QFile(filename);
      if(file->open(mode))
      {
        QTextStream *str = new QTextStream(file);
        str->setCodec("UTF-8");
        channelMap.insert(key, str);
        files.append(file);
        streams.append(str);
      }
    }
  }
  settings->endGroup();
}

void LoggingConfig::readLevels(QSettings *settings, QHash<QString, QTextStream *>& channelMap)
{
  settings->beginGroup("levels");
  for(QString levelName : settings->allKeys())
  {
    // Split the "level.channel" string
    QStringList levelList = levelName.split(".", QString::SkipEmptyParts);
    QString level = levelList.at(0);

    // Use default if category is not given
    QString category = levelList.size() > 1 ? levelList.at(1) : "default";

    QStringList channels = settings->value(levelName).toStringList();
    if(category == "default")
    {
      // assign default channels to the corresponding
      // stream list (debugStreams, ...)
      if(level == "debug")
        addDefaultChannels(channels, channelMap, debugStreams);
      else if(level == "info")
        addDefaultChannels(channels, channelMap, infoStreams);
      else if(level == "warning")
        addDefaultChannels(channels, channelMap, warningStreams);
      else if(level == "critical")
        addDefaultChannels(channels, channelMap, criticalStreams);
      else if(level == "fatal")
        addDefaultChannels(channels, channelMap, fatalStreams);
    }
    else
    {
      if(level == "debug")
        addCatChannels(category, channels, channelMap, debugStreamsCat);
      else if(level == "info")
        addCatChannels(category, channels, channelMap, infoStreamsCat);
      else if(level == "warning")
        addCatChannels(category, channels, channelMap, warningStreamsCat);
      else if(level == "critical")
        addCatChannels(category, channels, channelMap, criticalStreamsCat);
      else if(level == "fatal")
        addCatChannels(category, channels, channelMap, fatalStreamsCat);
    }
  }
  settings->endGroup();
}

} // namespace internal
} // namespace logging
} // namespace atools
