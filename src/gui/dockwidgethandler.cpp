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
#include <QDebug>
#include <QEvent>
#include <QMouseEvent>

namespace atools {
namespace gui {

class DockEventFilter :
  public QObject
{
public:
  DockEventFilter()
  {

  }

  bool autoRaiseDockWindow = false, autoRaiseMainWindow = false;

private:
  virtual bool eventFilter(QObject *object, QEvent *event) override;

};

bool DockEventFilter::eventFilter(QObject *object, QEvent *event)
{
  if(event->type() == QEvent::Enter)
  {
    if(autoRaiseDockWindow)
    {
      QDockWidget *widget = dynamic_cast<QDockWidget *>(object);
      if(widget != nullptr)
      {
        qDebug() << Q_FUNC_INFO << event->type() << widget->objectName();
        if(widget->isFloating())
        {
          widget->activateWindow();
          widget->raise();
        }
      }
    }

    if(autoRaiseMainWindow)
    {
      QMainWindow *mainWindow = dynamic_cast<QMainWindow *>(object);
      if(mainWindow != nullptr)
      {
        mainWindow->activateWindow();
        mainWindow->raise();
      }
    }
  }

  return QObject::eventFilter(object, event);
}

DockWidgetHandler::DockWidgetHandler(QMainWindow *parent, const QList<QDockWidget *>& dockWidgets)
  : mainWindow(parent), dockList(dockWidgets)
{
  dockEventFilter = new DockEventFilter();
}

DockWidgetHandler::~DockWidgetHandler()
{
  delete dockEventFilter;
}

void DockWidgetHandler::dockTopLevelChanged(bool topLevel)
{
  qDebug() << Q_FUNC_INFO;

  Q_UNUSED(topLevel)
  updateDockTabStatus();
}

void DockWidgetHandler::dockLocationChanged(Qt::DockWidgetArea area)
{
  qDebug() << Q_FUNC_INFO;

  Q_UNUSED(area)
  updateDockTabStatus();
}

void DockWidgetHandler::connectDockWindow(QDockWidget *dockWidget)
{
  updateDockTabStatus();
  connect(dockWidget->toggleViewAction(), &QAction::toggled, this, &DockWidgetHandler::dockViewToggled);
  connect(dockWidget, &QDockWidget::dockLocationChanged, this, &DockWidgetHandler::dockLocationChanged);
  connect(dockWidget, &QDockWidget::topLevelChanged, this, &DockWidgetHandler::dockTopLevelChanged);
  dockWidget->installEventFilter(dockEventFilter);
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
  qDebug() << Q_FUNC_INFO;
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
  qDebug() << Q_FUNC_INFO;
  dockWidget->show();
  dockWidget->activateWindow();
  dockWidget->raise();
}

void DockWidgetHandler::setHandleDockViews(bool value)
{
  handleDockViews = value;
  updateDockTabStatus();
}

bool DockWidgetHandler::isAutoRaiseDockWindows() const
{
  return dockEventFilter->autoRaiseDockWindow;
}

void DockWidgetHandler::setAutoRaiseDockWindows(bool value)
{
  dockEventFilter->autoRaiseDockWindow = value;
}

bool DockWidgetHandler::isAutoRaiseMainWindow() const
{
  return dockEventFilter->autoRaiseMainWindow;
}

void DockWidgetHandler::setAutoRaiseMainWindow(bool value)
{
  dockEventFilter->autoRaiseMainWindow = value;
}

void DockWidgetHandler::setDockingAllowed(bool value)
{
  if(allowedAreas.isEmpty())
  {
    // Create backup
    for(QDockWidget *dock : dockList)
      allowedAreas.append(dock->allowedAreas());
  }

  if(value)
  {
    // Restore backup
    for(int i = 0; i < dockList.size(); i++)
      dockList[i]->setAllowedAreas(allowedAreas.value(i));
  }
  else
  {
    // Forbid docking for all widgets
    for(QDockWidget *dock : dockList)
      dock->setAllowedAreas(value ? Qt::AllDockWidgetAreas : Qt::NoDockWidgetArea);
  }
}

void DockWidgetHandler::raiseFloatingWindow(QDockWidget *dockWidget)
{
  qDebug() << Q_FUNC_INFO;
  if(dockWidget->isVisible() && dockWidget->isFloating())
    dockWidget->raise();
}

void DockWidgetHandler::connectDockWindows()
{
  for(QDockWidget *dock : dockList)
    connectDockWindow(dock);
  mainWindow->installEventFilter(dockEventFilter);
}

void DockWidgetHandler::raiseFloatingWindows()
{
  for(QDockWidget *dock : dockList)
    raiseFloatingWindow(dock);
}

} // namespace gui
} // namespace atools
