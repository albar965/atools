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

#ifndef ATOOLS_GUI_ACTIONICONSAVER_H
#define ATOOLS_GUI_ACTIONICONSAVER_H

#include <QHash>

class QAction;
class QIcon;

namespace atools {
namespace gui {

class ActionTool;

/* Use this for context menus to save and
 * restore the action icons.*/
class ActionIconSaver
{
public:
  /*
   * Saves the icon of the given actions and restores it when the destructor is called
   */
  explicit ActionIconSaver(QList<QAction *> actions);
  ~ActionIconSaver();

  /* Do not allow copying */
  ActionIconSaver(const ActionIconSaver& other) = delete;
  ActionIconSaver& operator=(const ActionIconSaver& other) = delete;

private:
  friend class atools::gui::ActionTool;

  QHash<QAction *, QIcon> icons;
};

} // namespace gui
} // namespace atools

#endif // ATOOLS_GUI_ACTIONICONSAVER_H
