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
#include <QRegularExpression>

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

void HelpHandler::openUrl(const QUrl& url)
{
  openUrl(parentWidget, url);
}

void HelpHandler::openUrl(QWidget *parent, const QUrl& url)
{
  qDebug() << Q_FUNC_INFO << "About to open URL" << url;

  if(!QDesktopServices::openUrl(url))
    QMessageBox::warning(parent, QApplication::applicationName(), QString(
                           tr("Error opening help URL <i>%1</i>")).arg(url.toDisplayString()));
}

void HelpHandler::openUrlWeb(const QString& url)
{
  openUrlWeb(parentWidget, url);
}

void HelpHandler::openUrlWeb(QWidget *parent, const QString& url)
{
  qDebug() << Q_FUNC_INFO << url;
  openUrl(parent, QUrl(url));
}

void HelpHandler::openFile(const QString& filepath)
{
  openFile(parentWidget, filepath);
}

void HelpHandler::openFile(QWidget *parent, const QString& filepath)
{
  qDebug() << Q_FUNC_INFO << filepath;

  if(QFile::exists(filepath))
    openUrl(parent, QUrl::fromLocalFile(QDir::toNativeSeparators(filepath)));
  else
    QMessageBox::warning(parent, QApplication::applicationName(), QString(
                           tr("Help file <i>%1</i> not found")).arg(filepath));
}

QUrl HelpHandler::getHelpUrlWeb(const QString& urlString, const QString& language, const QString& anchor)
{
  // Replace variable and create URL
  QUrl url = QUrl(atools::replaceVar(urlString, "LANG", language));

  if(!anchor.isEmpty())
    url.setFragment(atools::replaceVar(anchor, "LANG", language));

  return url;
}

QUrl HelpHandler::getHelpUrlFile(QWidget *parent, const QString& urlString, const QString& language)
{
  QUrl url;
  // Replace variable and create URL
  QString urlStr(atools::replaceVar(urlString, "LANG", language));

  // Do not use system separator since this uses URLs
  if(QFileInfo::exists(QCoreApplication::applicationDirPath() + "/" + urlStr))
    url = QUrl::fromLocalFile(QCoreApplication::applicationDirPath() + "/" + urlStr);
  else
    QMessageBox::warning(parent, QApplication::applicationName(), QString(
                           tr("Help file <i>%1</i> not found")).arg(urlStr));

  return url;
}

QString HelpHandler::getHelpFile(const QString& filepath, bool override)
{
  QString lang = override ? "en" : getLanguageFull();

  // Replace variable and create URL
  QString urlStr(atools::replaceVar(filepath, "LANG", lang));

  // Do not use system separator since this uses URLs
  if(QFileInfo::exists(QCoreApplication::applicationDirPath() + "/" + urlStr))
    // Full match with language and region if given
    return QCoreApplication::applicationDirPath() + "/" + urlStr;
  else
  {
    // Try a file without region
    lang = lang.section("_", 0, 0);
    if(!lang.isEmpty())
    {
      urlStr = atools::replaceVar(filepath, "LANG", lang);
      if(QFileInfo::exists(QCoreApplication::applicationDirPath() + "/" + urlStr))
        return QCoreApplication::applicationDirPath() + "/" + urlStr;
    }

    // Try same language with any region by iterating over dir
    QDir dir(QCoreApplication::applicationDirPath() + "/" + QFileInfo(filepath).path());
    QString filter(atools::replaceVar(QFileInfo(filepath).fileName(), "LANG", lang + "_*"));
    QFileInfoList list = dir.entryInfoList({filter});

    if(!list.isEmpty())
      return list.first().filePath();

    // Fall back to plain English
    urlStr = atools::replaceVar(filepath, "LANG", "en");
    return QCoreApplication::applicationDirPath() + "/" + urlStr;
  }
}

QUrl HelpHandler::getHelpUrlFile(const QString& urlString, const QString& language)
{
  return getHelpUrlFile(parentWidget, urlString, language);
}

void HelpHandler::openHelpUrlWeb(const QString& urlString, const QString& language, const QString& anchor)
{
  openHelpUrlWeb(parentWidget, urlString, language, anchor);
}

void HelpHandler::openHelpUrlWeb(QWidget *parent, const QString& urlString, const QString& language,
                                 const QString& anchor)
{
  qDebug() << Q_FUNC_INFO << "About to open URL" << urlString << "languages" << language << "anchor" << anchor;

  QUrl url = getHelpUrlWeb(urlString, language, anchor);
  if(!url.isEmpty())
    openUrl(parent, url);
  else
    QMessageBox::warning(parent, QApplication::applicationName(), QString(
                           tr("URL is empty for \"%1\" and anchor \"%2\".").arg(urlString).arg(anchor)));
}

void HelpHandler::openHelpUrlFile(const QString& urlString, const QString& language)
{
  openHelpUrlFile(parentWidget, urlString, language);
}

void HelpHandler::openHelpUrlFile(QWidget *parent, const QString& urlString, const QString& language)
{
  qDebug() << Q_FUNC_INFO << "About to open URL" << urlString << "languages" << language;

  QUrl url = getHelpUrlFile(parent, urlString, language);
  if(!url.isEmpty())
    openUrl(parent, url);
  else
    QMessageBox::warning(parent, QApplication::applicationName(), QString(
                           tr("URL is empty for \"%1\".").arg(urlString)));
}

QString HelpHandler::getLanguage()
{
  QString lang = getLanguageFull();
  if(!lang.isEmpty())
    return lang.section(QRegularExpression("[_-]"), 0, 0);
  else
    return "en";
}

QString HelpHandler::getLanguageFull()
{
  QString overrideLang =
    atools::settings::Settings::instance().valueStr("MainWindow/HelpLanguage", QString());

  QString lang;

  if(overrideLang.isEmpty())
  {
    QStringList uiLanguages = QLocale().uiLanguages();

    if(!uiLanguages.isEmpty())
      lang = uiLanguages.first().replace("-", "_");
    else
      lang = "en";
  }
  else
    lang = overrideLang;

  return lang;
}

} // namespace gui
} // namespace atools
