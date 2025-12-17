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

#ifndef ATOOLS_GUI_GRIDDELEGATE_H
#define ATOOLS_GUI_GRIDDELEGATE_H

#include <QPen>
#include <QStyledItemDelegate>

namespace atools {
namespace gui {

/*
 * Delegate that is used to draw a grid around cells in e.g. a QTreeWidget
 */
class GridDelegate :
  public QStyledItemDelegate
{
  Q_OBJECT

public:
  explicit GridDelegate(QObject *parent);
  virtual ~GridDelegate() override;

  void styleChanged();

  /* Width of border pen */
  void setBorderPenWidth(double value)
  {
    borderPenWidth = value;
  }

  void setHeightIncrease(int value)
  {
    heightIncrease = value;
  }

private:
  QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
  void paint(QPainter *painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

  QPen gridPen;
  double borderPenWidth = 1.5;
  int heightIncrease = 3;
};

} // namespace gui
} // namespace atools

#endif // ATOOLS_GUI_GRIDDELEGATE_H
