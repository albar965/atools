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
  /*
   * borderPenWidthParam = width border around cells
   * heightIncreaseParam = cell height increased by pixels
   */
  explicit GridDelegate(QObject *parent, double borderPenWidthParam = 1.5, int heightIncreaseParam = 3);
  virtual ~GridDelegate() override;

  /* Updates pen */
  void styleChanged();

private:
  QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
  void paint(QPainter *painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

  QPen gridPen;
  double borderPenWidth;
  int heightIncrease;
};

} // namespace gui
} // namespace atools

#endif // ATOOLS_GUI_GRIDDELEGATE_H
