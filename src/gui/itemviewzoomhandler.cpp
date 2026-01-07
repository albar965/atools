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

#include "gui/itemviewzoomhandler.h"

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

using atools::settings::Settings;

void ItemViewZoomHandler::init(QAbstractItemView *view, QAction *zoomInAction, QAction *zoomOutAction,
                               QAction *zoomDefaultAction, QString settingsKeyStr, double marginParm)
{
  Q_ASSERT(view != nullptr);

  margin = marginParm;
  itemView = view;
  actionZoomIn = zoomInAction;
  actionZoomOut = zoomOutAction;
  actionZoomDefault = zoomDefaultAction;
  settingsKey = settingsKeyStr;

  initTableViewZoom();

  if(actionZoomIn != nullptr)
    connect(actionZoomIn, &QAction::triggered, this, &ItemViewZoomHandler::zoomIn);

  if(actionZoomOut != nullptr)
    connect(actionZoomOut, &QAction::triggered, this, &ItemViewZoomHandler::zoomOut);

  if(actionZoomDefault != nullptr)
    connect(actionZoomDefault, &QAction::triggered, this, &ItemViewZoomHandler::zoomDefault);
}

ItemViewZoomHandler::~ItemViewZoomHandler()
{
  if(actionZoomIn != nullptr)
    disconnect(actionZoomIn, &QAction::triggered, this, &ItemViewZoomHandler::zoomIn);

  if(actionZoomOut != nullptr)
    disconnect(actionZoomOut, &QAction::triggered, this, &ItemViewZoomHandler::zoomOut);

  if(actionZoomDefault != nullptr)
    disconnect(actionZoomDefault, &QAction::triggered, this, &ItemViewZoomHandler::zoomDefault);
}

void ItemViewZoomHandler::setTableViewFontSize(double pointSize)
{
  QFont font(QApplication::font());
  font.setPointSizeF(pointSize);

  double fontHeight = QFontMetricsF(font).height();

#ifdef DEBUG_INFORMATION
  qDebug() << Q_FUNC_INFO << "pointSize" << pointSize << itemView->objectName() << "new font height" << fontHeight
           << "point size" << font.pointSizeF();
#endif

  itemView->setFont(font);

  QTableView *tableView = dynamic_cast<QTableView *>(itemView);
  if(tableView != nullptr)
  {
    // Adjust the cell height - default is too big
    tableView->verticalHeader()->setMinimumSectionSize(atools::roundToInt(fontHeight + sectionToFontSize + margin * 2.));
    tableView->verticalHeader()->setDefaultSectionSize(atools::roundToInt(fontHeight + sectionToFontSize + margin * 2.));
  }
}

void ItemViewZoomHandler::initTableViewZoom()
{
  // Adjust cell height to be smaller than default but according to font height
  double fontPointSize = QApplication::font().pointSizeF();
  setTableViewFontSize(!settingsKey.isEmpty() ? Settings::instance().valueDouble(settingsKey, fontPointSize) : fontPointSize);
}

void ItemViewZoomHandler::zoomPercent(int percent)
{
#ifdef DEBUG_INFORMATION
  qDebug() << Q_FUNC_INFO << itemView->objectName() << percent;
#endif

  double pointSize = QApplication::font().pointSizeF() * percent / 100.;

  setTableViewFontSize(pointSize);
  if(!settingsKey.isEmpty())
    Settings::instance().setValue(settingsKey, pointSize);
  enableDisableZoomActions();
}

void ItemViewZoomHandler::zoomTableView(int value)
{
#ifdef DEBUG_INFORMATION
  qDebug() << Q_FUNC_INFO << itemView->objectName() << value;
#endif

  double pointSize = QApplication::font().pointSizeF();

  if(value != 0)
    pointSize = itemView->font().pointSizeF() + value;

  setTableViewFontSize(pointSize);

  if(!settingsKey.isEmpty())
    Settings::instance().setValue(settingsKey, pointSize);
  enableDisableZoomActions();
}

void ItemViewZoomHandler::enableDisableZoomActions()
{
  if(actionZoomDefault != nullptr)
    actionZoomDefault->setEnabled(atools::almostNotEqual(itemView->font().pointSizeF(), QApplication::font().pointSizeF()));

  if(actionZoomIn != nullptr)
    actionZoomIn->setEnabled(itemView->font().pointSizeF() < maxFontSize);

  if(actionZoomOut != nullptr)
    actionZoomOut->setEnabled(itemView->font().pointSizeF() > minFontSize);
}

} // namespace gui
} // namespace atools
