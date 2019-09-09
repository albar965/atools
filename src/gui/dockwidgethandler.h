/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_DOCKWIDGETHANDLER_H
#define ATOOLS_DOCKWIDGETHANDLER_H

#include <QObject>

class QMainWindow;
class QDockWidget;

namespace atools {
namespace gui {

/*
 * Improves dock widget handling expecially if dock windows are stacked.
 * Closes whole stack if one dock is closed by the accopanied action.
 * On reopening opens full stack again.
 * Also takes care of raising dock window in a stack if activated.
 */
class DockWidgetHandler :
  public QObject
{
  Q_OBJECT

public:
  explicit DockWidgetHandler(QMainWindow *parent, const QList<QDockWidget *>& dockWidgets);
  virtual ~DockWidgetHandler() override;

  /* Raise all windows having floating state */
  void raiseFloatingWindows();
  static void raiseFloatingWindow(QDockWidget *dockWidget);

  /* Connect all internally */
  void connectDockWindows();

  /* Show, activate and raise a dock widget */
  static void activateWindow(QDockWidget *dockWidget);

  /* true if enabled and state is saved */
  bool isHandleDockViews() const
  {
    return handleDockViews;
  }

  void setHandleDockViews(bool value);

private:
  /* One dock view was toggled by the accompanied action */
  void dockViewToggled();
  void toggledDockWindow(QDockWidget *dockWidget, bool checked);

  /* Collect dockStackList with all stacked dock widgets */
  void updateDockTabStatus();
  void updateDockTabStatus(QDockWidget *dockWidget);

  /* Dock widget floating state changed */
  void dockTopLevelChanged(bool topLevel);

  /* Dock widget docking area changed */
  void dockLocationChanged(Qt::DockWidgetArea area);

  void connectDockWindow(QDockWidget *dockWidget);

  /* Stacks are only rememberd if this is true */
  bool handleDockViews = false;

  /* A list of stacked widgets */
  QList<QList<QDockWidget *> > dockStackList;
  QMainWindow *mainWindow;

  /* All handled docks */
  QList<QDockWidget *> dockList;
};

} // namespace gui
} // namespace atools

#endif // ATOOLS_DOCKWIDGETHANDLER_H
