/*****************************************************************************
* Copyright 2015-2022 Alexander Barthel alex@littlenavmap.org
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

#include "exception.h"

#include <QAction>
#include <QDockWidget>
#include <QMainWindow>
#include <QDebug>
#include <QEvent>
#include <QMouseEvent>
#include <QStatusBar>
#include <QMenuBar>
#include <QToolBar>
#include <QMessageBox>
#include <QApplication>
#include <QScreen>
#include <QFile>

namespace atools {
namespace gui {

/* Do not restore these states */
const static Qt::WindowStates WINDOW_STATE_MASK =
  ~(Qt::WindowStates(Qt::WindowMinimized) | Qt::WindowStates(Qt::WindowActive));

/* Saves the main window states and states of all attached widgets like the status bars and the menu bar. */
struct MainWindowState
{
  explicit MainWindowState(QMainWindow *mainWindow)
  {
    fromWindow(mainWindow);
  }

  MainWindowState()
  {
  }

  /* Copy state to main window and all related widgets. Saved position to place fullscreen and maximized windows
   * is used if position is null. null = use current screen, otherwise use save screen position. */
  void toWindow(QMainWindow *mainWindow, const QPoint *position) const;

  /* Save state from main window and all related widgets */
  void fromWindow(const QMainWindow *mainWindow);

  /* Create an initial fullscreen configuration without docks and toobars depending on configuration */
  void initFullscreen(atools::gui::DockFlags flags);

  /* Clear all and set valid to false */
  void clear();

  /* false if default constructed or cleared */
  bool isValid() const
  {
    return valid;
  }

  QByteArray mainWindowState; // State from main window including toolbars and dock widgets
  QSize mainWindowSize;
  QPoint mainWindowPosition;
  Qt::WindowStates mainWindowStates = Qt::WindowNoState;

  bool statusBarVisible = true, valid = false, verbose = false;
};

QDebug operator<<(QDebug out, const MainWindowState& obj)
{
  QDebugStateSaver saver(out);
  out.noquote().nospace() << "MainWindowState["
                          << "size " << obj.mainWindowState.size()
                          << ", window size " << obj.mainWindowSize
                          << ", window position " << obj.mainWindowPosition
                          << ", window states " << obj.mainWindowStates
                          << ", statusbar " << obj.statusBarVisible
                          << ", valid " << obj.valid
                          << "]";
  return out;
}

void MainWindowState::toWindow(QMainWindow *mainWindow, const QPoint *position) const
{
  if(verbose)
    qDebug() << Q_FUNC_INFO << *this;

  if(!valid)
    qWarning() << Q_FUNC_INFO << "Calling on invalid state";

  if(mainWindowStates.testFlag(Qt::WindowMaximized) || mainWindowStates.testFlag(Qt::WindowFullScreen))
    // Move window to position before going fullscreen or maximized to catch right screen
    mainWindow->move(position == nullptr ? mainWindowPosition : *position);

  // Set normal, maximized or fullscreen
  mainWindow->setWindowState(mainWindowStates & WINDOW_STATE_MASK);

  if(!mainWindowStates.testFlag(Qt::WindowMaximized) && !mainWindowStates.testFlag(Qt::WindowFullScreen))
  {
    // Change size and position only if main window is not maximized or full screen
    if(mainWindowSize.isValid())
      mainWindow->resize(mainWindowSize);
    mainWindow->move(mainWindowPosition);
  }
  if(mainWindow->statusBar() != nullptr)
    mainWindow->statusBar()->setVisible(statusBarVisible);

  // Restores the state of this mainwindow's toolbars and dockwidgets. Also restores the corner settings too.
  // Has to be called after setting size to avoid unwanted widget resizing
  if(!mainWindowState.isEmpty())
    mainWindow->restoreState(mainWindowState);

  if(mainWindow->menuWidget() != nullptr)
    mainWindow->menuWidget()->setVisible(true); // Do not hide

  // Check if window if off screen ==================
  bool visible = false;
  qDebug() << Q_FUNC_INFO << "mainWindow->frameGeometry()" << mainWindow->frameGeometry();

  // Has to be visible for 20 pixels at least on one screen
  for(QScreen *screen : QGuiApplication::screens())
  {
    QRect geometry = screen->availableGeometry();
    QRect intersected = geometry.intersected(mainWindow->frameGeometry());
    if(intersected.width() > 20 && intersected.height() > 20)
    {
      qDebug() << Q_FUNC_INFO << "Visible on" << screen->name() << geometry;
      visible = true;
      break;
    }
  }

  if(!visible)
  {
    // Move back to primary top left plus offset
    QRect geometry = QApplication::primaryScreen()->availableGeometry();
    mainWindow->move(geometry.topLeft() + QPoint(20, 20));
    qDebug() << Q_FUNC_INFO << "Getting window back on screen" << QApplication::primaryScreen()->name() << geometry;
  }
}

