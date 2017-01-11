/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_HELPMENUHANDLER_H
#define ATOOLS_HELPMENUHANDLER_H

#include <QObject>
#include <QVector>

class QWidget;

namespace atools {
namespace gui {

/*
 * Provides slots for the help menu to display about dialogs, etc.
 */
class HelpHandler :
  public QObject
{
  Q_OBJECT

public:
  HelpHandler(QWidget *parent, const QString& aboutMessage, const QString& gitRevision);
  virtual ~HelpHandler();

  /* Open the help HTML file in the default browser */
  void help();
  QUrl getHelpUrlForFile(const QString& dir, const QString& file, const QString& anchor = QString());
  static QUrl getHelpUrlForFile(QWidget *parent, const QString& dir, const QString& file,
                                const QString& anchor = QString());

  /* Returns a valid help URL and also replaces the variable ${LANG} with one of the
   * supported langages */
  static QUrl getHelpUrl(QWidget *parent, const QString& urlString, const QStringList& languages,
                         const QString& anchor = QString());
  QUrl getHelpUrl(const QString& urlString, const QStringList& languages, const QString& anchor = QString());

  static void openHelpUrl(QWidget *parent, const QString& urlString, const QStringList& languages,
                          const QString& anchor = QString());
  void openHelpUrl(const QString& urlString, const QStringList& languages, const QString& anchor = QString());

  /* Display about this application dialog */
  void about();

  /* Display about Qt dialog */
  void aboutQt();

  /* Open an URL in the default browser. If that fails show an error dialog */
  void openUrl(const QUrl& url);
  static void openUrl(QWidget *parent, const QUrl& url);

  /* fileTemplate is a regexp like "little-navmap-user-manual-([a-z]{2})\.pdf".
   * Capture 1 is the language. */
  static QStringList getInstalledLanguages(const QString& directory, const QString& fileTemplate);

private:
  static QString getLanguage();

  QWidget *parentWidget;
  QString message, rev;

};

} // namespace gui
} // namespace atools

#endif // ATOOLS_HELPMENUHANDLER_H
