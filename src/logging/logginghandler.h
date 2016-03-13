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

#ifndef ATOOLS_LOGGING_LOGGINGHANDLER_H
#define ATOOLS_LOGGING_LOGGINGHANDLER_H

#include <QString>
#include <QIODevice>
#include <QHash>
#include <QVector>
#include <QMutex>

class QTextStream;
class QFile;
class QStringList;
class QSettings;

namespace atools {
namespace logging {
namespace internal {
class LoggingConfig;
}

/*
 * Reads a log4j like configuration file and directs the
 * qDebug, qInfo, etc. streams to stdout, stderr or file
 * streams depending on configuration as shown below.
 *
 *
 * [configuration]
 * messagepattern = [%{time yyyy-MM-dd h:mm:ss.zzz} %{category} \
 * %{if-debug}DEBUG%{endif}%{if-info}INFO %{endif}%{if-warning}WARN
 * %{endif}%{if-critical}CRIT %{endif}%{if-fatal}FATAL%{endif}]
 * %{file}:%{line}:
 * %{message}
 *
 * files = roll
 * maxfiles = 2
 * abort = fatal
 *
 * [channels]
 * console     = stdio
 * console-err = stderr
 * log         = myapplication.log
 *
 * [levels]
 * debug.default   = log
 * info.default     = console,log
 * warning.default  = console-err,log
 * critical.default = console-err,log
 * fatal.default    = console-err,log
 *
 */
class LoggingHandler
{
public:
  /*
   * Loads the logging configuration and prepares all log files to be stored in
   * the given directory.
   *
   * @param logConfiguration Path of the log configuration file. See above.
   * @param logDirectory Where the log files should be stored.
   * @param logFilePrefix Prefix all log file names with this string
   */
  static void initialize(const QString& logConfiguration,
                         const QString& logDirectory = QString(), const QString& logFilePrefix = QString());

  /*
   * Loads the logging configuration and prepares all log files to be stored in
   * the system defined temporary directory.
   *
   * @param logConfiguration Path of the log configuration file. See above.
   */
  static void initializeForTemp(const QString& logConfiguration);

  /*
   * Flush and close all logging streams/files. After this call the default Qt
   * logging will be used again.
   */
  static void shutdown();

  /*
   * @return A list of log files using absolute path
   */
  static QStringList getLogFiles();

private:
  LoggingHandler(const QString& logConfiguration, const QString& logDirectory, const QString& logFilePrefix);
  ~LoggingHandler();

  void logToCatChannels(const QHash<QString, QVector<QTextStream *> >& streamListCat,
                        const QVector<QTextStream *>& streamList2,
                        const QString& message,
                        const QString& category = QString());

  void checkAbortType(QtMsgType type);

  static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);

  static LoggingHandler *instance;

  atools::logging::internal::LoggingConfig *logConfig;
  QtMessageHandler oldMessageHandler;

  mutable QMutex mutex;
};

} // namespace logging
} // namespace atools

#endif // ATOOLS_LOGGING_LOGGINGHANDLER_H
