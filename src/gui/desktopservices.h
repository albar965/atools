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

#ifndef ATOOLS_DESKTOPSERVICES_H
#define ATOOLS_DESKTOPSERVICES_H

#include <QUrl>
#include <QCoreApplication>

class QObject;
class QWidget;
class QString;

namespace atools {
namespace gui {

/*
 * Provides a more flexible replacement of QDesktopServices.
 * Allows to select files in file manager (macOS and Windows).
 *
 * Shows error message dialog boxes.
 */
class DesktopServices
{
  Q_DECLARE_TR_FUNCTIONS(DesktopServices)

public:
  DesktopServices(QWidget *parentWidgetParam);

  /* Open a file in the default application or open the path and select the file in the file manager. */
  static void openFile(QWidget *parent, QString path, bool showInFileManager = false);

  void openFile(const QString& path, bool showInFileManager = false) const
  {
    openFile(parentWidget, path, showInFileManager);
  }

  /* Opens an URL in the default web browser.
   * Uses openFile() for all schemes of "file:///" */
  static void openUrl(QWidget *parent, const QUrl& url, bool showInFileManager = false);

  void openUrl(const QUrl& url, bool showInFileManager = false) const
  {
    openUrl(parentWidget, url, showInFileManager);
  }

private:
  static QString clean(QString pathOrUrl);
  static void runProcess(QWidget *parent, const QString& program, const QStringList& arguments);

  QWidget *parentWidget;
};

} // namespace gui
} // namespace atools

#endif // ATOOLS_DESKTOPSERVICES_H