void MainWindowState::fromWindow(const QMainWindow *mainWindow)
{
  clear();
  mainWindowState = mainWindow->saveState();
  mainWindowSize = mainWindow->size();
  mainWindowPosition = mainWindow->pos();
  mainWindowStates = mainWindow->windowState();
  statusBarVisible = mainWindow->statusBar()->isVisible();
  valid = true;

  if(verbose)
    qDebug() << Q_FUNC_INFO << *this;
}

void MainWindowState::initFullscreen(atools::gui::DockFlags flags)
{
  clear();

  mainWindowStates = flags.testFlag(MAXIMIZE) ? Qt::WindowMaximized : Qt::WindowFullScreen;
  statusBarVisible = !flags.testFlag(HIDE_STATUSBAR);
  valid = true;

  if(verbose)
    qDebug() << Q_FUNC_INFO << *this;
}

void MainWindowState::clear()
{
  mainWindowState.clear();
  mainWindowSize = QSize();
  mainWindowPosition = QPoint();
  mainWindowStates = Qt::WindowNoState;
  statusBarVisible = true;
  valid = false;
}

QDataStream& operator<<(QDataStream& out, const atools::gui::MainWindowState& state)
{
  bool menuVisible = true;
  out << state.valid << state.mainWindowState << state.mainWindowSize << state.mainWindowPosition
      << state.mainWindowStates << state.statusBarVisible << menuVisible;
  return out;
}

QDataStream& operator>>(QDataStream& in, atools::gui::MainWindowState& state)
{
  bool menuVisible;
  in >> state.valid >> state.mainWindowState >> state.mainWindowSize >> state.mainWindowPosition
  >> state.mainWindowStates >> state.statusBarVisible >> menuVisible;
  return in;
}

// ===================================================================================
class DockEventFilter :
  public QObject
{
public:
  DockEventFilter(QMainWindow *mainWindowParam)
    : mainWindow(mainWindowParam)
  {

  }

  bool autoRaiseWindow = false, autoRaiseMainWindow = false;

private:
  virtual bool eventFilter(QObject *object, QEvent *event) override;

  QMainWindow *mainWindow;

};

bool DockEventFilter::eventFilter(QObject *object, QEvent *event)
{
  if(event->type() == QEvent::Enter)
  {
    if(!mainWindow->isMinimized() && mainWindow->isVisible())
    {
      if(autoRaiseWindow)
      {
        // Raise normal dock widget ==============
        QDockWidget *widget = dynamic_cast<QDockWidget *>(object);
        if(widget != nullptr)
        {
          if(widget->isFloating())
          {
            widget->activateWindow();
            widget->raise();
          }
        }
        else
        {
          // Raise non-modal dialog widget ==============
          QDialog *dialog = dynamic_cast<QDialog *>(object);
          if(dialog != nullptr)
          {
            if(!dialog->isModal())
            {
              dialog->activateWindow();
              dialog->raise();
            }
          }
        }
      }

      if(autoRaiseMainWindow)
      {
        QMainWindow *win = dynamic_cast<QMainWindow *>(object);
        if(win != nullptr)
        {
          win->activateWindow();
          win->raise();
        }
      }
    }
  }

  return QObject::eventFilter(object, event);
}

