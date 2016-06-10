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
  QUrl getHelpUrl(const QString& dir, const QString& file);

  /* Display about this application dialog */
  void about();

  /* Display about Qt dialog */
  void aboutQt();

  void openHelpUrl(const QUrl& url);

  /* A link to the directory of this file will be appended to the dialog */
  void addDirFileLink(const QString& text, const QString& file);

private:
  QWidget *parentWidget;
  QString message, rev;

  QVector<std::pair<QString, QString> > fileLinks;

};

} // namespace gui
} // namespace atools

#endif // ATOOLS_HELPMENUHANDLER_H
