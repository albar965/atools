/*****************************************************************************
* Copyright 2015-2023 Alexander Barthel alex@littlenavmap.org
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

#include "gui/application.h"
#include "atools.h"

#include <cstdlib>
#include <QDebug>
#include <QMessageBox>
#include <QUrl>
#include <QThread>
#include <QTimer>
#include <QProcess>

namespace atools {
namespace gui {

QHash<QString, QStringList> Application::reportFiles;
QStringList Application::emailAddresses;
QSet<QObject *> Application::tooltipExceptions;

bool Application::showExceptionDialog = true;
bool Application::restartProcess = false;
bool Application::tooltipsDisabled = false;

Application::Application(int& argc, char **argv, int)
  : QApplication(argc, argv)
{
}

Application::~Application()
{
  if(restartProcess)
  {
    qDebug() << Q_FUNC_INFO << "Starting" << QCoreApplication::applicationFilePath();
    restartProcess = false;
    bool result = QProcess::startDetached(QCoreApplication::applicationFilePath(), QCoreApplication::arguments());
    if(result)
      qInfo() << Q_FUNC_INFO << "Success.";
    else
      qWarning() << Q_FUNC_INFO << "FAILED.";
  }
}

Application *Application::applicationInstance()
{
  return dynamic_cast<Application *>(QCoreApplication::instance());
}

bool Application::notify(QObject *receiver, QEvent *event)
{
#ifndef DEBUG_NO_APPLICATION_EXCEPTION
  try
  {
#endif
  if(tooltipsDisabled && (event->type() == QEvent::ToolTip || event->type() == QEvent::ToolTipChange) &&
     !tooltipExceptions.contains(receiver))
    return false;
  else
    return QApplication::notify(receiver, event);

#ifndef DEBUG_NO_APPLICATION_EXCEPTION
}
catch(std::exception& e)
{
  qCritical() << "receiver" << (receiver == nullptr ? "null" : receiver->objectName());
  qCritical() << "event" << (event == nullptr ? 0 : static_cast<int>(event->type()));

  ATOOLS_HANDLE_EXCEPTION(e);
}
catch(...)
{
  qCritical() << "receiver" << (receiver == nullptr ? "null" : receiver->objectName());
  qCritical() << "event" << (event == nullptr ? 0 : static_cast<int>(event->type()));

  ATOOLS_HANDLE_UNKNOWN_EXCEPTION;
}
#endif
}

QString Application::generalErrorMessage()
{
  return tr("<b>If the problem persists or occurs during startup "
              "delete all settings and database files of %1 and try again.</b><br/><br/>"
              "<b>If you wish to report this error attach the log and configuration files "
                "to your report, add all other available information and send it to "
                "the contact address below.</b><br/>").arg(QCoreApplication::applicationName());
}

void Application::setTooltipsDisabled(const QList<QObject *>& exceptions)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
  tooltipExceptions = QSet<QObject *>(exceptions.begin(), exceptions.end());
#else
  tooltipExceptions = exceptions.toSet();
#endif
  tooltipsDisabled = true;
}

void Application::setTooltipsEnabled()
{
  tooltipExceptions.clear();
  tooltipsDisabled = false;
}

void Application::handleException(const char *file, int line, const std::exception& e)
{
  qCritical() << "Caught exception in file" << file << "line" << line << "what" << e.what();

  if(showExceptionDialog)
    QMessageBox::critical(nullptr, QCoreApplication::applicationName(),
                          tr("<b>Caught exception in file \"%1\" line %2.</b><br/><br/>"
                             "<i>%3</i><br/><br/>"
                             "%4"
                             "<hr/>%5"
                               "<hr/>%6<br/>"
                               "<h3>Press OK to exit application.</h3>"
                             ).
                          arg(file).arg(line).
                          arg(e.what()).
                          arg(generalErrorMessage()).
                          arg(getContactHtml()).
                          arg(getReportPathHtml()));

  std::abort();
}

void Application::handleException(const char *file, int line)
{
  qCritical() << "Caught unknown exception in file" << file << "line" << line;

  if(showExceptionDialog)
    QMessageBox::critical(nullptr, QCoreApplication::applicationName(),
                          tr("<b>Caught unknown exception in file %1 line %2.</b><br/><br/>"
                             "%2"
                             "<hr/>%4"
                               "<hr/>%5<br/>"
                               "<h3>Press OK to exit application.</h3>"
                             ).
                          arg(file).arg(line).
                          arg(generalErrorMessage()).
                          arg(getContactHtml()).
                          arg(getReportPathHtml()));

  std::abort();
}

void Application::addReportPath(const QString& header, const QStringList& paths)
{
  reportFiles.insert(header, paths);
}

QString Application::getContactHtml()
{
  QString contactStr(tr("<b>Contact:</b><br/>"));

  contactStr.append(tr("<a href=\"https://www.littlenavmap.org/contact.html\">"
                         "Little Navmap - Contact and Support</a>"));
  return contactStr;
}

QString Application::getEmailHtml()
{
  QString mailStr(tr("<b>Contact:</b><br/>"));

  QStringList emails;
  for(QString mail : emailAddresses)
    emails.append(QString("<a href=\"mailto:%1\">%1</a>").arg(mail));
  mailStr.append(emails.join(" or "));
  return mailStr;
}

void Application::processEventsExtended()
{
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  QThread::msleep(10);
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

QString Application::getReportPathHtml()
{
  // Sort keys to avoid random order
  QList<QString> keys = reportFiles.keys();
  std::sort(keys.begin(), keys.end());

  QString fileStr;
  for(QString header : keys)
  {
    fileStr.append(tr("<b>%1</b><br/>").arg(header));
    const QStringList& paths = reportFiles.value(header);

    for(const QString& path : paths)
      fileStr += tr("<a href=\"%1\">%2</a><br/>").arg(QUrl::fromLocalFile(path).toString()).arg(atools::elideTextShortLeft(path, 80));

    if(header != keys.constLast())
      fileStr.append(tr("<br/>"));
  }

  return fileStr;
}

} // namespace gui
} // namespace atools
