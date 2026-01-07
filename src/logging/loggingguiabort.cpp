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

#include "logging/loggingguiabort.h"

#include "logging/logginghandler.h"

#if defined(QT_WIDGETS_LIB)
#include "gui/application.h"
#include "gui/dialog.h"
#include <QMainWindow>
#endif

#include <QCoreApplication>

#include <QThread>
#include <QDebug>

namespace atools {
namespace logging {

LoggingGuiAbortHandler *LoggingGuiAbortHandler::instance = nullptr;

void LoggingGuiAbortHandler::guiAbortFunction(const QString& msg)
{
  qDebug() << Q_FUNC_INFO;

  if(LoggingHandler::instance == nullptr)
    qWarning() << Q_FUNC_INFO << "LoggingHandler::instance==nullptr";

#if defined(QT_WIDGETS_LIB)
  // Called by signal on main thread context
  if(atools::gui::Application::isShowExceptionDialog())
    atools::gui::Dialog::warning(LoggingHandler::parentWidget,
                                 QObject::tr("<b>A fatal error has occurred.</b><br/><br/>"
                                             "%1<br/><br/>"
                                             "%2"
                                             "<hr/>%3"
                                               "<hr/>%4<br/>"
                                               "<h3>Press OK to exit application.</h3>").
                                 arg(msg).arg(atools::gui::Application::generalErrorMessage()).
                                 arg(atools::gui::Application::getContactHtml()).arg(atools::gui::Application::getReportPathHtml()));
#else
  Q_UNUSED(msg)
#endif

#ifdef Q_OS_WIN32
  // Will not call any crash handler on windows - is not helpful anyway
  std::exit(1);
#else
  // Allow OS crash handler to pop up i.e. generate a core file under linux or show crash dialog on macOS
  abort();
#endif
}

LoggingGuiAbortHandler::LoggingGuiAbortHandler()
{

}

LoggingGuiAbortHandler::~LoggingGuiAbortHandler()
{

}

void LoggingGuiAbortHandler::setGuiAbortFunction(QWidget *parent)
{
  qDebug() << Q_FUNC_INFO;

  if(LoggingHandler::instance == nullptr)
    qWarning() << Q_FUNC_INFO << "LoggingHandler::instance==nullptr";

  if(instance == nullptr)
    instance = new LoggingGuiAbortHandler();

  LoggingHandler::parentWidget = parent;
  // Connect to slot with queued connection to allow passing the message to the main thread
  connect(LoggingHandler::instance, &LoggingHandler::guiAbortSignal, instance,
          &LoggingGuiAbortHandler::guiAbortFunction, Qt::QueuedConnection);

  LoggingHandler::setAbortFunction([](QtMsgType type, const QMessageLogContext& context, const QString& msg) -> void
      {
        Q_UNUSED(type)
        Q_UNUSED(context)

        // Call guiAbortFunc on main thread context
        emit LoggingHandler::instance->guiAbortSignal(msg);

        // Allow delivery
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

        // Stop this thread forever
        // This will never be called if this is the main thread
        QThread::msleep(std::numeric_limits<unsigned long>::max());
      });
}

void LoggingGuiAbortHandler::resetGuiAbortFunction()
{
  delete instance;
  instance = nullptr;
}

} // namespace logging
} // namespace atools
