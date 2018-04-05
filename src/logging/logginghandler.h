/*****************************************************************************
* Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
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
#include <QLoggingCategory>

#include <functional>

class QTextStream;
class QFile;
class QStringList;
class QSettings;

namespace atools {
namespace logging {
namespace internal {
class LoggingConfig;
}

class LoggingGuiAbortHandler;

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
class LoggingHandler :
  public QObject
{
  Q_OBJECT

public:
  /*
   * Loads the logging configuration and prepares all log files to be stored in
   * the given directory.
   *
   * @param logConfiguration Path of the log configuration file. See above.
   * @param logDirectory Where the log files should be stored.
   * @param logFilePrefix Prefix all log file names with this string
   */
  static void initialize(const QString& logConfiguration, const QString& logDirectory, const QString& logFilePrefix);

  /* Uses current directory for log and application name as prefix */
  static void initialize(const QString& logConfiguration);

  /*
   * Loads the logging configuration and prepares all log files to be stored in
   * the system defined temporary directory.
   * This will prefix all log files with orgranization and application name and append ".log"
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

  typedef std::function<void (QtMsgType type, const QMessageLogContext &context, const QString &msg)> LogFunctionType;
  /* Function will be called on the calling thread context */
  static void setLogFunction(LogFunctionType loggingFunction);

  typedef std::function<void (QtMsgType type, const QMessageLogContext &context, const QString &msg)> AbortFunctionType;

  /* All functions will be called on the calling thread context - do not use GUI elements there */
  static void setAbortFunction(AbortFunctionType abortFunction);
  static void resetAbortFunction();

signals:
  /* Sent to main thread to allow GUI handling */
  void guiAbortSignal(const QString& msg);

private:
  /* Moved GUI elements out to avoid linking to qwidgets */
  friend class LoggingGuiAbortHandler;

  LoggingHandler(const QString& logConfiguration, const QString& logDirectory, const QString& logFilePrefix);
  ~LoggingHandler();

  void logToCatChannels(const QHash<QString, QVector<QTextStream *> >& streamListCat,
                        const QVector<QTextStream *>& streamList2,
                        const QString& message,
                        const QString& category = QString());

  void checkAbortType(QtMsgType type, const QMessageLogContext& context, const QString& msg);

  static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);
  static void categoryFilter(QLoggingCategory *category);

  static LoggingHandler *instance;

  atools::logging::internal::LoggingConfig *logConfig;
  QtMessageHandler oldMessageHandler = nullptr;
  QLoggingCategory::CategoryFilter oldCategoryFilter = nullptr;

  mutable QMutex mutex;

  static LogFunctionType logFunc;
  static AbortFunctionType abortFunc;
  static QWidget *parentWidget;

};

} // namespace logging
} // namespace atools

#endif // ATOOLS_LOGGING_LOGGINGHANDLER_H
