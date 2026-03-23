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

#include "gui/widgetzoomhandler.h"

#include "settings/settings.h"
#include "atools.h"

#include <QDebug>
#include <QAction>
#include <QFont>
#include <QTableView>
#include <QHeaderView>
#include <QTreeWidget>
#include <QApplication>

namespace atools {
namespace gui {

QSet<QObject *> WidgetZoomHandler::registeredWidgets;
QMutex WidgetZoomHandler::registeredWidgetsMutex;

using atools::settings::Settings;

void WidgetZoomHandler::init(QWidget *widgetParam, QAction *zoomInAction, QAction *zoomOutAction,
                             QAction *zoomDefaultAction, QString settingsKeyStr, double marginParm)
{
  Q_ASSERT(widgetParam != nullptr);

  margin = marginParm;
  widget = widgetParam;
  actionZoomIn = zoomInAction;
  actionZoomOut = zoomOutAction;
  actionZoomDefault = zoomDefaultAction;
  settingsKey = settingsKeyStr;

  initTableViewZoom();

  if(actionZoomIn != nullptr)
    connect(actionZoomIn, &QAction::triggered, this, &WidgetZoomHandler::zoomIn);

  if(actionZoomOut != nullptr)
    connect(actionZoomOut, &QAction::triggered, this, &WidgetZoomHandler::zoomOut);

  if(actionZoomDefault != nullptr)
    connect(actionZoomDefault, &QAction::triggered, this, &WidgetZoomHandler::zoomDefault);

  {
    QMutexLocker locker(&registeredWidgetsMutex);
    registeredWidgets.insert(widget);
  }

#ifdef DEBUG_INFORMATION_ZOOMHANDLER
  qDebug() << Q_FUNC_INFO << "Registered" << widget->objectName();
#endif
}

WidgetZoomHandler::~WidgetZoomHandler()
{
  if(actionZoomIn != nullptr)
    disconnect(actionZoomIn, &QAction::triggered, this, &WidgetZoomHandler::zoomIn);

  if(actionZoomOut != nullptr)
    disconnect(actionZoomOut, &QAction::triggered, this, &WidgetZoomHandler::zoomOut);

  if(actionZoomDefault != nullptr)
    disconnect(actionZoomDefault, &QAction::triggered, this, &WidgetZoomHandler::zoomDefault);

  {
    QMutexLocker locker(&registeredWidgetsMutex);
    registeredWidgets.remove(widget);
  }
#ifdef DEBUG_INFORMATION_ZOOMHANDLER
  qDebug() << Q_FUNC_INFO << "Unregistered" << widget->objectName();
#endif
}

void WidgetZoomHandler::setTableViewFontSize(double pointSize)
{
  QFont widgetFont(QApplication::font());
  widgetFont.setPointSizeF(pointSize);

  double fontHeight = QFontMetricsF(widgetFont).height();

#ifdef DEBUG_INFORMATION_ZOOMHANDLER
  qDebug() << Q_FUNC_INFO << widget->objectName() << "pointSize" << pointSize << "new font height" << fontHeight
           << "point size" << widgetFont.pointSizeF();
#endif

  // Set font for whole widget ====
  widget->setFont(widgetFont);

  QTableView *tableView = dynamic_cast<QTableView *>(widget);
  if(tableView != nullptr)
  {
    // Adjust the cell height - default is too big
    int size = atools::roundToInt(fontHeight + sectionToFontSize + margin * 2.);

#ifdef DEBUG_INFORMATION_ZOOMHANDLER
    qDebug() << Q_FUNC_INFO << widget->objectName() << "table view vertical header sizes" << size;
#endif

    tableView->verticalHeader()->setMinimumSectionSize(size);
    tableView->verticalHeader()->setDefaultSectionSize(size);
    tableView->verticalHeader()->setFont(widgetFont);
    tableView->horizontalHeader()->setFont(widgetFont);
  }
  else
  {
    // Adjust font for all items separately since widget->setFont() does not affect cells if font is set separately for each cell
    QTreeWidget *treeView = dynamic_cast<QTreeWidget *>(widget);
    if(treeView != nullptr)
    {
      // Do not use recursion but rather a stack
      QList<QTreeWidgetItem *> itemStack;
      const QTreeWidgetItem *root = treeView->invisibleRootItem();

      // Add all root children
      for(int i = 0; i < root->childCount(); ++i)
      {
        if(root->child(i) != nullptr)
          itemStack.append(root->child(i));
      }

      QList<QTreeWidgetItem *> items;
      while(!itemStack.isEmpty())
      {
        // Remove first from stack
        QTreeWidgetItem *item = itemStack.takeFirst();
        // Change font size for all columns
        for(int i = 0; i < item->columnCount(); i++)
        {
          // Set global widget font into each item - client has to apply a post process to re-assign bold, etc.
          QFont itemFont = item->font(i);
          if(itemFont != widgetFont)
            item->setFont(i, widgetFont);
        }

        // Put children on stack for further processing
        for(int i = 0; i < item->childCount(); ++i)
        {
          if(item != nullptr)
            itemStack.append(item->child(i));
        }
      }
    }
  }

  widget->updateGeometry();
}

void WidgetZoomHandler::initTableViewZoom()
{
  // Adjust cell height to be smaller than default but according to font height
  double fontPointSize = QApplication::font().pointSizeF();
  setTableViewFontSize(!settingsKey.isEmpty() ? Settings::instance().valueDouble(settingsKey, fontPointSize) : fontPointSize);
}

void WidgetZoomHandler::zoomPercent(int percent)
{
#ifdef DEBUG_INFORMATION_ZOOMHANDLER
  qDebug() << Q_FUNC_INFO << widget->objectName() << percent;
#endif

  double pointSize = QApplication::font().pointSizeF() * percent / 100.;

  setTableViewFontSize(pointSize);
  if(!settingsKey.isEmpty())
    Settings::instance().setValue(settingsKey, pointSize);
  enableDisableZoomActions();
}

const QSet<QObject *> WidgetZoomHandler::getRegisteredWidgets()
{
  QMutexLocker locker(&registeredWidgetsMutex);
  return registeredWidgets;
}

void WidgetZoomHandler::zoomTableView(int value)
{
#ifdef DEBUG_INFORMATION_ZOOMHANDLER
  qDebug() << Q_FUNC_INFO << widget->objectName() << value;
#endif

  double pointSize = QApplication::font().pointSizeF();

  if(value != 0)
    pointSize = widget->font().pointSizeF() + value;

  setTableViewFontSize(pointSize);

  if(!settingsKey.isEmpty())
    Settings::instance().setValue(settingsKey, pointSize);
  enableDisableZoomActions();
}

void WidgetZoomHandler::enableDisableZoomActions()
{
  if(actionZoomDefault != nullptr)
    actionZoomDefault->setEnabled(atools::almostNotEqual(widget->font().pointSizeF(), QApplication::font().pointSizeF()));

  if(actionZoomIn != nullptr)
    actionZoomIn->setEnabled(widget->font().pointSizeF() < maxFontSize);

  if(actionZoomOut != nullptr)
    actionZoomOut->setEnabled(widget->font().pointSizeF() > minFontSize);
}

} // namespace gui
} // namespace atools