// ===================================================================================
DockWidgetHandler::DockWidgetHandler(QMainWindow *parentMainWindow, const QList<QDockWidget *>& dockWidgetsParam,
                                     const QList<QToolBar *>& toolBarsParam, bool verboseLog)
  : QObject(parentMainWindow), mainWindow(parentMainWindow), dockWidgets(dockWidgetsParam), toolBars(toolBarsParam), verbose(verboseLog)
{
  dockEventFilter = new DockEventFilter(mainWindow);
  normalState = new MainWindowState;
  normalState->verbose = verbose;
  fullscreenState = new MainWindowState;
  fullscreenState->verbose = verbose;
}

DockWidgetHandler::~DockWidgetHandler()
{
  delete dockEventFilter;
  delete normalState;
  delete fullscreenState;
}

void DockWidgetHandler::dockVisibilityChanged(bool visible)
{
  if(verbose)
    qDebug() << Q_FUNC_INFO << "visible" << visible;

  if(visible)
  {
    QDockWidget *dockWidget = dynamic_cast<QDockWidget *>(sender());
    if(dockWidget != nullptr)
    {
      if(dockWidget->isFloating())
      {
        // Check if widget or its title bar are off screen and correct position if needed
        QPoint pos = dockWidget->pos();
        if(pos.y() < 0)
          pos.setY(10);
        if(pos.x() < 0)
          pos.setX(10);
        if(pos != dockWidget->pos())
        {
          qDebug() << Q_FUNC_INFO << "Correcting dock position for" << dockWidget->objectName()
                   << "from" << dockWidget->pos() << "to" << pos;
          dockWidget->move(pos);
        }
      }
    }
  }
}

void DockWidgetHandler::dockTopLevelChanged(bool topLevel)
{
  if(verbose)
    qDebug() << Q_FUNC_INFO;

  updateDockTabStatus();

  // Restore title bar state if widget is not floating
  QDockWidget *dockWidget = dynamic_cast<QDockWidget *>(sender());
  if(dockWidget != nullptr)
    setHideTitleBar(dockWidget, hideTitle && !topLevel);
}

void DockWidgetHandler::dockLocationChanged(Qt::DockWidgetArea area)
{
  if(verbose)
    qDebug() << Q_FUNC_INFO;

  Q_UNUSED(area)
  updateDockTabStatus();
}

void DockWidgetHandler::connectDockWidget(QDockWidget *dockWidget)
{
  updateDockTabStatus();
  connect(dockWidget->toggleViewAction(), &QAction::toggled, this, &DockWidgetHandler::dockViewToggled);
  connect(dockWidget, &QDockWidget::dockLocationChanged, this, &DockWidgetHandler::dockLocationChanged);
  connect(dockWidget, &QDockWidget::topLevelChanged, this, &DockWidgetHandler::dockTopLevelChanged);
  connect(dockWidget, &QDockWidget::visibilityChanged, this, &DockWidgetHandler::dockVisibilityChanged);
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
    for(QDockWidget *dock : dockWidgets)
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
  if(verbose)
    qDebug() << Q_FUNC_INFO;

  if(handleDockViews)
  {
    QAction *action = dynamic_cast<QAction *>(sender());
    if(action != nullptr)
    {
      bool checked = action->isChecked();
      for(QDockWidget *dock : dockWidgets)
      {
        if(action == dock->toggleViewAction())
          toggledDockWindow(dock, checked);
      }
    }
  }
}

