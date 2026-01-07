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

#include "logging/loggingconfig.h"
#include "atools.h"
#include "settings/settings.h"
#include "io/fileroller.h"

#include <QDebug>
#include <QDir>
#include <QSettings>
#include <QCoreApplication>
#include <QDateTime>
#include <QStringBuilder>

namespace atools {
namespace logging {
namespace internal {

LoggingConfig::LoggingConfig(const QString& logConfiguration, const QString& logDirectory, const QString& logFilePrefix)
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

  QHash<QString, Channel *> channelMap;
  // Create all file streams and add them to the channelMap
  readChannels(settings, channelMap);

  // Assign log levels to channels
  readLevels(settings, channelMap);

  delete settings;
}

LoggingConfig::~LoggingConfig()
{
  // Close streams and files and collect channel objects which are duplicated between streams
  QSet<Channel *> channels;

  closeStreams(channels, debugStreams);
  closeStreams(channels, infoStreams);
  closeStreams(channels, warningStreams);
  closeStreams(channels, criticalStreams);
  closeStreams(channels, fatalStreams);
  closeStreams(channels, emptyStreams);

  closeStreams(channels, debugStreamsCat);
  closeStreams(channels, infoStreamsCat);
  closeStreams(channels, warningStreamsCat);
  closeStreams(channels, criticalStreamsCat);
  closeStreams(channels, fatalStreamsCat);
  closeStreams(channels, emptyStreamsCat);

  // Delete channels
  qDeleteAll(channels);
}

void LoggingConfig::closeStreams(QSet<Channel *>& channels, const ChannelMap& channelMap)
{
  for(auto it = channelMap.constBegin(); it != channelMap.constEnd(); ++it)
    closeStreams(channels, it.value());
}

void LoggingConfig::closeStreams(QSet<Channel *>& channels, const ChannelList& channelVector)
{
  for(Channel *channel : channelVector)
  {
    if(channel->stream != nullptr)
    {
      delete channel->stream;
      channel->stream = nullptr;
    }

    if(channel->file != nullptr)
    {
      // This is a log file and not stderr or stdout
      delete channel->file;
      channel->file = nullptr;
    }

    // Remember object for later deletion
    channels.insert(channel);
  }
}

QStringList LoggingConfig::getLogFiles(bool includeBackups) const
{
  QSet<QString> filenames;
  collectFileNames(filenames, debugStreams);
  collectFileNames(filenames, infoStreams);
  collectFileNames(filenames, warningStreams);
  collectFileNames(filenames, criticalStreams);
  collectFileNames(filenames, fatalStreams);
  collectFileNames(filenames, emptyStreams);

  collectFileNames(filenames, debugStreamsCat);
  collectFileNames(filenames, infoStreamsCat);
  collectFileNames(filenames, warningStreamsCat);
  collectFileNames(filenames, criticalStreamsCat);
  collectFileNames(filenames, fatalStreamsCat);
  collectFileNames(filenames, emptyStreamsCat);

  QStringList filenameList;
  for(const QString& filename : std::as_const(filenames))
  {
    if(QFile::exists(filename))
      filenameList.append(filename);

    if(includeBackups)
    {
      for(int i = 1; i <= maximumBackupFiles; i++)
      {
        if(QFile::exists(filename % ".1"))
          filenameList.append(filename % ".1");
      }
    }
  }

  std::sort(filenameList.begin(), filenameList.end());

  return filenameList;
}

void LoggingConfig::collectFileNames(QSet<QString>& filenames, const ChannelMap& channelMap) const
{
  for(auto it = channelMap.constBegin(); it != channelMap.constEnd(); ++it)
    collectFileNames(filenames, {it.value()});
}

void LoggingConfig::collectFileNames(QSet<QString>& filenames, const ChannelList& channelVector) const
{
  for(const Channel *channel : channelVector)
  {
    if(channel->file != nullptr)
    {
      QString filename = channel->file->fileName();

      if(!filename.isEmpty())
        filenames.insert(atools::nativeCleanPath(QFileInfo(filename).absoluteFilePath()));
    }
  }
}

void LoggingConfig::checkStreamSize(Channel *channel)
{
  // This needs to be called withing mutex lock
  if(maximumFileSizeBytes > 0 && channel->file != nullptr && channel->file->size() > maximumFileSizeBytes)
  {
    // Maximum size is active and exceeded
    channel->stream->setDevice(nullptr);

    // Remember filename
    QString filename = channel->file->fileName();

    // Close file and delete file object
    delete channel->file;
    channel->file = nullptr;

    // Backup and delete original log
    io::FileRoller(maximumBackupFiles).rollFile(filename);

    // Create new log file
    QFile *file = new QFile(filename);
    if(file->open(fileOpenMode))
    {
      // Put log file into stream
      channel->file = file;
      channel->stream->setDevice(channel->file);

      channel->stream->setLocale(QLocale::C);
    }
  }
}

ChannelList& LoggingConfig::getStream(QtMsgType type)
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

ChannelMap& LoggingConfig::getCatStream(QtMsgType type)
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

void LoggingConfig::addDefaultChannels(const QStringList& channelsForLevel, const QHash<QString, Channel *>& channelMap,
                                       ChannelList& channelList)
{
  for(const QString& name : std::as_const(channelsForLevel))
  {
    if(channelMap.contains(name))
      channelList.append(channelMap.value(name));
  }
}

void LoggingConfig::addCatChannels(const QString& category, const QStringList& channelsForLevel,
                                   const QHash<QString, Channel *>& channelMap, ChannelMap& streamList)
{
  for(const QString& name : std::as_const(channelsForLevel))
  {
    if(channelMap.contains(name))
    {
      if(streamList.contains(category))
        streamList[category].append(channelMap.value(name));
      else
      {
        ChannelList catstr;
        catstr.append(channelMap.value(name));
        streamList.insert(category, catstr);
      }
    }
  }
}

void LoggingConfig::readConfigurationSection(QSettings *settings)
{
  const static QLatin1String DEFAULTPATTERN("[%{time yyyy-MM-dd h:mm:ss.zzz} %{category} "
                                            "%{if-debug}DEBUG%{endif}"
                                            "%{if-info}INFO %{endif}"
                                            "%{if-warning}WARN %{endif}"
                                            "%{if-critical}CRIT %{endif}"
                                            "%{if-fatal}FATAL%{endif}]: %{message}");

  // Use different patterns for debug and release builds
#ifdef QT_NO_DEBUG
  QString pattern = settings->value("configuration/messagepattern", DEFAULTPATTERN).toString();
#else
  // Use release pattern as fallback in debug builds
  QString pattern = settings->value("configuration/messagepatterndebug",
                                    settings->value("configuration/messagepattern", DEFAULTPATTERN)).toString();
#endif

  // Set the configured message pattern
  qSetMessagePattern(pattern);

  maximumFileSizeBytes = settings->value("configuration/maxsize").toLongLong();

#ifndef QT_NO_DEBUG
  narrow = settings->value("configuration/narrow").toBool();
#endif

  QString filesParameter = settings->value("configuration/files").toString();
  if(filesParameter == "truncate" || filesParameter == "roll")
    fileOpenMode = QIODevice::WriteOnly | QIODevice::Text;
  else if(filesParameter == "append")
    fileOpenMode = QIODevice::Append | QIODevice::Text;
  else
  {
    qWarning() << "Invalid value for configuration/files:" << filesParameter << "use either truncate, append or roll.";
    fileOpenMode = QIODevice::WriteOnly | QIODevice::Text;
  }

  // Always append if rolling by size - rolling is done in the logging function
  if(maximumFileSizeBytes > 0)
    fileOpenMode = QIODevice::Append | QIODevice::Text;

  rolling = settings->value("configuration/files").toString() == "roll";
  maximumBackupFiles = settings->value("configuration/maxfiles").toInt();

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

void LoggingConfig::readChannels(QSettings *settings, QHash<QString, Channel *>& channelMap)
{
  settings->beginGroup("channels");
  const QStringList keys = settings->allKeys();
  for(const QString& key : keys)
  {
    QString channelName = settings->value(key).toString();

    if(channelName == "stdio")
    {
      QTextStream *io = new QTextStream(stdout);
      // Most terminals can deal with utf-8
      channelMap.insert(key, new Channel({io, nullptr}));
    }
    else if(channelName == "stderr")
    {
      QTextStream *err = new QTextStream(stderr);
      // Most terminals can deal with utf-8
      channelMap.insert(key, new Channel({err, nullptr}));
    }
    else
    {
      // Create a stream for the channel
      QString filename = channelName;
      if(filename.isEmpty())
      {
        // No filename
        if(logPrefix.isEmpty())
          filename = QCoreApplication::organizationName().replace(" ", "_").toLower() + "-" +
                     QCoreApplication::applicationName().replace(" ", "_").toLower();
        else
          filename = logPrefix;
      }
      else if(!logPrefix.isEmpty())
        filename = logPrefix + filename;

      if(!filename.endsWith(".log", Qt::CaseInsensitive))
        filename += ".log";

      if(!logDir.isEmpty())
        filename = logDir + QDir::separator() + filename;

      if(rolling && maximumFileSizeBytes <= 0)
        // Create log file backups
        io::FileRoller(maximumBackupFiles).rollFile(filename);

      QFile *file = new QFile(filename);
      if(file->open(fileOpenMode))
      {
        QTextStream *stream = new QTextStream(file);
        stream->setLocale(QLocale::C);
        channelMap.insert(key, new Channel({stream, file}));
      }
    }
  }
  settings->endGroup();
}

void LoggingConfig::readLevels(QSettings *settings, QHash<QString, Channel *>& channelMap)
{
  settings->beginGroup("levels");
  const QStringList keys = settings->allKeys();
  for(const QString& levelName : keys)
  {
    // Split the "level.channel" string
    QStringList levelList = levelName.split('.', Qt::SkipEmptyParts);
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
