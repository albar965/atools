/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_HELPHANDLER_H
#define ATOOLS_HELPHANDLER_H

#include <QObject>

namespace atools {
namespace gui {

/*
 * Provides slots for the help menu to display about dialogs, etc.
 * Also has methods to open local or remote help files for given language.
 */
class HelpHandler
  : public QObject
{
  Q_OBJECT

public:
  explicit HelpHandler(QWidget *parent, const QString& aboutMessage, const QString& gitRevision);

  /*
   * Get a help file where the ${LANG} variable in filepath will replaced with the
   * system UI language.
   *
   * Falls back to English if indicator file is missing.
   *
   * This will consider region fallbacks in both directions like pt_BR -> pt or pt -> pt_BR
   */
  static QString getHelpFile(const QString& filepath, const QString& language);

  /*
   * Open a help URL where the ${LANG} variable in urlString will replaced with the
   * system UI language.
   *
   * Falls back to English if indicator file is missing.
   *
   * This will consider region fallbacks in both directions like pt_BR -> pt or pt -> pt_BR
   */
  static void openHelpUrlWeb(QWidget *parent, const QString& urlString, const QString& language);
  void openHelpUrlWeb(const QString& urlString, const QString& language);
  static QUrl getHelpUrlWeb(const QString& urlString, const QString& language);

  static void openHelpUrlFile(QWidget *parent, const QString& urlString, const QString& language);
  void openHelpUrlFile(const QString& urlString, const QString& language);

  /* Display about this application dialog */
  void about();

  /* Display about Qt dialog */
  void aboutQt();

private:
  /* Returns a valid help URL and also replaces the variable ${LANG} with one of the
   * supported langages */
  static QUrl getHelpUrlFile(QWidget *parent, const QString& urlString, const QString& language);
  QUrl getHelpUrlFile(const QString& urlString, const QString& language);

  QWidget *parentWidget;
  QString message, rev;

};

} // namespace gui
} // namespace atools

#endif // ATOOLS_HELPHANDLER_H
