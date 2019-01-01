/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel albar965@mailbox.org
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

  /*
   * Get a help file where the ${LANG} variable in filepath will replaced with the
   * system UI language.
   *
   * Falls back to English if indicator file is missing.
   *
   * This will consider region fallbacks in both directions like pt_BR -> pt or pt -> pt_BR
   */
  static QString getHelpFile(const QString& filepath, bool override);

  /*
   * Open a help URL where the ${LANG} variable in urlString will replaced with the
   * system UI language.
   *
   * Falls back to English if indicator file is missing.
   *
   * This will consider region fallbacks in both directions like pt_BR -> pt or pt -> pt_BR
   */
  static void openHelpUrlWeb(QWidget *parent, const QString& urlString, const QString& language,
                             const QString& anchor = QString());
  void openHelpUrlWeb(const QString& urlString, const QString& language, const QString& anchor = QString());

  static void openHelpUrlFile(QWidget *parent, const QString& urlString, const QString& language);
  void openHelpUrlFile(const QString& urlString, const QString& language);

  /* Display about this application dialog */
  void about();

  /* Display about Qt dialog */
  void aboutQt();

  /* Open an URL in the default browser or application. If that fails show an error dialog */
  void openUrl(const QUrl& url);
  static void openUrl(QWidget *parent, const QUrl& url);

  /* Open a file in the default browser. If that fails show an error dialog */
  void openUrlWeb(const QString& url);
  static void openUrlWeb(QWidget *parent, const QString& url);

  /* Open a file using the default application. If that fails show an error dialog */
  void openFile(const QString& filepath);
  static void openFile(QWidget *parent, const QString& filepath);

  static QUrl getHelpUrlWeb(const QString& urlString, const QString& language, const QString& anchor = QString());

private:
  /* Returns a valid help URL and also replaces the variable ${LANG} with one of the
   * supported langages */
  static QUrl getHelpUrlFile(QWidget *parent, const QString& urlString, const QString& language);
  QUrl getHelpUrlFile(const QString& urlString, const QString& language);

  static QString getLanguage();
  static QString getLanguageFull();

  QWidget *parentWidget;
  QString message, rev;

};

} // namespace gui
} // namespace atools

#endif // ATOOLS_HELPMENUHANDLER_H