void DockWidgetHandler::activateWindow(QDockWidget *dockWidget)
{
  if(verbose)
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

bool DockWidgetHandler::isAutoRaiseWindows() const
{
  return dockEventFilter->autoRaiseWindow;
}

void DockWidgetHandler::setAutoRaiseWindows(bool value)
{
  dockEventFilter->autoRaiseWindow = value;
}

bool DockWidgetHandler::isAutoRaiseMainWindow() const
{
  return dockEventFilter->autoRaiseMainWindow;
}

void DockWidgetHandler::setAutoRaiseMainWindow(bool value)
{
  dockEventFilter->autoRaiseMainWindow = value;
}

void DockWidgetHandler::setStayOnTopMain(bool value) const
{
  setStayOnTop(mainWindow, value);

  for(QDockWidget *dock : dockWidgets)
  {
    if(dock->isFloating())
      setStayOnTop(dock, value);
  }
}

bool DockWidgetHandler::isStayOnTopMain() const
{
  return isStayOnTop(mainWindow);
}

void DockWidgetHandler::setStayOnTop(QWidget *window, bool value) const
{
  if(window->windowFlags().testFlag(Qt::WindowStaysOnTopHint) != value)
  {
    bool visible = window->isVisible();

    window->setWindowFlag(Qt::WindowStaysOnTopHint, value);

    if(visible)
      // Need to reopen window since changing window flags closes window
      window->show();
  }
}

bool DockWidgetHandler::isStayOnTop(QWidget *window) const
{
  return window->windowFlags().testFlag(Qt::WindowStaysOnTopHint);
}

void DockWidgetHandler::setMovingAllowed(bool allow)
{
  if(features.isEmpty())
  {
    // Create backup
    for(QDockWidget *dock : dockWidgets)
      features.append(dock->features());
  }

  if(allow)
  {
    // Restore backup
    for(int i = 0; i < dockWidgets.size(); i++)
    {
      dockWidgets[i]->setFeatures(features.value(i, QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable |
                                                 QDockWidget::DockWidgetFloatable));
      if(verbose)
        qDebug() << Q_FUNC_INFO << dockWidgets[i]->objectName() << "after" << dockWidgets[i]->features();
    }
  }
  else
  {
    // Forbid moving for all widgets
    for(QDockWidget *dock : dockWidgets)
    {
      QDockWidget::DockWidgetFeatures f = dock->features();
      dock->setFeatures(allow ? (f | QDockWidget::DockWidgetMovable) : (f & ~QDockWidget::DockWidgetMovable));
      if(verbose)
        qDebug() << Q_FUNC_INFO << dock->objectName() << "before" << f << "after" << dock->features();
    }
  }
}

void DockWidgetHandler::setDockingAllowed(bool allow)
{
  if(allowedAreas.isEmpty())
  {
    // Create backup
    for(QDockWidget *dock : dockWidgets)
      allowedAreas.append(dock->allowedAreas());
  }

  if(allow)
  {
    // Restore backup
    for(int i = 0; i < dockWidgets.size(); i++)
    {
      dockWidgets[i]->setAllowedAreas(allowedAreas.value(i, Qt::AllDockWidgetAreas));
      if(verbose)
        qDebug() << Q_FUNC_INFO << dockWidgets[i]->objectName() << "after" << dockWidgets[i]->allowedAreas();
    }
  }
  else
  {
    // Forbid docking for all widgets
    for(QDockWidget *dock : dockWidgets)
    {
      Qt::DockWidgetAreas a = dock->allowedAreas();
      dock->setAllowedAreas(allow ? Qt::AllDockWidgetAreas : Qt::NoDockWidgetArea);
      if(verbose)
        qDebug() << Q_FUNC_INFO << dock->allowedAreas() << "before" << a << "after" << dock->allowedAreas();
    }
  }
}

void DockWidgetHandler::setHideTitleBar(bool hide)
{
  hideTitle = hide;

  for(QDockWidget *dock : dockWidgets)
    setHideTitleBar(dock, hide);
}

void DockWidgetHandler::setHideTitleBar(QDockWidget *dockWidget, bool hide)
{
  // is null if default title bar is used - i.e. it is visible
  QWidget *widget = dockWidget->titleBarWidget();

  if(hide)
  {
    // Hide if not floating and not already hidden
    if(!dockWidget->isFloating() && widget == nullptr)
      dockWidget->setTitleBarWidget(new QWidget(dockWidget));
  }
  else
  {
    // Show if not already default
    if(widget != nullptr)
    {
      // Setting bar to null regains ownership of widget
      widget->deleteLater();
      dockWidget->setTitleBarWidget(nullptr);
    }
  }
}

void DockWidgetHandler::setMovingAllowed(QDockWidget *dockWidget, bool allow)
{
  if(allow)
    dockWidget->setFeatures(dockWidget->features() | QDockWidget::DockWidgetMovable);
  else
    dockWidget->setFeatures(dockWidget->features() & ~QDockWidget::DockWidgetMovable);
}

void DockWidgetHandler::setDockingAllowed(QDockWidget *dockWidget, bool allow)
{
  dockWidget->setAllowedAreas(allow ? Qt::AllDockWidgetAreas : Qt::NoDockWidgetArea);
}

void DockWidgetHandler::raiseFloatingDockWidget(QDockWidget *dockWidget)
{
  if(dockWidget->isVisible() && dockWidget->isFloating())
    dockWidget->raise();
}

void DockWidgetHandler::connectDockWindows()
{
  for(QDockWidget *dock : dockWidgets)
    connectDockWidget(dock);
  mainWindow->installEventFilter(dockEventFilter);
}

void DockWidgetHandler::raiseWindows()
{
  for(QDockWidget *dock : dockWidgets)
    raiseFloatingDockWidget(dock);

  for(QDialog *dialog : dialogWidgets)
  {
    if(dialog->isVisible())
      dialog->raise();
  }
}

// ==========================================================================
// Fullscreen methods

void DockWidgetHandler::setFullScreenOn(atools::gui::DockFlags flags)
{
  if(!fullscreen)
  {
    if(verbose)
      qDebug() << Q_FUNC_INFO;

    // Copy window layout to state
    normalState->fromWindow(mainWindow);

    if(!fullscreenState->isValid())
    {
      // No saved fullscreen configuration yet - create a new one
      fullscreenState->initFullscreen(flags);

      if(flags.testFlag(HIDE_TOOLBARS))
      {
        for(QToolBar *toolBar : toolBars)
          toolBar->setVisible(false);
      }

      if(flags.testFlag(HIDE_DOCKS))
      {
        for(QDockWidget *dockWidget : dockWidgets)
          dockWidget->setVisible(false);
      }
    }

    // Main window to fullscreen - keep window on same screen
    fullscreenState->toWindow(mainWindow, &normalState->mainWindowPosition);

    fullscreen = true;
    delayedFullscreen = false;
  }
  else
    qWarning() << Q_FUNC_INFO << "Already fullscreen";
}

void DockWidgetHandler::setFullScreenOff()
{
  if(fullscreen)
  {
    if(verbose)
      qDebug() << Q_FUNC_INFO;

    // Save full screen layout
    fullscreenState->fromWindow(mainWindow);

    // Assign normal state to window and keep window on same screen
    normalState->toWindow(mainWindow, &fullscreenState->mainWindowPosition);

    fullscreen = false;
    delayedFullscreen = false;
  }
  else
    qWarning() << Q_FUNC_INFO << "Already no fullscreen";
}

QByteArray DockWidgetHandler::saveState()
{
  // Save current state - other state was saved when switching fs/normal
  if(fullscreen)
    fullscreenState->fromWindow(mainWindow);
  else
    normalState->fromWindow(mainWindow);

  qDebug() << Q_FUNC_INFO << "normalState" << *normalState;
  qDebug() << Q_FUNC_INFO << "fullscreenState" << *fullscreenState;

  // Save states for each mode and also fullscreen status
  QByteArray data;
  QDataStream stream(&data, QIODevice::WriteOnly);
  stream << fullscreen << *normalState << *fullscreenState;
  return data;
}

void DockWidgetHandler::restoreState(QByteArray data)
{
  QDataStream stream(&data, QIODevice::ReadOnly);
  stream >> fullscreen >> *normalState >> *fullscreenState;
  delayedFullscreen = false;

  qDebug() << Q_FUNC_INFO << "normalState" << *normalState;
  qDebug() << Q_FUNC_INFO << "fullscreenState" << *fullscreenState;
}

void DockWidgetHandler::currentStateToWindow()
{
  if(verbose)
    qDebug() << Q_FUNC_INFO;

  if(fullscreen)
    fullscreenState->toWindow(mainWindow, nullptr);
  else
    normalState->toWindow(mainWindow, nullptr);
}

void DockWidgetHandler::normalStateToWindow()
{
  normalState->toWindow(mainWindow, nullptr);
  delayedFullscreen = fullscreen; // Set flag to allow switch to fullscreen later after showing windows
  fullscreen = false;
}

void DockWidgetHandler::fullscreenStateToWindow()
{
  fullscreenState->toWindow(mainWindow, nullptr);
  fullscreen = true;
  delayedFullscreen = false;
}

void DockWidgetHandler::resetWindowState(const QSize& size, const QString& filename)
{
  QFile file(filename);
  if(file.open(QIODevice::ReadOnly))
  {
    QByteArray bytes = file.readAll();

    if(!bytes.isEmpty())
    {
      qDebug() << Q_FUNC_INFO;

      // Reset also ends fullscreen mode
      fullscreen = false;

      // End maximized and fullscreen state
      mainWindow->setWindowState(Qt::WindowNoState);

      // Move to origin and apply size
      mainWindow->move(QGuiApplication::primaryScreen()->availableGeometry().topLeft());
      mainWindow->resize(size);

      // Reload state now. This has to be done after resizing the window.
      mainWindow->restoreState(bytes);

      if(mainWindow->menuWidget() != nullptr)
        mainWindow->menuWidget()->setVisible(true); // Do not hide

      if(mainWindow->statusBar() != nullptr)
        mainWindow->statusBar()->setVisible(true);

      normalState->fromWindow(mainWindow);

      // Have to set status bar manually since window is not rendered yet an visible returns false
      normalState->statusBarVisible = true;

      fullscreenState->clear();
    }
    else
      throw atools::Exception(tr("Error reading \"%1\": %2").arg(filename).arg(file.errorString()));

    file.close();
  }
  else
    throw atools::Exception(tr("Error reading \"%1\": %2").arg(filename).arg(file.errorString()));
}

void DockWidgetHandler::saveWindowState(const QString& filename, bool allowUndockCentral)
{
  qDebug() << Q_FUNC_INFO << filename;

  QFile file(filename);
  if(file.open(QIODevice::WriteOnly))
  {
    // Copy current window status to slot
    if(fullscreen)
      fullscreenState->fromWindow(mainWindow);
    else
      normalState->fromWindow(mainWindow);

    // Save all to stream
    QDataStream stream(&file);
    stream << FILE_MAGIC_NUMBER << FILE_VERSION
           << allowUndockCentral << fullscreen
           << *normalState << *fullscreenState;

    if(file.error() != QFileDevice::NoError)
      throw atools::Exception(tr("Error writing \"%1\": %2").arg(filename).arg(file.errorString()));

    file.close();
  }
  else
    throw atools::Exception(tr("Error writing \"%1\": %2").arg(filename).arg(file.errorString()));
}

bool DockWidgetHandler::loadWindowState(const QString& filename, bool allowUndockCentral,
                                        const QString& allowUndockCentralErrorMessage)
{
  qDebug() << Q_FUNC_INFO << filename;
  QFile file(filename);
  if(file.open(QIODevice::ReadOnly))
  {
    QDataStream stream(&file);

    // Read and check magic number and version =================
    quint32 magicNumber;
    quint16 version;
    stream >> magicNumber >> version;

    if(magicNumber != FILE_MAGIC_NUMBER)
      throw atools::Exception(tr("Error reading \"%1\": Invalid magic number. Not a window layout file.").
                              arg(filename));
    if(version != FILE_VERSION)
      throw atools::Exception(tr("Error reading \"%1\": Invalid version. Incompatible window layout file.").
                              arg(filename));

    // Read all into temporary variables ===============
    bool fs, allowUndock = allowUndockCentral;
    MainWindowState normal, full;
    stream >> allowUndock >> fs >> normal >> full;

    if(file.error() != QFileDevice::NoError)
      throw atools::Exception("Error reading \"" + filename + "\": " + file.errorString());

    file.close();

    int retval = QMessageBox::Yes;
    if(allowUndock != allowUndockCentral)
      // A layout file can only be applied properly if the state of the central widget (normal or dock widget)
      // is the same - show warning
      retval = QMessageBox::question(mainWindow, QApplication::applicationName(),
                                     allowUndockCentralErrorMessage, QMessageBox::Yes | QMessageBox::Cancel);

    if(retval == QMessageBox::Yes)
    {
      // Copy temporary variables to fields
      // fullscreen = fs; // leave this up to the application
      *normalState = normal;
      *fullscreenState = full;

      if(verbose)
      {
        qDebug() << Q_FUNC_INFO << "normalState" << *normalState;
        qDebug() << Q_FUNC_INFO << "fullscreenState" << *fullscreenState;
      }

      return true;
    }
  }
  else
    throw atools::Exception(tr("Error reading \"%1\": %2").arg(filename).arg(file.errorString()));

  // nothing to apply
  return false;
}

bool DockWidgetHandler::isWindowLayoutFile(const QString& filename)
{
  qDebug() << Q_FUNC_INFO << filename;
  QFile file(filename);
  if(file.open(QIODevice::ReadOnly))
  {
    QDataStream stream(&file);

    // Read and check magic number and version =================
    quint32 magicNumber = 0;
    quint16 version = 0;
    stream >> magicNumber >> version;

    bool ok = magicNumber == FILE_MAGIC_NUMBER && version == FILE_VERSION && file.error() == QFileDevice::NoError;

    file.close();

    return ok;
  }
  return false;
}

void DockWidgetHandler::addDialogWidget(QDialog *dialogWidget)
{
  if(dialogWidget != nullptr)
  {
    dialogWidget->installEventFilter(dockEventFilter);
    dialogWidgets.append(dialogWidget);
    setStayOnTop(dialogWidget, isStayOnTopMain());
  }
}

void DockWidgetHandler::removeDialogWidget(QDialog *dialogWidget)
{
  if(dialogWidget != nullptr)
  {
    dialogWidgets.removeAll(dialogWidget);
    dialogWidget->removeEventFilter(dockEventFilter);
  }
}

void DockWidgetHandler::setStayOnTopDialogWidgets(bool value) const
{
  for(QDialog *dialog : dialogWidgets)
    setStayOnTop(dialog, value);
}

void DockWidgetHandler::closeAllDialogWidgets()
{
  for(QDialog *dialog : dialogWidgets)
  {
    qDebug() << Q_FUNC_INFO << "Dialog" << dialog->objectName();
    dialog->close();
  }

  dialogWidgets.clear();
}

void DockWidgetHandler::registerMetaTypes()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  qRegisterMetaTypeStreamOperators<atools::gui::MainWindowState>();
#endif
}

} // namespace gui
} // namespace atools

// Enable use in QVariant
Q_DECLARE_METATYPE(atools::gui::MainWindowState);
