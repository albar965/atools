/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

#include "atools.h"
#include "gui/application.h"
#include "gui/desktopservices.h"
#include "gui/dialog.h"

#include <QDir>
#include <QFileInfo>
#include <QSslSocket>
#include <QUrl>

namespace atools {
namespace gui {

HelpHandler::HelpHandler(QWidget *parent, const QString& aboutMessage, const QString& gitRevision)
  : QObject(parent), parentWidget(parent), message(aboutMessage), rev(gitRevision)
{

}

HelpHandler::~HelpHandler()
{

}

void HelpHandler::about()
{
  QString sslTxt;

  if(QSslSocket::supportsSsl())
  {
    if(QSslSocket::sslLibraryBuildVersionString() == QSslSocket::sslLibraryVersionString())
      sslTxt = tr("<p>%1 (build and library)</p>").arg(QSslSocket::sslLibraryBuildVersionString());
    else
      sslTxt = tr("<p>%1 (build)<br/>%2 (library)</p>").
               arg(QSslSocket::sslLibraryBuildVersionString()).arg(QSslSocket::sslLibraryVersionString());
  }

#if defined(WINARCH64)
  QString applicationVersion = QCoreApplication::applicationVersion() + tr(" 64-bit");
#elif defined(WINARCH32)
  QString applicationVersion = QCoreApplication::applicationVersion() + tr(" 32-bit");
#else
  QString applicationVersion = QCoreApplication::applicationVersion();
#endif

  QMessageBox::about(parentWidget,
                     tr("About %1").arg(QCoreApplication::applicationName()),
                     tr("<p><b>%1</b></p>%2<p><hr/>Version %3 (revision %4)</p>"
                          "<p>atools Version %5 (revision %6)</p>"
                            "%7"
                            "<hr/>%8"
                              "<hr/>%9<br/>").
                     arg(QCoreApplication::applicationName()).
                     arg(message).
                     arg(applicationVersion).
                     arg(rev).
                     arg(atools::version()).
                     arg(atools::gitRevision()).
                     arg(sslTxt).
                     arg(atools::gui::Application::getContactHtml()).
                     arg(atools::gui::Application::getReportPathHtml()));
}

void HelpHandler::aboutQt()
{
  QMessageBox::aboutQt(parentWidget, tr("About Qt"));
}

QUrl HelpHandler::getHelpUrlWeb(const QString& urlString, const QString& language)
{
  // Replace variable and create URL
  return QUrl(atools::replaceVar(urlString, "LANG", language));
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
    atools::gui::Dialog::warning(parent, tr("Help file \"%1\" not found").arg(urlStr));

  return url;
}

QString HelpHandler::getHelpFile(const QString& filepath, const QString& language)
{
  QString lang = language;

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
      return list.constFirst().filePath();

    // Fall back to plain English
    urlStr = atools::replaceVar(filepath, "LANG", "en");
    return QCoreApplication::applicationDirPath() + "/" + urlStr;
  }
}

QUrl HelpHandler::getHelpUrlFile(const QString& urlString, const QString& language)
{
  return getHelpUrlFile(parentWidget, urlString, language);
}

void HelpHandler::openHelpUrlWeb(const QString& urlString, const QString& language)
{
  openHelpUrlWeb(parentWidget, urlString, language);
}

void HelpHandler::openHelpUrlWeb(QWidget *parent, const QString& urlString, const QString& language)
{
  qDebug() << Q_FUNC_INFO << "About to open URL" << urlString << "languages" << language;

  QUrl url = getHelpUrlWeb(urlString, language);
  if(!url.isEmpty())
    DesktopServices::openUrl(parent, url);
  else
    atools::gui::Dialog::warning(parent, tr("URL is empty for \"%1\".").arg(urlString));
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
    DesktopServices::openUrl(parent, url);
  else
    atools::gui::Dialog::warning(parent, tr("URL is empty for \"%1\".").arg(urlString));
}

} // namespace gui
} // namespace atools
