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

#include "application.h"

#include <QDebug>
#include <QMessageBox>
#include <QUrl>

namespace atools {
namespace gui {

QHash<QString, QStringList> Application::reportFiles;

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

void Application::handleException(const char *file, int line, const std::exception& e)
{
  qCritical() << "Caught exception in file" << file << "line" << line << "what" << e.what();
  QMessageBox::critical(nullptr, QApplication::applicationName(),
                        tr("Caught exception in file %1 line %2.<br/><br/>"
                           "<i>%3</i><br/><br/>"
                           "<b>If the problem persists or occurs during startup "
                             "delete all settings and database files of %4 and try again.</b><br/><br/>"
                             "<b>If you wish to report this error attach the log and configuration files "
                               "to your report and add all other available information.</b><br/><br/>"
                               "%5"
                               "<h3>Exiting now.</h3>"
                           ).
                        arg(file).arg(line).
                        arg(e.what()).
                        arg(QApplication::applicationName()).
                        arg(getReportPaths()));
  std::exit(1);
}

void Application::handleException(const char *file, int line)
{
  qCritical() << "Caught unknown exception in file" << file << "line" << line;
  QMessageBox::critical(nullptr, QApplication::applicationName(),
                        tr("Caught unknown exception in file %1 line %2.<br/><br/>"
                           "<b>If the problem persists or occurs during startup "
                             "delete all settings and database files of %3 and try again.</b><br/><br/>"
                             "%4"
                             "<h3>Exiting now.</h3>"
                           ).
                        arg(file).arg(line).
                        arg(QApplication::applicationName()).
                        arg(getReportPaths()));
  std::exit(1);
}

void Application::addReportPath(const QString& header, const QStringList& paths)
{
  reportFiles.insert(header, paths);
}

QString Application::getReportPaths()
{
  QString fileStr;
  for(QString header : reportFiles.keys())
  {
    fileStr.append(tr("%1<br/>").arg(header));
    const QStringList& paths = reportFiles.value(header);

    for(const QString& path : paths)
      fileStr += tr("<a href=\"%1\">%2</a><br/>").arg(QUrl::fromLocalFile(path).toString()).arg(path);
    fileStr.append(tr("<br/>"));
  }

  return fileStr;
}

} // namespace gui
} // namespace atools
