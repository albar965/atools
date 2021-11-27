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

#include "gui/griddelegate.h"
#include "atools.h"

#include <QApplication>
#include <QPainter>

namespace atools {
namespace gui {

GridDelegate::GridDelegate(QObject *parent) :
  QStyledItemDelegate(parent)
{
  styleChanged();
}

void GridDelegate::styleChanged()
{
  gridPen = QPen(QApplication::palette().color(QPalette::Active, QPalette::Window), borderPenWidth);
}

QSize GridDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
  QSize sz = QStyledItemDelegate::sizeHint(option, index);
  sz.setHeight(sz.height() + heightIncrease);
  return sz;
}

void GridDelegate::paint(QPainter *painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
  QStyledItemDelegate::paint(painter, option, index);
  painter->save();
  painter->setPen(gridPen);
  painter->drawRect(option.rect);
  painter->restore();
}

} // namespace gui
} // namespace atools
