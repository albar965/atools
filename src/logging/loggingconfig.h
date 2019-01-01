/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_LOGGING_LOGGINGCONFIG_H
#define ATOOLS_LOGGING_LOGGINGCONFIG_H

#include "logging/loggingtypes.h"

#include <QString>
#include <QIODevice>
#include <QVector>
#include <QHash>

class QFile;
class QTextStream;
class QSettings;

namespace atools {
namespace logging {
class LoggingHandler;
namespace internal {

/* Internal logging class that reads the configuration and sets up all the
 * streams. */
class LoggingConfig
{
public:
  LoggingConfig(const QString& logConfig, const QString& logDirectory, const QString& logFilePrefix);
  ~LoggingConfig();

private:
  friend class atools::logging::LoggingHandler;

  /* get a list of log files (excluding stdout and stderr) */
  QStringList getLogFiles();

  /* Get the message level that should cause the application to abort */
  QtMsgType getAbortType() const
  {
    return abortType;
  }

  /* get all default streams for the given level */
  ChannelVector& getStream(QtMsgType type);

  /* get all categorized streams for the given level */
  ChannelMap& getCatStream(QtMsgType type);

  void addDefaultChannels(const QStringList& channelsForLevel, const QHash<QString, Channel *>& channelMap,
                          ChannelVector& channelList);

  void addCatChannels(const QString& category, const QStringList& channelsForLevel,
                      const QHash<QString, Channel *>& channelMap, ChannelMap& streamList);

  /* Populate the channelMap with name/stream pairs (stdio, stderr and files)
   *
   * [channels]
   * console = stdio
   * console-err = stderr
   * log = littlelogbook.log
   */
  void readChannels(QSettings *settings, QHash<QString, Channel *>& channelMap);

  /* Read the channelMap and assign the log levels to channels
   *
   * [levels]
   * debug.default =
   * info.default = log
   * warning.default = console-err,log
   * critical.default = console-err,log
   * fatal.default = console-err,log
   */
  void readLevels(QSettings *settings, QHash<QString, Channel *>& channelMap);

  QIODevice::OpenMode readFilesParameter(QSettings *settings) const;
  bool readFilesRollParameter(QSettings *settings) const;
  int readFilesMaxParameter(QSettings *settings) const;
  void readConfigurationSection(QSettings *settings);

  void closeStreams(QSet<Channel *>& channels,
                    const ChannelMap& channelMap);

  void closeStreams(QSet<Channel *>& channels, const ChannelVector& channelVector);

  /* Collects all log files names into set */
  void collectFileNames(QSet<QString>& filenames, const ChannelVector& channelVector);
  void collectFileNames(QSet<QString>& filenames, const ChannelMap& channelMap);

  /* Check if file size exceeds limit. Rolls files, creates a new one and replaces device in text stream */
  void checkStreamSize(Channel *channel);

  QIODevice::OpenMode mode = QIODevice::NotOpen;
  bool rolling = false;
  int maximumBackupFiles = 0;

  /* 0 of -1 if not used */
  qint64 maximumFileSizeBytes = 0;

  QString logConfig, logDir, logPrefix;

  // Messages of this type or worse cause a call to abort()
  QtMsgType abortType = QtFatalMsg;

  // Streams for default logging category one for each message type
  // Channel objects can be shared between lists
  ChannelVector debugStreams, infoStreams, warningStreams, criticalStreams, fatalStreams,
                emptyStreams;
  // Streams for all other categories in a hash-list where the key is the category name.
  // One container for each message type
  // Channel objects can be shared between lists
  ChannelMap debugStreamsCat, infoStreamsCat, warningStreamsCat,
             criticalStreamsCat, fatalStreamsCat, emptyStreamsCat;
};

} // namespace internal
} // namespace logging
} // namespace atools

#endif // ATOOLS_LOGGING_LOGGINGCONFIG_H
