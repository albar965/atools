/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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

#include "util/flags.h"

#include <QBitArray>
#include <QDockWidget>

class QMainWindow;
class QDockWidget;
class QMouseEvent;
class QToolBar;

namespace atools {
namespace gui {
class DockEventFilter;
struct MainWindowState;

/* Flags defining behavior when calling setFullScreenOn() */
enum DockFlag : quint32
{
  NONE = 0,
  HIDE_DOCKS = 1 << 0, /* Hide docks initially when fullscreen mode has no saved layout yet */
  HIDE_TOOLBARS = 1 << 1, /* As above for tool bars */
  HIDE_STATUSBAR = 1 << 2, /* As above for the status bar */
  MAXIMIZE = 1 << 4 /* Maximize window when going into fullscreen instead of using full screen mode */
};

ATOOLS_DECLARE_FLAGS_32(DockFlags, DockFlag)
ATOOLS_DECLARE_OPERATORS_FOR_FLAGS(atools::gui::DockFlags)

/*
 * Improves dock widget handling expecially if dock windows are stacked.
 * Closes whole stack if one dock is closed by the accopanied action.
 * On reopening opens full stack again.
 * Also takes care of raising dock window in a stack if activated.
 *
 * Contains more functions for fullscreen mode with maximized central widget
 * and saving main window states for each normal and fullscreen mode.
 */
class DockWidgetHandler :
  public QObject
{
  Q_OBJECT

public:
  /* Need to pass in all dock and toolbars since these cannot be accessed from the main window */
  explicit DockWidgetHandler(QMainWindow *parentMainWindow, const QList<QDockWidget *>& dockWidgetsParam,
                             const QList<QToolBar *>& toolBarsParam, bool verboseLog);
  virtual ~DockWidgetHandler() override;

  DockWidgetHandler(const DockWidgetHandler& other) = delete;
  DockWidgetHandler& operator=(const DockWidgetHandler& other) = delete;

  /* Raise all dock windows having floating state as well as non-modal dialogs */
  void raiseWindows();

  /* Raise on dock widget */
  static void raiseFloatingDockWidget(QDockWidget *dockWidget);

  /* Connect all internally */
  void connectDockWindows();

  /* Saves fullscreen and normal main window state to bytes. */
  QByteArray saveState() const;

  /* Restores fullscreen and normal state as well as saved window layouts but does not change main window. */
  void restoreState(QByteArray data);

  /* Assign current state normal or fullscreen to current window.
   * This covers main menu widget, toolbars and dock widgets.
   * Moves window to saved screen position.*/
  void currentStateToWindow();

  /* As above but assigns the normal state to the main window and sets the delayedFullscreen flag if
   * fullscreen mode was enabled. This allows to switch to fullscreen later to avoid a messed up layout.
   * Changes fullscreen to false.
   * Moves window to saved screen (position).*/
  void normalStateToWindow();

  /* As above but assigns the fullscreen state to the main window
   * Changes fullscreen to true.
   * Moves window to saved screen (position).*/
  void fullscreenStateToWindow();

  /* Show, activate and raise a dock widget */
  void activateWindow(QDockWidget *dockWidget);

  /* true if enabled and state is saved */
  bool isHandleDockViews() const
  {
    return handleDockViews;
  }

  /* Set to true to enable handler */
  void setHandleDockViews(bool value);

  /* Raise floating dock windows and non-modal dialogs when mouse enters window */
  bool isAutoRaiseWindows() const;
  void setAutoRaiseWindows(bool value);

  /* Raise main window when mouse enters window */
  bool isAutoRaiseMainWindow() const;
  void setAutoRaiseMainWindow(bool value);

  /* Keep window on top of others */
  bool isStayOnTop(QWidget *window) const;
  void setStayOnTop(QWidget *window, bool value) const;

  bool isStayOnTopMain() const;
  void setStayOnTopMain(bool value) const;

  /* Forbid docking if value is false. */
  void setDockingAllowed(bool allow);

  /* For one single widget not managed by this handler. */
  static void setDockingAllowed(QDockWidget *dockWidget, bool allow);

  /* Hide title bar for all docked windows. The title bar is restored if dock is floating */
  void setHideTitleBar(bool hide);

  /* Hide title bar for given dock window if not floating. */
  static void setHideTitleBar(QDockWidget *dockWidget, bool hide);

  /* Global status of title bar visibility */
  bool getHideTitle() const
  {
    return hideTitle;
  }

  /* Forbid moving by click in title bar if value is false. */
  void setMovingAllowed(bool allow);

  /* For one single widget not managed by this handler. */
  static void setMovingAllowed(QDockWidget *dockWidget, bool allow);

  /* Closes all docks, toolbars and menu bar depending on flags and sets the mainwindow to full screen.
   * Window will only be maximized depending on flags.
   * Configuration like docks is stored separately for fullscreen mode. */
  void setFullScreenOn(atools::gui::DockFlags flags);

  /* Ends fullscreen mode and restores all windows, toolbars, status bar and menu bar again */
  void setFullScreenOff();

  /* true is central widget is maximized */
  bool isFullScreen() const
  {
    return fullscreen;
  }

  /* Loads the main windows state from the given file, applies given size to the window and ends fullscreen mode.
   * Places window on the primary screen. Status bar always set visible. */
  void resetWindowState(const QSize& size, const QString& filename);

  /* Save normal and fullscreen window states to file.
   *  @param allowUndockCentral Has to be true if the central widget can be undocked
   *  @throws atools::Exception in case of IO error */
  void saveWindowState(const QString& filename, bool allowUndockCentral);

  /* Load normal and fullscreen window states as well as fullscreen status.
   * Loaded state is not applied to the main window.
   * @param allowUndockCentral Has to be true if the central widget can be undocked
   * @param allowUndockCentralErrorMessage Message to show if the saved state for
   * allowUndockCentral does not match the given state.
   * @return true if state was loaded and should be applied
   * @throws atools::Exception in case of IO error */
  bool loadWindowState(const QString& filename, bool allowUndockCentral, const QString& allowUndockCentralErrorMessage);

  /* True if files exists, is readable and magic number matches */
  static bool isWindowLayoutFile(const QString& filename);

  /* true after calling normalStateToWindow() while using fullscreen mode. Can be used to delay the switch to
   * fullscreen to avoid a distorted layout */
  bool isDelayedFullscreen() const
  {
    return delayedFullscreen;
  }

  /* Extra non-modal dialogs which are used for auto raise. */
  void addDialogWidget(QDialog *dialogWidget);
  void removeDialogWidget(QDialog *dialogWidget);

  void setStayOnTopDialogWidgets(bool value) const;

  /* Closes all widgets and removes them from the list afterwards */
  void closeAllDialogWidgets();

  /* Calls qRegisterMetaTypeStreamOperators for needed classes. */
  static void registerMetaTypes();

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

  void connectDockWidget(QDockWidget *dockWidget);

  /* Stacks are only rememberd if this is true */
  bool handleDockViews = false;

  /* A list of stacked widgets */
  QList<QList<QDockWidget *> > dockStackList;

  /* Main window to handle */
  QMainWindow *mainWindow;

  /* All handled docks and tool bars */
  QList<QDockWidget *> dockWidgets;
  QList<QDialog *> dialogWidgets;
  QList<QToolBar *> toolBars;

  /* Event filter to detect leave and enter events for auto raise */
  DockEventFilter *dockEventFilter;

  /* Backup of allowed areas used when calling setDockingAllowed() */
  QVector<Qt::DockWidgetAreas> allowedAreas;

  /* Backup of features used when calling setMovingAllowed() */
  QVector<QDockWidget::DockWidgetFeatures> features;

  /* Saved state of main window including dock widgets and toolbars */
  MainWindowState *normalState, *fullscreenState;

  bool fullscreen = false, delayedFullscreen = false, verbose = false, hideTitle = false;

  static Q_DECL_CONSTEXPR quint32 FILE_MAGIC_NUMBER = 0x2D6A9C2F;
  static Q_DECL_CONSTEXPR quint16 FILE_VERSION = 2;
};

} // namespace gui
} // namespace atools

#endif // ATOOLS_DOCKWIDGETHANDLER_H
