/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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

#include <cstdlib>
#include <QDebug>
#include <QMessageBox>
#include <QUrl>
#include <QThread>
#include <QTimer>

namespace atools {
namespace gui {

QHash<QString, QStringList> Application::reportFiles;
QStringList Application::emailAddresses;
bool Application::showExceptionDialog = true;

Application::Application(int& argc, char **argv, int)
  : QApplication(argc, argv)
{

}

Application::~Application()
{

}

bool Application::notify(QObject *receiver, QEvent *event)
{
  try
  {
    return QApplication::notify(receiver, event);
  }
  catch(std::exception& e)
  {
    ATOOLS_HANDLE_EXCEPTION(e);
  }
  catch(...)
  {
    ATOOLS_HANDLE_UNKNOWN_EXCEPTION;
  }
}

QString Application::generalErrorMessage()
{
  return tr("<b>If the problem persists or occurs during startup "
              "delete all settings and database files of %4 and try again.</b><br/><br/>"
              "<b>If you wish to report this error attach the log and configuration files "
                "to your report, add all other available information and send it to one "
                "of the contact addresses below.</b><br/>").arg(QApplication::applicationName());
}

void Application::sendFontChanged()
{
  Application *app = dynamic_cast<Application *>(instance());

  if(app != nullptr)
    emit app->fontChanged();
  else
    qWarning() << Q_FUNC_INFO << "app is null";
}

void Application::handleException(const char *file, int line, const std::exception& e)
{
  qCritical() << "Caught exception in file" << file << "line" << line << "what" << e.what();

  if(showExceptionDialog)
    QMessageBox::critical(nullptr, QApplication::applicationName(),
                          tr("<b>Caught exception in file %1 line %2.</b><br/><br/>"
                             "<i>%3</i><br/><br/>"
                             "%4"
                             "<hr/>%5"
                               "<hr/>%6<br/>"
                               "<h3>Press OK to exit application.</h3>"
                             ).
                          arg(file).arg(line).
                          arg(e.what()).
                          arg(generalErrorMessage()).
                          arg(getEmailHtml()).
                          arg(getReportPathHtml()));

  std::abort();
}

void Application::handleException(const char *file, int line)
{
  qCritical() << "Caught unknown exception in file" << file << "line" << line;

  if(showExceptionDialog)
    QMessageBox::critical(nullptr, QApplication::applicationName(),
                          tr("<b>Caught unknown exception in file %1 line %2.</b><br/><br/>"
                             "%2"
                             "<hr/>%4"
                               "<hr/>%5<br/>"
                               "<h3>Press OK to exit application.</h3>"
                             ).
                          arg(file).arg(line).
                          arg(generalErrorMessage()).
                          arg(getEmailHtml()).
                          arg(getReportPathHtml()));

  std::abort();
}

void Application::addReportPath(const QString& header, const QStringList& paths)
{
  reportFiles.insert(header, paths);
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
      fileStr += tr("<a href=\"%1\">%2</a><br/>").arg(QUrl::fromLocalFile(path).toString()).arg(path);

    if(header != keys.last())
      fileStr.append(tr("<br/>"));
  }

  return fileStr;
}

} // namespace gui
} // namespace atools
