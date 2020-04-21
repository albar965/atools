/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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

#include "gui/dockwidgethandler.h"

#include <QAction>
#include <QDockWidget>
#include <QMainWindow>

namespace atools {
namespace gui {

DockWidgetHandler::DockWidgetHandler(QMainWindow *parent, const QList<QDockWidget *>& dockWidgets)
  : mainWindow(parent), dockList(dockWidgets)
{

}

DockWidgetHandler::~DockWidgetHandler()
{

}

void DockWidgetHandler::dockTopLevelChanged(bool topLevel)
{
  Q_UNUSED(topLevel)
  updateDockTabStatus();
}

void DockWidgetHandler::dockLocationChanged(Qt::DockWidgetArea area)
{
  Q_UNUSED(area)
  updateDockTabStatus();
}

void DockWidgetHandler::connectDockWindow(QDockWidget *dockWidget)
{
  updateDockTabStatus();
  connect(dockWidget->toggleViewAction(), &QAction::toggled, this, &DockWidgetHandler::dockViewToggled);
  connect(dockWidget, &QDockWidget::dockLocationChanged, this, &DockWidgetHandler::dockLocationChanged);
  connect(dockWidget, &QDockWidget::topLevelChanged, this, &DockWidgetHandler::dockTopLevelChanged);
}

void DockWidgetHandler::toggledDockWindow(QDockWidget *dockWidget, bool checked)
{
  bool handle = handleDockViews;

  // Do not remember stacks triggered by signals
  handleDockViews = false;

  if(checked)
  {
    // Find a stack that contains the widget ==================
    auto it = std::find_if(dockStackList.begin(), dockStackList.end(),
                           [dockWidget](QList<QDockWidget *>& list)
        {
          return list.contains(dockWidget);
        });

    if(it != dockStackList.end())
    {
      // Found a stack now show all stack member widgets
      for(QDockWidget *dock : *it)
      {
        if(dock != dockWidget)
          dock->show();
      }
    }

    // Show the widget whose action fired
    dockWidget->show();
    dockWidget->activateWindow();
    dockWidget->raise();
  }
  else
  {
    // Even floating widgets can have tabified buddies - ignore floating
    if(!dockWidget->isFloating())
    {
      for(QDockWidget *dock : mainWindow->tabifiedDockWidgets(dockWidget))
      {
        if(!dock->isFloating())
          dock->close();
      }
    }
  }
  handleDockViews = handle;
}

void DockWidgetHandler::updateDockTabStatus()
{
  if(handleDockViews)
  {
    dockStackList.clear();
    for(QDockWidget *dock : dockList)
      updateDockTabStatus(dock);
  }
}

void DockWidgetHandler::updateDockTabStatus(QDockWidget *dockWidget)
{
  if(dockWidget->isFloating())
    return;

  QList<QDockWidget *> tabified = mainWindow->tabifiedDockWidgets(dockWidget);
  if(!tabified.isEmpty())
  {
    auto it = std::find_if(dockStackList.begin(), dockStackList.end(), [dockWidget](QList<QDockWidget *>& list) -> bool
        {
          return list.contains(dockWidget);
        });

    if(it == dockStackList.end())
    {
      auto rmIt = std::remove_if(tabified.begin(), tabified.end(), [](QDockWidget *dock) -> bool
          {
            return dock->isFloating();
          });
      if(rmIt != tabified.end())
        tabified.erase(rmIt, tabified.end());

      if(!tabified.isEmpty())
      {
        tabified.append(dockWidget);
        dockStackList.append(tabified);
      }
    }
  }
}

void DockWidgetHandler::dockViewToggled()
{
  if(handleDockViews)
  {
    QAction *action = dynamic_cast<QAction *>(sender());
    if(action != nullptr)
    {
      bool checked = action->isChecked();
      for(QDockWidget *dock : dockList)
      {
        if(action == dock->toggleViewAction())
          toggledDockWindow(dock, checked);
      }
    }
  }
}

void DockWidgetHandler::activateWindow(QDockWidget *dockWidget)
{
  dockWidget->show();
  dockWidget->activateWindow();
  dockWidget->raise();
}

void DockWidgetHandler::setHandleDockViews(bool value)
{
  handleDockViews = value;
  updateDockTabStatus();
}

void DockWidgetHandler::raiseFloatingWindow(QDockWidget *dockWidget)
{
  if(dockWidget->isVisible() && dockWidget->isFloating())
    dockWidget->raise();
}

void DockWidgetHandler::connectDockWindows()
{
  for(QDockWidget *dock : dockList)
    connectDockWindow(dock);
}

void DockWidgetHandler::raiseFloatingWindows()
{
  for(QDockWidget *dock : dockList)
    raiseFloatingWindow(dock);
}

} // namespace gui
} // namespace atools
