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
#include "gui/application.h"

#include <QDebug>
#include <QMessageBox>
#include <QApplication>
#include <QUrl>
#include <QDir>
#include <QFileInfo>
#include <QDesktopServices>

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
  QMessageBox::about(parentWidget,
                     tr("About %1").arg(QApplication::applicationName()),
                     tr("<p><b>%1</b></p>%2<p><hr/>Version %3 (revision %4)</p>"
                          "<p>atools Version %5 (revision %6)</p>"
                            "<hr/>%7"
                              "<hr/>%8<br/>").
                     arg(QApplication::applicationName()).
                     arg(message).
                     arg(QApplication::applicationVersion()).
                     arg(rev).
                     arg(atools::version()).
                     arg(atools::gitRevision()).
                     arg(atools::gui::Application::getEmailHtml()).
                     arg(atools::gui::Application::getReportPathHtml()));
}

void HelpHandler::aboutQt()
{
  QMessageBox::aboutQt(parentWidget, tr("About Qt"));
}

void HelpHandler::help()
{
  QUrl url = getHelpUrlForFile("help", "index.html");
  if(!url.isEmpty())
    openUrl(url);
}

void HelpHandler::openUrl(const QUrl& url)
{
  openUrl(parentWidget, url);
}

void HelpHandler::openUrl(QWidget *parent, const QUrl& url)
{
  if(!QDesktopServices::openUrl(url))
    QMessageBox::warning(parent, QApplication::applicationName(), QString(
                           tr("Error opening help URL <i>%1</i>")).arg(url.toDisplayString()));
}

QUrl HelpHandler::getHelpUrlForFile(const QString& dir, const QString& file, const QString& anchor)
{
  return getHelpUrlForFile(parentWidget, dir, file, anchor);
}

QUrl HelpHandler::getHelpUrlForFile(QWidget *parent, const QString& dir, const QString& file,
                                    const QString& anchor)
{
  QString lang = getLanguage();

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
    QMessageBox::warning(parent, QApplication::applicationName(), QString(
                           tr("Help file <i>%1</i> not found")).arg(QDir::toNativeSeparators(defaultHelpFile)));

  url.setFragment(anchor);

  qDebug() << "Help file" << url;
  return url;
}

QUrl HelpHandler::getHelpUrl(QWidget *parent, const QString& urlString, const QStringList& languages,
                             const QString& anchor)
{
  QString lang = getLanguage();

  if(!languages.contains(lang))
    // Fallback to English
    lang = "en";

  // Replace variable and create URL
  QUrl url(atools::replaceVar(urlString, "LANG", lang));
  if(!anchor.isEmpty())
    url.setFragment(atools::replaceVar(anchor, "LANG", lang));

  if(url.isLocalFile() && !QFileInfo::exists(url.toLocalFile()))
    QMessageBox::warning(parent, QApplication::applicationName(), QString(
                           tr("Help file <i>%1</i> not found")).arg(QDir::toNativeSeparators(url.toLocalFile())));

  return url;
}

QUrl HelpHandler::getHelpUrl(const QString& urlString, const QStringList& languages, const QString& anchor)
{
  return getHelpUrl(parentWidget, urlString, languages, anchor);
}

void HelpHandler::openHelpUrl(const QString& urlString, const QStringList& languages, const QString& anchor)
{
  openHelpUrl(parentWidget, urlString, languages, anchor);
}

void HelpHandler::openHelpUrl(QWidget *parent, const QString& urlString, const QStringList& languages,
                              const QString& anchor)
{
  QUrl url = getHelpUrl(parent, urlString, languages, anchor);
  if(!url.isEmpty())
    openUrl(parent, url);
  else
    QMessageBox::warning(parent, QApplication::applicationName(), QString(
                           tr("URL is empty for \"%1\" and anchor \"%2\".").arg(urlString).arg(anchor)));
}

QString HelpHandler::getLanguage()
{
  QString overrideLang =
    atools::settings::Settings::instance().valueStr("MainWindow/HelpLanguage", QString());

  QString lang;

  if(overrideLang.isEmpty())
    lang = QLocale::system().bcp47Name().section('-', 0, 0);
  else
    lang = overrideLang;

  return lang;
}

} // namespace gui
} // namespace atools
