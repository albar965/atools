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

#include "gui/helphandler.h"

#include "logging/logginghandler.h"
#include "settings/settings.h"
#include "atools.h"
#include "logging/loggingdefs.h"

#include <QMessageBox>
#include <QApplication>
#include <QUrl>
#include <QDir>
#include <QFileInfo>
#include <QDesktopServices>
#include <QSettings>

namespace atools {
namespace gui {

HelpHandler::HelpHandler(QWidget *parent, const QString& aboutMessage, const QString& gitRevision)
  : parentWidget(parent), message(aboutMessage), rev(gitRevision)
{

}

HelpHandler::~HelpHandler()
{

}

void HelpHandler::about()
{
  QStringList logs = atools::logging::LoggingHandler::getLogFiles();
  QString logStr;

  for(const QString& log : logs)
  {
    QUrl url(QUrl::fromLocalFile(log));

    logStr += QString("<a href=\"%1\">%2<a><br/>").arg(url.toString()).arg(log);
  }

  QMessageBox::about(parentWidget,
                     tr("About %1").arg(QApplication::applicationName()),
                     tr("<p><b>%1</b></p>%2<p><hr/>Version %3 (revision %4)</p>"
                          "<p>atools Version %5 (revision %6)</p>"
                            "<p><hr/>%7:</p><p><i>%8</i></p>").
                     arg(QApplication::applicationName()).
                     arg(message).
                     arg(QApplication::applicationVersion()).
                     arg(rev).
                     arg(atools::version()).
                     arg(atools::gitRevision()).
                     arg(logs.size() > 1 ? tr("Logfiles") : tr("Logfile")).
                     arg(logStr));
}

void HelpHandler::aboutQt()
{
  QMessageBox::aboutQt(parentWidget, tr("About Qt"));
}

void HelpHandler::help()
{
  QUrl url = getHelpUrl("help", "index.html");
  if(!url.isEmpty())
    openHelpUrl(url);
}

void HelpHandler::openHelpUrl(const QUrl& url)
{
  if(!QDesktopServices::openUrl(url))
    QMessageBox::warning(parentWidget, QApplication::applicationName(), QString(
                           tr("Error opening help URL <i>%1</i>")).arg(url.toDisplayString()));
}

QUrl HelpHandler::getHelpUrl(const QString& dir, const QString& file)
{
  QString overrideLang =
    atools::settings::Settings::instance()->value("MainWindow/HelpLanguage", QString()).toString();
  QString lang;

  if(overrideLang.isEmpty())
    lang = QLocale::system().bcp47Name().section('-', 0, 0);
  else
    lang = overrideLang;

  QString appPath = QFileInfo(QCoreApplication::applicationFilePath()).absolutePath();
  QString helpPrefix(appPath + QDir::separator() + dir + QDir::separator());
  QString helpSuffix(QString(QDir::separator()) + file);

  QString helpFile(helpPrefix + lang + helpSuffix),
  defaultHelpFile(helpPrefix + "en" + helpSuffix);

  QUrl url;
  if(QFileInfo::exists(helpFile))
    url = QUrl::fromLocalFile(helpFile);
  else if(QFileInfo::exists(defaultHelpFile))
    url = QUrl::fromLocalFile(defaultHelpFile);
  else
    QMessageBox::warning(parentWidget, QApplication::applicationName(), QString(
                           tr("Help file <i>%1</i> not found")).arg(QDir::toNativeSeparators(defaultHelpFile)));

  qDebug() << "Help file" << url;
  return url;
}

} // namespace gui
} // namespace atools
