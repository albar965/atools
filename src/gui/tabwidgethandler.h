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

#ifndef ATOOLS_TABWIDGETHANDLER_H
#define ATOOLS_TABWIDGETHANDLER_H

#include <QObject>
#include <QList>

class QTabWidget;
class QToolButton;
class QAction;

namespace atools {
namespace gui {

struct TabData
{
  QWidget *widget;
  int id;
};

/*
 * Enhances tab widgets with a right corner widget and a context menu above the tabs that allow to enable and
 * disable tabs. State of open/closed and reordered tabs can be saved and loaded to a settings file.
 *
 * This handler uses a concept of ids instead of indexes. Create an enum starting with 0 with  consecutive values for each tab and
 * use this for the id space. Actual tab indexes can differ from ids if tabs are closed or re-ordered.
 */
class TabWidgetHandler :
  public QObject
{
  Q_OBJECT

public:
  /* Creates corner button and all/none/reset actions */
  explicit TabWidgetHandler(QTabWidget *tabWidgetParam, const QList<QWidget *>& additionalWidgets,
                            const QIcon& icon, const QString& toolButtonTooltip);
  virtual ~TabWidgetHandler() override;

  TabWidgetHandler(const TabWidgetHandler& other) = delete;
  TabWidgetHandler& operator=(const TabWidgetHandler& other) = delete;

  /* Creates the menu entries for the tool button and the context menu. tabIdsParam contains the ids for
   * each tab and has to correspond to the actual tabs contained by the tab widget.
   * tabIdsParam.size() has to be equal to tabWidgetParam->count() set in the constructor */
  void init(const QList<int>& tabIdsParam, const QString& settingsPrefixParam);

  /* Save or restore open/close and order of tabs. Call init before. */
  void saveState() const;
  void restoreState();

  /* Id of currently open tab or -1 if none */
  int getCurrentTabId() const;

  template<typename TYPE>
  TYPE getCurrentTabId() const
  {
    return static_cast<TYPE>(getCurrentTabId());
  }

  /* Set current tab. Tab will be re-opened if was closed.
   *  left = true: left side of current otherwise to the right.*/
  void setCurrentTab(int id, bool left = false);

  /* Open tab in background if it was closed. left = true: left side of current otherwise to the right. */
  void openTab(int id, bool left = false);

  /* true if tab is not closed */
  bool isTabVisible(int id) const;

  /* Reset tab order to default (as defined in tabIdsParam of method init. Closed tabs are reopened and the first tab
   * is set to current. */
  void reset();

  /* Get index in tab widget for id or -1 if tab is not contained by widget. */
  int getIndexForId(int id) const;

  /* True if tab layout is locked and close buttons are hidden */
  bool isLocked() const;

  /* Changes tab bar attributes for macOS style */
  void styleChanged();

signals:
  /* Current tab has changed */
  void  tabChanged(int id);

  /* A tab was closed */
  void  tabClosed(int id);

  /* A tab was opened */
  void  tabOpened(int id);

private:
  /* Removes actions from menu button before running init */
  void clear();

  void resetInternal();

  /* Blocks signals and removes all tabs from widget */
  void clearTabWidget();

  /* Signal from tab widget */
  void currentChanged();

  /* Signal from tab widget - Replaces close button with an empty dummy label if only one tab is left. */
  void tabCloseRequested(int index);

  /* Same as above but does not check lock status */
  void tabCloseRequestedInternal(int index);

  /* Any of the none, all, reset or other actions were triggered or toggled */
  void toolbarActionTriggered();

  /* Update widget/action state based on active tabs in the widget - also replaces close button on last tab */
  void updateWidgets();

  /* Update tab widget movable and closable properties */
  void updateTabs();

  /* Get id for tab or -1 if index is not valid */
  int idForIndex(int index) const;

  /* Get id from property widget */
  int idForWidget(QWidget *widget) const;

  /* Get a list of tabs that are not part of the widget - i.e. currently closed */
  const QList<int> missingTabIds() const;

  /* Add tab at the end or at index */
  int addTab(int id);
  int insertTab(int index, int id);

  /* Context menu for tab bar */
  void tableContextMenu(const QPoint& pos);

  /* Re-enables the close button after the widget had ony one tab */
  void fixSingleTab();

  /* Contains all information for tabs */
  struct Tab
  {
    Tab()
      : widget(nullptr)
    {
    }

    Tab(QWidget * tabParam, const QString& titleParam, const QString& tooltipParam, QAction * actionParam)
      : widget(tabParam), title(titleParam), tooltip(tooltipParam), action(actionParam)
    {
    }

    /* true if initialized and not default constructed */
    bool isValid() const
    {
      return widget != nullptr;
    }

    QWidget *widget; /* The tab widget. Contains id as property ID_PROPERTY */
    QString title, tooltip; /* Saved texts needed when adding tab */
    QAction *action; /* Action for tool button or menu. Has id in "data" field. */
  };

  /* Contains all (also closed) tabs. Index corresponds to tab id as given in tabIdsParam */
  QList<Tab> tabs;

  /* Various action to restore, close or reset all tabs */
  QAction *actionOpenAll = nullptr, *actionCloseExcept = nullptr, *actionResetLayout = nullptr,
          *actionLockLayout = nullptr;

  QTabWidget *tabWidget;
  QToolButton *toolButtonCorner = nullptr;

  /* Prefix used when saving settings */
  QString settingsPrefix;
};

} // namespace gui
} // namespace atools

#endif // ATOOLS_TABWIDGETHANDLER_H
