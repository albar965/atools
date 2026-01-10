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

#include "logging/logginghandler.h"
#include "atools.h"
#include "logging/loggingconfig.h"

#include <QDebug>
#include <QDir>
#include <QCoreApplication>
#include <QThread>

namespace atools {
namespace logging {

using internal::LoggingConfig;
using internal::Channel;

LoggingHandler *LoggingHandler::instance = nullptr;
LoggingHandler::LogFunctionType LoggingHandler::logFunc;
LoggingHandler::AbortFunctionType LoggingHandler::abortFunc;
QWidget *LoggingHandler::parentWidget = nullptr;

LoggingHandler::LoggingHandler(const QString& logConfiguration,
                               const QString& logDirectory,
                               const QString& logFilePrefix)
{
  logConfig = new LoggingConfig(logConfiguration, logDirectory, logFilePrefix);

  // Override category filter since some systems disable debug logging in the qtlogging.ini
  oldCategoryFilter = QLoggingCategory::installFilter(categoryFilter);

  // Install callback function
  if(logConfig->narrow)
    oldMessageHandler = qInstallMessageHandler(LoggingHandler::messageHandlerNarrow);
  else
    oldMessageHandler = qInstallMessageHandler(LoggingHandler::messageHandler);
}

LoggingHandler::~LoggingHandler()
{
  qInfo() << Q_FUNC_INFO;
}

void LoggingHandler::initialize(const QString& logConfiguration, const QString& logDirectory, const QString& logFilePrefix)
{
  if(instance == nullptr)
    instance = new LoggingHandler(logConfiguration, logDirectory, logFilePrefix);
  else
    qWarning() << "LoggingHandler::initialize called more than once";
}

void LoggingHandler::initialize(const QString& logConfiguration, const QString& logDirectory)
{
  if(instance == nullptr)
    instance = new LoggingHandler(logConfiguration, logDirectory, prefix());
  else
    qWarning() << "LoggingHandler::initialize called more than once";
}

void LoggingHandler::initialize(const QString& logConfiguration)
{
  if(instance == nullptr)
    instance = new LoggingHandler(logConfiguration, QDir::currentPath(), prefix());
  else
    qWarning() << "LoggingHandler::initialize called more than once";
}

void LoggingHandler::initializeForTemp(const QString& logConfiguration)
{
  if(instance == nullptr)
    instance = new LoggingHandler(logConfiguration, QDir::tempPath(), prefix());
  else
    qWarning() << "LoggingHandler::initializeForTemp called more than once";
}

QString LoggingHandler::prefix()
{
  return QCoreApplication::organizationName().replace(" ", "_").toLower() + "-" +
         QCoreApplication::applicationName().replace(" ", "_").toLower();
}

const QStringList LoggingHandler::getLogFiles(bool includeBackups)
{
  if(instance != nullptr)
    return instance->logConfig->getLogFiles(includeBackups);
  else
    return QStringList();
}

void LoggingHandler::logToCatChannels(internal::ChannelMap& streamListCat,
                                      internal::ChannelList& streamList, const QString& message, const QString& category)
{
  QMutexLocker locker(&instance->mutex);

  if(category.isEmpty())
  {
    for(Channel *channel : streamList)
    {
      (*channel->stream) << message << Qt::endl << Qt::flush;
      instance->logConfig->checkStreamSize(channel);
    }
  }
  else if(streamListCat.contains(category))
  {
    for(Channel *channel : std::as_const(streamListCat[category]))
    {
      (*channel->stream) << message << Qt::endl << Qt::flush;
      instance->logConfig->checkStreamSize(channel);
    }
  }
}

void LoggingHandler::checkAbortType(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
  QtMsgType abortType = logConfig->getAbortType();
  bool doAbort = false;
  switch(type)
  {
    case QtDebugMsg:
    case QtInfoMsg:
      break;

    case QtWarningMsg:
      doAbort = abortType == QtWarningMsg;
      break;

    case QtCriticalMsg:
      doAbort = abortType == QtWarningMsg || abortType == QtCriticalMsg;
      break;

    case QtFatalMsg:
      doAbort = abortType == QtWarningMsg || abortType == QtCriticalMsg || abortType == QtFatalMsg;
      break;
  }

  if(doAbort)
  {
    if(abortFunc)
      abortFunc(type, context, msg);
    else
      abort();
  }
}

void LoggingHandler::messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
  static const QLatin1String DEFAULT("default");

