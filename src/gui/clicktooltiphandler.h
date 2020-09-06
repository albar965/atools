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

#ifndef LNM_CLICKTOOLTIPHANDLER_H
#define LNM_CLICKTOOLTIPHANDLER_H

#include <QObject>

class QLabel;

namespace atools {
namespace gui {

/*
 * Catches events from a widget to show a tooltip on click.
 */
class ClickToolTipHandler :
  public QObject
{
public:
  ClickToolTipHandler(QLabel *parentLabel);
  virtual ~ClickToolTipHandler() override;

private:
  virtual bool eventFilter(QObject *object, QEvent *event) override;

  QLabel *label;
};

} // namespace gui
} // namespace atools

#endif // LNM_CLICKTOOLTIPHANDLER_H
