/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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

#include "gui/tablezoomhandler.h"

#include "logging/loggingdefs.h"
#include "settings/settings.h"
#include "atools.h"

#include <QAction>
#include <QFont>
#include <QTableView>
#include <QHeaderView>

namespace atools {
namespace gui {

using atools::settings::Settings;

TableZoomHandler::TableZoomHandler(QTableView *view, QAction *zoomInAction, QAction *zoomOutAction,
                                   QAction *zoomDefaultAction, QString settingsKeyStr)
  : tableView(view), actionZoomIn(zoomInAction), actionZoomOut(zoomOutAction),
    actionZoomDefault(zoomDefaultAction), settingsKey(settingsKeyStr)
{
  Q_ASSERT(view != nullptr);

  initTableViewZoom();

  if(actionZoomIn != nullptr)
    connect(actionZoomIn, &QAction::triggered, this, &TableZoomHandler::zoomIn);
  if(actionZoomOut != nullptr)
    connect(actionZoomOut, &QAction::triggered, this, &TableZoomHandler::zoomOut);
  if(actionZoomDefault != nullptr)
    connect(actionZoomDefault, &QAction::triggered, this, &TableZoomHandler::zoomDefault);
}

TableZoomHandler::~TableZoomHandler()
{
  if(actionZoomIn != nullptr)
    disconnect(actionZoomIn, &QAction::triggered, this, &TableZoomHandler::zoomIn);
  if(actionZoomOut != nullptr)
    disconnect(actionZoomOut, &QAction::triggered, this, &TableZoomHandler::zoomOut);
  if(actionZoomDefault != nullptr)
    disconnect(actionZoomDefault, &QAction::triggered, this, &TableZoomHandler::zoomDefault);
}

void TableZoomHandler::setTableViewFontSize(float pointSize)
{
  QFont newFont(tableView->font());
  newFont.setPointSizeF(pointSize);

  int newFontHeight = QFontMetrics(newFont).height();

  qDebug() << "new font height" << newFontHeight << "point size" << newFont.pointSize();

  tableView->setFont(newFont);

  // Adjust the cell height - default is too big
  tableView->verticalHeader()->setDefaultSectionSize(newFontHeight + sectionToFontSize);
  tableView->verticalHeader()->setMinimumSectionSize(newFontHeight + sectionToFontSize);
}

void TableZoomHandler::initTableViewZoom()
{
  // Adjust cell height to be smaller than default but according to font height
  defaultTableViewFontPointSize = static_cast<float>(tableView->font().pointSizeF());

  float newPointSize = 0.f;
  if(!settingsKey.isEmpty())
    newPointSize = Settings::instance().valueFloat(settingsKey,
                                                   defaultTableViewFontPointSize);
  else
    newPointSize = defaultTableViewFontPointSize;
  setTableViewFontSize(newPointSize);
}

void TableZoomHandler::zoomIn()
{
  zoomTableView(1);
}

void TableZoomHandler::zoomOut()
{
  zoomTableView(-1);
}

void TableZoomHandler::zoomDefault()
{
  zoomTableView(0);
}

void TableZoomHandler::zoomPercent(int percent)
{
  float newPointSize = defaultTableViewFontPointSize * percent / 100.f;

  setTableViewFontSize(newPointSize);
  if(!settingsKey.isEmpty())
    Settings::instance().setValue(settingsKey, tableView->font().pointSize());
  enableDisableZoomActions();
}

void TableZoomHandler::zoomTableView(int value)
{
  float newPointSize = defaultTableViewFontPointSize;

  if(value != 0)
    newPointSize = tableView->font().pointSize() + value;

  setTableViewFontSize(newPointSize);
  if(!settingsKey.isEmpty())
    Settings::instance().setValue(settingsKey, tableView->font().pointSize());
  enableDisableZoomActions();
}

void TableZoomHandler::enableDisableZoomActions()
{
  if(actionZoomDefault != nullptr)
    actionZoomDefault->setEnabled(atools::almostNotEqual(static_cast<float>(tableView->font().pointSizeF()),
                                                         defaultTableViewFontPointSize));
  if(actionZoomIn != nullptr)
    actionZoomIn->setEnabled(tableView->font().pointSize() < maxFontSize);
  if(actionZoomOut != nullptr)
    actionZoomOut->setEnabled(tableView->font().pointSize() > minFontSize);
}

} // namespace gui
} // namespace atools
