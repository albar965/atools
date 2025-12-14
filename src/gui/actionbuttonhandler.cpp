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

#include "gui/actionbuttonhandler.h"

#include <QAction>
#include <QDebug>

namespace atools {
namespace gui {

ActionButtonHandler::ActionButtonHandler(QObject *parent)
  : QObject{parent}
{

}

ActionButtonHandler::~ActionButtonHandler()
{

}

void ActionButtonHandler::setNoneAction(QAction *action)
{
  actionNone = action;
  connect(actionNone, &QAction::triggered, this, &ActionButtonHandler::noneTriggered);
}

void ActionButtonHandler::setAllAction(QAction *action)
{
  actionAll = action;
  connect(actionAll, &QAction::triggered, this, &ActionButtonHandler::allTriggered);
}

void ActionButtonHandler::addOtherAction(QAction *action)
{
  actionsOther.insert(action, action->isChecked());
  connect(action, &QAction::triggered, this, &ActionButtonHandler::otherTriggered);
}

void ActionButtonHandler::noneTriggered()
{
#ifdef DEBUG_INFORMATION
  qDebug() << Q_FUNC_INFO << "lastTriggered" << lastTriggered;
#endif

  if(lastTriggered == NONE)
    // Repeated press - restore state
    toggleSavedStateWithActions();
  else
    // Save state and uncheck all
    saveAndSetActionState(false);

  lastTriggered = NONE;

  emit actionNoneTriggered(actionNone);
}

void ActionButtonHandler::allTriggered()
{
#ifdef DEBUG_INFORMATION
  qDebug() << Q_FUNC_INFO << "lastTriggered" << lastTriggered;
#endif

  if(lastTriggered == ALL)
    // Repeated press - restore state
    toggleSavedStateWithActions();
  else
    // Save state and check all
    saveAndSetActionState(true);

  lastTriggered = ALL;

  emit actionAllTriggered(actionAll);
}

void ActionButtonHandler::otherTriggered()
{
#ifdef DEBUG_INFORMATION
  qDebug() << Q_FUNC_INFO << "lastTriggered" << lastTriggered;
#endif

  QAction *action = dynamic_cast<QAction *>(sender());
  if(action != nullptr)
    // Save state for restore
    actionsOther[action] = action->isChecked();

  lastTriggered = ACTION;

  emit actionOtherTriggered(action);
}

void ActionButtonHandler::toggleSavedStateWithActions()
{
#ifdef DEBUG_INFORMATION
  qDebug() << Q_FUNC_INFO;
#endif

  for(auto it = actionsOther.begin(); it != actionsOther.end(); ++it)
  {
    QAction *action = it.key();
    bool checked = action->isChecked();

    // Set new check state
    action->blockSignals(true);
    action->setChecked(it.value());
    action->blockSignals(false);

    // Save old check state
    it.value() = checked;
  }
}

void ActionButtonHandler::saveAndSetActionState(bool checkAction)
{
#ifdef DEBUG_INFORMATION
  qDebug() << Q_FUNC_INFO << "actionChecked" << checkAction;
#endif

  for(auto it = actionsOther.begin(); it != actionsOther.end(); ++it)
  {
    QAction *action = it.key();

    // Save check state
    it.value() = action->isChecked();

    // Set new state
    action->blockSignals(true);
    action->setChecked(checkAction);
    action->blockSignals(false);
  }
}

} // namespace gui
} // namespace atools
