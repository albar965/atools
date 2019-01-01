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

#include "logging/logginghandler.h"
#include "logging/loggingconfig.h"

#include <QDebug>
#include <QDir>
#include <QApplication>
#include <QThread>

namespace atools {
namespace logging {

using internal::LoggingConfig;
using internal::Channel;

LoggingHandler *LoggingHandler::instance = nullptr;
LoggingHandler::LogFunctionType LoggingHandler::logFunc = nullptr;
LoggingHandler::AbortFunctionType LoggingHandler::abortFunc = nullptr;
QWidget *LoggingHandler::parentWidget = nullptr;

LoggingHandler::LoggingHandler(const QString& logConfiguration,
                               const QString& logDirectory,
                               const QString& logFilePrefix)
{
  logConfig = new LoggingConfig(logConfiguration, logDirectory, logFilePrefix);

  // Override category filter since some systems disable debug logging in the qtlogging.ini
  oldCategoryFilter = QLoggingCategory::installFilter(categoryFilter);

  // Install callback function
  oldMessageHandler = qInstallMessageHandler(LoggingHandler::messageHandler);
}

LoggingHandler::~LoggingHandler()
{
  qInstallMessageHandler(oldMessageHandler);
  QLoggingCategory::installFilter(oldCategoryFilter);
  delete logConfig;
}

void LoggingHandler::initialize(const QString& logConfiguration,
                                const QString& logDirectory,
                                const QString& logFilePrefix)
{
  if(instance == nullptr)
    instance = new LoggingHandler(logConfiguration, logDirectory, logFilePrefix);
  else
    qWarning() << "LoggingHandler::initialize called more than once";
}

void LoggingHandler::initialize(const QString& logConfiguration)
{
  if(instance == nullptr)
    instance = new LoggingHandler(logConfiguration, QDir::currentPath(),
                                  QApplication::organizationName().replace(" ", "_").toLower() + "-" +
                                  QApplication::applicationName().replace(" ", "_").toLower());
  else
    qWarning() << "LoggingHandler::initialize called more than once";
}

void LoggingHandler::initializeForTemp(const QString& logConfiguration)
{
  if(instance == nullptr)
    instance = new LoggingHandler(logConfiguration, QDir::tempPath(),
                                  QApplication::organizationName().replace(" ", "_").toLower() + "-" +
                                  QApplication::applicationName().replace(" ", "_").toLower());
  else
    qWarning() << "LoggingHandler::initializeForTemp called more than once";
}

void LoggingHandler::shutdown()
{
  if(instance != nullptr)
  {
    delete instance;
    instance = nullptr;
  }
  else
    qWarning() << "LoggingHandler::shutdown called more than once";
}

QStringList LoggingHandler::getLogFiles()
{
  if(instance != nullptr)
    return instance->logConfig->getLogFiles();
  else
    return QStringList();
}

void LoggingHandler::setLogFunction(LoggingHandler::LogFunctionType loggingFunction)
{
  logFunc = loggingFunction;
}

void LoggingHandler::setAbortFunction(LoggingHandler::AbortFunctionType abortFunction)
{
  qDebug() << Q_FUNC_INFO;

  abortFunc = abortFunction;
}

void LoggingHandler::resetAbortFunction()
{
  qDebug() << Q_FUNC_INFO;

  abortFunc = nullptr;
  parentWidget = nullptr;
}

void LoggingHandler::logToCatChannels(internal::ChannelMap& streamListCat,
                                      internal::ChannelVector& streamList, const QString& message,
                                      const QString& category)
{
  mutex.lock();

  if(category.isEmpty())
  {
    for(Channel *channel : streamList)
    {
      (*channel->stream) << message << endl << flush;
      instance->logConfig->checkStreamSize(channel);
    }
  }
  else if(streamListCat.contains(category))
  {
    for(Channel *channel : streamListCat[category])
    {
      (*channel->stream) << message << endl << flush;
      instance->logConfig->checkStreamSize(channel);
    }
  }
  mutex.unlock();
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
  if(logFunc != nullptr)
    logFunc(type, context, msg);

  QString category = context.category;

  if(category == "default")
    category.clear();

  instance->logToCatChannels(instance->logConfig->getCatStream(type),
                             instance->logConfig->getStream(type),
                             qFormatLogMessage(type, context, msg),
                             category);

  instance->checkAbortType(type, context, msg);
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
