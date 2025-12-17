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

#include "gui/actiontool.h"

#include "gui/actiontextsaver.h"

#include "gui/actionstatesaver.h"

#include <QAction>
#include <QStringBuilder>

namespace atools {
namespace gui {

ActionTool::ActionTool(QList<QAction *> actions)
{
  textSaver = new ActionTextSaver(actions);
  stateSaver = new ActionStateSaver(actions);
}

ActionTool::~ActionTool()
{
  delete textSaver;
  delete stateSaver;
}

void ActionTool::setText(QAction *action, const QString& arg, const QString& suffix)
{
  if(action->text().contains("%1"))
    action->setText(action->text().arg(action->isEnabled() ? arg : QString()) % (action->isEnabled() ? QString() : suffix));
}

void ActionTool::setText(QAction *action, bool enabled, const QString& arg, const QString& suffix)
{
  action->setEnabled(enabled);

  if(action->text().contains("%1"))
    action->setText(action->text().arg(enabled ? arg : QString()) % (enabled ? QString() : suffix));
}

void ActionTool::finishTexts(const QString& objectText)
{
  for(QAction *action : std::as_const(stateSaver->actions))
  {
    if(action->text().contains("%1"))
      action->setText(action->text().arg(action->isEnabled() ? objectText : QString()));
  }
}

void ActionTool::enableAll()
{
  for(QAction *action : std::as_const(stateSaver->actions))
    action->setEnabled(true);
}

void ActionTool::disableAll()
{
  for(QAction *action : std::as_const(stateSaver->actions))
    action->setDisabled(true);

}

} // namespace gui
} // namespace atools
