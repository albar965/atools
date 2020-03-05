/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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

#include "gui/tools.h"
#include "gui/dialog.h"
#include "gui/helphandler.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QDebug>

#ifdef Q_OS_WIN32
#include <windows.h>
#endif

namespace atools {
namespace gui {

bool showInFileManager(const QString& filepath, QWidget *parent)
{
#ifdef Q_OS_WIN32
  QFileInfo fp(filepath);
  fp.makeAbsolute();

  // if(!QProcess::startDetached("explorer.exe", {"/select", QDir::toNativeSeparators(fp.filePath())},
  // QDir::homePath()))
  // QMessageBox::warning(mainWindow, QApplication::applicationName(), QString(
  // tr("Error starting explorer.exe with path \"%1\"")).
  // arg(query.queryItemValue("filepath")));

  if(fp.exists())
  {
    // Syntax is: explorer /select, "C:\Folder1\Folder2\file_to_select"
    // Dir separators MUST be win-style slashes

    // QProcess::startDetached() has an obscure bug. If the path has
    // no spaces and a comma(and maybe other special characters) it doesn't
    // get wrapped in quotes. So explorer.exe can't find the correct path and
    // displays the default one. If we wrap the path in quotes and pass it to
    // QProcess::startDetached() explorer.exe still shows the default path. In
    // this case QProcess::startDetached() probably puts its own quotes around ours.

    STARTUPINFO startupInfo;
    ::ZeroMemory(&startupInfo, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);

    PROCESS_INFORMATION processInfo;
    ::ZeroMemory(&processInfo, sizeof(processInfo));

    QString nativePath(QDir::toNativeSeparators(fp.canonicalFilePath()));
    QString cmd = QString("explorer /select,\"%1\"").arg(nativePath);
    qDebug() << Q_FUNC_INFO << "command" << cmd;
    LPWSTR lpCmd = new WCHAR[cmd.size() + 1];
    cmd.toWCharArray(lpCmd);
    lpCmd[cmd.size()] = 0;

    bool ret = ::CreateProcessW(NULL, lpCmd, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo);
    delete[] lpCmd;

    if(ret)
    {
      ::CloseHandle(processInfo.hProcess);
      ::CloseHandle(processInfo.hThread);
    }
    return true;
  }
  else
  {
    // If the item to select doesn't exist, try to open its parent
    QUrl url = QUrl::fromLocalFile(fp.path());
    if(!QDesktopServices::openUrl(url))
    {
      atools::gui::Dialog::warning(parent, QObject::tr("Error opening path \"%1\"").arg(filepath));
      return false;
    }
    else
      return true;
  }
#else
  QFileInfo fi(filepath);
  QUrl url = QUrl::fromLocalFile(fi.canonicalPath());
  qDebug() << Q_FUNC_INFO << "url" << url;

  if(!QDesktopServices::openUrl(url))
  {
    atools::gui::Dialog::warning(parent, QObject::tr("Error opening path \"%1\"").arg(filepath));
    return false;
  }
  else
    return true;

#endif
}

void anchorClicked(QWidget *parent, const QUrl& url)
{
  qDebug() << Q_FUNC_INFO << url;

  if(url.scheme() == "http" || url.scheme() == "https" || url.scheme() == "ftp")
    // Open a normal link from the userpoint description
    atools::gui::HelpHandler::openUrl(parent, url);
  else if(url.scheme() == "file")
  {
    if(url.isLocalFile())
    {
      QFileInfo info(url.toLocalFile());
      if(info.exists())
        // Open a file from the userpoint description
        atools::gui::HelpHandler::openUrl(parent, url);
      else
        atools::gui::Dialog::warning(parent, QObject::tr("File or directory \"%1\" does not exist.").
                                     arg(url.toDisplayString()));
    }
  }
}

} // namespace gui
} // namespace atools
