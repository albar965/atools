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

#ifndef ATOOLS_LOGGING_LOGGINGCONFIG_H
#define ATOOLS_LOGGING_LOGGINGCONFIG_H

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
  QVector<QTextStream *> getStream(QtMsgType type);

  /* get all categorized streams for the given level */
  QHash<QString, QVector<QTextStream *> > getCatStream(QtMsgType type);

  void addDefaultChannels(const QStringList& channelsForLevel,
                          const QHash<QString, QTextStream *>& channelMap,
                          QVector<QTextStream *>& streamList);
  void addCatChannels(const QString& category, const QStringList& channelsForLevel,
                      const QHash<QString, QTextStream *>& channelMap,
                      QHash<QString, QVector<QTextStream *> >& streamList);

  /* Populate the channelMap with name/stream pairs (stdio, stderr and files)
   *
   * [channels]
   * console = stdio
   * console-err = stderr
   * log = littlelogbook.log
   */
  void readChannels(QSettings *settings, QHash<QString, QTextStream *>& channelMap);

  /* Read the channelMap and assign the log levels to channels
   *
   * [levels]
   * debug.default =
   * info.default = log
   * warning.default = console-err,log
   * critical.default = console-err,log
   * fatal.default = console-err,log
   */
  void readLevels(QSettings *settings, QHash<QString, QTextStream *>& channelMap);

  QIODevice::OpenMode readFilesParameter(QSettings *settings) const;
  bool readFilesRollParameter(QSettings *settings) const;
  int readFilesMaxParameter(QSettings *settings) const;
  void readConfigurationSection(QSettings *settings);

  QIODevice::OpenMode mode = QIODevice::NotOpen;
  bool rolling = false;
  int maxFiles = 0;

  QString logConfig, logDir, logPrefix;

  // All open files for deletion
  QVector<QFile *> files;
  // All open streams for deletion
  QVector<QTextStream *> streams;

  // Messages of this type or worse cause a call to abort()
  QtMsgType abortType = QtFatalMsg;

  // Streams for default logging category one for each message type
  QVector<QTextStream *> debugStreams, infoStreams, warningStreams, criticalStreams, fatalStreams,
                         emptyStreams;
  // Streams for all other categories in a hash-list where the key is the
  // category name.
  // One container for each message type
  QHash<QString, QVector<QTextStream *> > debugStreamsCat, infoStreamsCat, warningStreamsCat,
                                          criticalStreamsCat, fatalStreamsCat, emptyStreamsCat;

};

} // namespace internal
} // namespace logging
} // namespace atools

#endif // ATOOLS_LOGGING_LOGGINGCONFIG_H
