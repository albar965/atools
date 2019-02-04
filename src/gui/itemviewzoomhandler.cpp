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

#include "gui/itemviewzoomhandler.h"

#include "settings/settings.h"
#include "atools.h"

#include <QDebug>
#include <QAction>
#include <QFont>
#include <QTableView>
#include <QHeaderView>

namespace atools {
namespace gui {

using atools::settings::Settings;

ItemViewZoomHandler::ItemViewZoomHandler(QAbstractItemView *view, QAction *zoomInAction,
                                         QAction *zoomOutAction, QAction *zoomDefaultAction,
                                         QString settingsKeyStr)
  : itemView(view), actionZoomIn(zoomInAction), actionZoomOut(zoomOutAction),
  actionZoomDefault(zoomDefaultAction), settingsKey(settingsKeyStr)
{
  Q_ASSERT(view != nullptr);

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

void ItemViewZoomHandler::fontChanged()
{
  qDebug() << Q_FUNC_INFO;

  setTableViewFontSize(defaultTableViewFontPointSize);
}

void ItemViewZoomHandler::setTableViewFontSize(float pointSize)
{
  QFont newFont(itemView->font());
  newFont.setPointSizeF(pointSize);

  int newFontHeight = QFontMetrics(newFont).height();

  qDebug() << "new font height" << newFontHeight << "point size" << newFont.pointSize();

  itemView->setFont(newFont);

  QTableView *tableView = dynamic_cast<QTableView *>(itemView);
  if(tableView != nullptr)
  {
    // Adjust the cell height - default is too big
    tableView->verticalHeader()->setDefaultSectionSize(newFontHeight + sectionToFontSize);
    tableView->verticalHeader()->setMinimumSectionSize(newFontHeight + sectionToFontSize);
  }
}

void ItemViewZoomHandler::initTableViewZoom()
{
  // Adjust cell height to be smaller than default but according to font height
  defaultTableViewFontPointSize = static_cast<float>(itemView->font().pointSizeF());

  // Increase default table font size for mac
#if defined(Q_OS_OSX)

  QTableView *tableView = dynamic_cast<QTableView *>(itemView);
  if(tableView != nullptr)
    defaultTableViewFontPointSize *= 1.4f;
#endif

  float newPointSize = 0.f;
  if(!settingsKey.isEmpty())
    newPointSize = Settings::instance().valueFloat(settingsKey,
                                                   defaultTableViewFontPointSize);
  else
    newPointSize = defaultTableViewFontPointSize;

  qDebug() << Q_FUNC_INFO << "newPointSize" << newPointSize
           << "defaultTableViewFontPointSize" << defaultTableViewFontPointSize;

  setTableViewFontSize(newPointSize);
}

void ItemViewZoomHandler::zoomIn()
{
  zoomTableView(1);
}

void ItemViewZoomHandler::zoomOut()
{
  zoomTableView(-1);
}

void ItemViewZoomHandler::zoomDefault()
{
  zoomTableView(0);
}

void ItemViewZoomHandler::zoomPercent(int percent)
{
  float newPointSize = defaultTableViewFontPointSize * percent / 100.f;

  setTableViewFontSize(newPointSize);
  if(!settingsKey.isEmpty())
    Settings::instance().setValue(settingsKey, itemView->font().pointSize());
  enableDisableZoomActions();
}

void ItemViewZoomHandler::zoomTableView(int value)
{
  float newPointSize = defaultTableViewFontPointSize;

  if(value != 0)
    newPointSize = itemView->font().pointSize() + value;

  setTableViewFontSize(newPointSize);
  if(!settingsKey.isEmpty())
    Settings::instance().setValue(settingsKey, itemView->font().pointSize());
  enableDisableZoomActions();
}

void ItemViewZoomHandler::enableDisableZoomActions()
{
  if(actionZoomDefault != nullptr)
    actionZoomDefault->setEnabled(atools::almostNotEqual(static_cast<float>(itemView->font().pointSizeF()),
                                                         defaultTableViewFontPointSize));
  if(actionZoomIn != nullptr)
    actionZoomIn->setEnabled(itemView->font().pointSize() < maxFontSize);
  if(actionZoomOut != nullptr)
    actionZoomOut->setEnabled(itemView->font().pointSize() > minFontSize);
}

} // namespace gui
} // namespace atools
