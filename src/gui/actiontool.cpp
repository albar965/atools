/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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
#include "gui/actioniconsaver.h"

namespace atools {
namespace gui {

QString ActionTool::ellipsisStr;

ActionTool::ActionTool(QList<QAction *> actions)
{
  textSaver = new ActionTextSaver(actions);
  stateSaver = new ActionStateSaver(actions);
  iconSaver = new ActionIconSaver(actions);
}

ActionTool::~ActionTool()
{
  delete textSaver;
  delete stateSaver;
  delete iconSaver;
}

void ActionTool::initTranslateableTexts()
{
  ellipsisStr = tr(" ...", "Ending ellipsis for menu items");
}

void ActionTool::setText(QAction *action, bool enabled, const QString& arg, const QString& suffix)
{
  if(action->isEnabled() != enabled)
    action->setEnabled(enabled);

  if(enabled)
  {
    // No suffix added if enabled
    if(action->text().contains("%1"))
      action->setText(action->text().arg(arg));
  }
  else
  {
    // Disabled with added suffix before ellipsis
    if(action->text().contains("%1"))
      action->setText(addTextBeforeEllipsis(action->text().arg(arg), suffix));
    else
      action->setText(addTextBeforeEllipsis(action->text(), suffix));
  }
  action->setText(action->text().replace(QStringLiteral("  "), QStringLiteral(" ")));
}

void ActionTool::finishTexts(const QString& objectText)
{
  for(QAction *action : std::as_const(stateSaver->actions))
  {
    if(action->text().contains("%1"))
      action->setText(action->text().arg(action->isEnabled() ? objectText : QStringLiteral()));
    action->setText(action->text().replace(QStringLiteral("  "), QStringLiteral(" ")));
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

QString ActionTool::addTextBeforeEllipsis(const QString& str, const QString& suffix)
{
  if(hasEllipsis(str))
  {
    QString retval;
    retval = removeEllipsis(str);
    retval = retval % suffix;
    retval = addEllipsis(retval);
    return retval;
  }
  else
    return str % suffix;
}

} // namespace gui
} // namespace atools