  if(logFunc)
    logFunc(type, context, msg);

  QString category = context.category;
  if(category == DEFAULT)
    category.clear();

  instance->logToCatChannels(instance->logConfig->getCatStream(type), instance->logConfig->getStream(type),
                             qFormatLogMessage(type, context, msg), category);

  instance->checkAbortType(type, context, msg);
}

void LoggingHandler::messageHandlerNarrow(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
  static const QLatin1String DOUBLE_COLON("::");
  static const QLatin1String VOID("void ");
  static const QLatin1String STATIC_VOID("static void ");
  static const QLatin1String VIRTUAL_VOID("virtual void ");
  static const QLatin1String VIRTUAL("virtual ");
  static const QLatin1String DEFAULT("default");

  if(msg.contains("invalid bitmap"))
  {
    bool dummy = false;
  }

  QString message = msg;
  QString function(context.function);
  QString file(context.file);

  // Remove file path ========================
  int idx = file.lastIndexOf(QDir::separator());
  if(idx != -1)
    file = file.mid(idx + 1);

  // Remove function name from message ========================
  bool funcRemoved = false;
  if(message.startsWith(function))
  {
    message.remove(0, function.size());
    funcRemoved = true;
  }

  // Remove method signature ========================
  idx = function.lastIndexOf('(');
  if(idx != -1)
    function = function.left(idx + 1) + ')';

  // Remove namespaces ========================
  int cnt = function.count(DOUBLE_COLON);
  if(cnt > 1)
  {
    // Get last :: in "namespace::Class::method"
    idx = function.lastIndexOf(DOUBLE_COLON);
    if(idx != -1)
    {
      // Get second last :: in "namespace::Class::method"
      int idx2 = function.lastIndexOf(DOUBLE_COLON, idx - 2);
      if(idx2 != -1)
        function = function.mid(idx2 + 2);
      else
        function = function.mid(idx);
    }
  }

  // Remove any return types ====================
  if(function.startsWith(VOID))
    function.remove(0, VOID.size());
  if(function.startsWith(STATIC_VOID))
    function.remove(0, STATIC_VOID.size());
  if(function.startsWith(VIRTUAL_VOID))
    function.remove(0, VIRTUAL_VOID.size());
  if(function.startsWith(VIRTUAL))
    function.remove(0, VIRTUAL.size());

  // Add shortened function name again if it was removed before
  if(funcRemoved)
    message.prepend(function);

  // Keep arrays for the lifetime of this function until logging is finished
  QByteArray fileBytes(file.toLocal8Bit());
  QByteArray functionBytes(function.toLocal8Bit());

  // Create a copy of the context
  QMessageLogContext ctx;
  ctx.version = context.version;
  ctx.line = context.line;
  ctx.file = fileBytes.constData();
  ctx.function = functionBytes.constData();
  ctx.category = context.category;

  if(logFunc)
    logFunc(type, ctx, message);

  QString category = ctx.category;

  if(category == DEFAULT)
    category.clear();

  instance->logToCatChannels(instance->logConfig->getCatStream(type), instance->logConfig->getStream(type),
                             qFormatLogMessage(type, ctx, message), category);

  instance->checkAbortType(type, ctx, message);

  // Null pointers to avoid double free
  ctx.file = nullptr;
  ctx.function = nullptr;
  ctx.category = nullptr;
}

void LoggingHandler::categoryFilter(QLoggingCategory *category)
{
  // Enable all - we do our own category filtering
  if(category != nullptr)
  {
    category->setEnabled(QtDebugMsg, true);
    category->setEnabled(QtCriticalMsg, true);
    category->setEnabled(QtInfoMsg, true);
    category->setEnabled(QtWarningMsg, true);
  }
}

} // namespace logging
} // namespace atools
