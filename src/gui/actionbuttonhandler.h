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

#ifndef ATOOLS_ACTIONBUTTONHANDLER_H
#define ATOOLS_ACTIONBUTTONHANDLER_H

#include <QHash>
#include <QObject>

class QAction;

namespace atools {
namespace gui {

/*
 * ZHandler takes care of none/all/selection actions in toolbar buttons.
 *
 * A press on all/none will show all or no features and a second press on all/none will revert to the selection.
 * There is no need to connect actions directly. Use this handlers signals instead.
 *
 * The handler does not take ownership of actions. All actions have to be checkable.
 */
class ActionButtonHandler :
  public QObject
{
  Q_OBJECT

public:
  explicit ActionButtonHandler(QObject *parent);
  virtual ~ActionButtonHandler() override;

  /* Set action which will toggle between all and selected features. */
  void setAllAction(QAction *action);

  /* Set action which will toggle between no and selected features. */
  void setNoneAction(QAction *action);

  /* Add any action which can be checked or unchecked by all and none actions */
  void addOtherAction(QAction *action);

signals:
  /* Sent when all is triggered. Other actions will be selected accordingly using blocked signals */
  void actionAllTriggered(QAction *action);

  /* Sent when none is triggered. Other actions will be selected accordingly using blocked signals */
  void actionNoneTriggered(QAction *action);

  /* Sent when any but none or all action is triggered */
  void actionOtherTriggered(QAction *action);

private:
  void noneTriggered();
  void allTriggered();
  void otherTriggered();

  /* Revert action checkstate to saved values */
  void toggleSavedStateWithActions();

  /* Save state of actions and set state to actionChecked */
  void saveAndSetActionState(bool checkAction);

  QAction *actionNone, *actionAll;

  /* All other actions with their states saved as value before pressing none/all.
   * Value is used to restore state when pressing none/all a second time. */
  QHash<QAction *, bool> actionsOther;

  enum
  {
    INVALID,
    NONE, /* None was last pressed */
    ALL, /* None was last time pressed */
    ACTION /* Other actions was last pressed */
  } lastTriggered = INVALID;
};

} // namespace gui
} // namespace atools

#endif // ATOOLS_ACTIONBUTTONHANDLER_H
