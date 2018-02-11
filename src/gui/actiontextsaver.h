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

#ifndef ATOOLS_GUI_ACTIONTEXTSAVER_H
#define ATOOLS_GUI_ACTIONTEXTSAVER_H

#include <QHash>

class QAction;

namespace atools {
namespace gui {

/* Use this for context menus that have %1, etc. placeholders to save and restore the action texts.*/
class ActionTextSaver
{
public:
  /*
   * Saves the text of the given actions and restores it when the destructor is called
   */
  ActionTextSaver(QList<QAction *> actions);
  ~ActionTextSaver();

private:
  QHash<QAction *, QString> texts;
};

} // namespace gui
} // namespace atools

#endif // ATOOLS_GUI_ACTIONTEXTSAVER_H
