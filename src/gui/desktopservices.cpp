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

#include "gui/desktopservices.h"

#include "atools.h"
#include "gui/dialog.h"

#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QProcess>

namespace atools {
namespace gui {

DesktopServices::DesktopServices(QWidget *parentWidgetParam)
  : parentWidget(parentWidgetParam)
{
}

void DesktopServices::openFile(QWidget *parent, QString path, bool showInFileManager)
{
  qDebug() << Q_FUNC_INFO << path << "showInFileManager" << showInFileManager;

  path = clean(path);

  QFileInfo fileinfo(path);
  if(fileinfo.exists())
  {
    // Start process if not empty
    QString program;
    QStringList arguments;

    if(showInFileManager)
    {
      // Get file managers and parameters to open and select file or dir
#if defined(Q_OS_WIN32)

      program = "explorer.exe";
      arguments << "/select," << atools::nativeCleanPath(path);

#elif defined(Q_OS_MACOS)

      // tell application "Finder" to (reveal POSIX file "FILENAME") activate
      if(QFile::exists("/usr/bin/osascript"))
      {
        program = "/usr/bin/osascript";
        arguments << "-e" << QString("tell application \"Finder\" to (reveal POSIX file \"%1\") activate").
          arg(atools::nativeCleanPath(path));
      }

#elif defined(Q_OS_LINUX)

      if(QFile::exists("/usr/bin/gdbus"))
      {
        // Use dbus to select file on linux
        // $XDG_CURRENT_DESKTOP https://wiki.archlinux.org/title/Xdg-utils
        // gdbus call --session --dest org.freedesktop.FileManager1 --object-path /org/freedesktop/FileManager1
        // --method org.freedesktop.FileManager1.ShowItems "['file://$FILE']" ""
        program = "/usr/bin/gdbus";
        arguments << "call" << "--session" << "--dest" << "org.freedesktop.FileManager1" << "--object-path" <<
          "/org/freedesktop/FileManager1" << "--method" << "org.freedesktop.FileManager1.ShowItems"
                  << QString("['file://%1']").arg(atools::nativeCleanPath(path)) << "\"\"";
      }
#endif
    }
    else
    {
#if defined(Q_OS_LINUX)
      // Use dbus to show file on linux
      if(QFile::exists("/usr/bin/gdbus") && fileinfo.isDir())
      {
        program = "/usr/bin/gdbus";
        arguments << "call" << "--session" << "--dest" << "org.freedesktop.FileManager1" << "--object-path" <<
          "/org/freedesktop/FileManager1" << "--method" << "org.freedesktop.FileManager1.ShowFolders"
                  << QString("['file://%1']").arg(atools::nativeCleanPath(path)) << "\"\"";
      }

      if(QFile::exists("/usr/bin/xdg-open") && fileinfo.isFile())
      {
        program = "/usr/bin/xdg-open";
        arguments << atools::nativeCleanPath(path);
      }
#endif
    }

    if(!program.isEmpty())
      // Start process - otherwise fall back to QDesktopServices
      runProcess(parent, program, arguments);
    else
    {
      if(!QDesktopServices::openUrl(QUrl::fromLocalFile(path)))
        atools::gui::Dialog::warning(parent, tr("Error opening path \"%1\".").arg(path));
    }
  }
  else
    atools::gui::Dialog::warning(parent, tr("File does not exist \"%1\".").arg(path));
}

void DesktopServices::openUrl(QWidget *parent, const QUrl& url, bool showInFileManager)
{
  qDebug() << Q_FUNC_INFO << url << "showInFileManager" << showInFileManager;

  if(url.isLocalFile())
    openFile(parent, atools::nativeCleanPath(url.toLocalFile()), showInFileManager);
  else
  {
    if(!QDesktopServices::openUrl(url))
      atools::gui::Dialog::warning(parent, tr("Error opening URL \"%1\".").arg(url.toDisplayString()));
  }
}

void DesktopServices::runProcess(QWidget *parent, const QString& program, const QStringList& arguments)
{
  QStringList env = QProcess::systemEnvironment();
#if defined(DEBUG_OPEN_FILE) && defined(Q_OS_LINUX)
  // Workaround for a KDE bug which does not open files from the build folder
  env.erase(std::remove_if(env.begin(), env.end(), [](const QString& str) {
        return str.startsWith("LD_LIBRARY_PATH=");
      }), env.end());
#endif

  qDebug() << Q_FUNC_INFO << "running" << program << arguments;

  QProcess process;
  process.setEnvironment(env);
  process.setProgram(program);
  process.setArguments(arguments);
  if(!process.startDetached())
    atools::gui::Dialog::warning(parent, tr("Error running program \"%1\" with arguments %2.").
                                 arg(program).arg(atools::strJoin("\"", arguments, "\"", "\"", "\"")));
}

QString DesktopServices::clean(QString pathOrUrl)
{
  // Windows
  if(pathOrUrl.startsWith("file:///"))
    pathOrUrl.remove(0, 8);

  // Unix
  if(pathOrUrl.startsWith("file://"))
    pathOrUrl.remove(0, 7);

  return atools::nativeCleanPath(pathOrUrl);
}

} // namespace gui
} // namespace atools
