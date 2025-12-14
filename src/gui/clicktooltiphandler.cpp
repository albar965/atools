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

#include "gui/clicktooltiphandler.h"

#include <QLabel>
#include <QMouseEvent>
#include <QToolTip>

namespace atools {
namespace gui {

ClickToolTipHandler::ClickToolTipHandler(QLabel *parentLabel)
  : QObject(parentLabel), label(parentLabel)
{

}

ClickToolTipHandler::~ClickToolTipHandler()
{

}

bool ClickToolTipHandler::eventFilter(QObject *object, QEvent *event)
{
  if(event->type() == QEvent::MouseButtonRelease && !label->hasSelectedText())
  {
    QMouseEvent *mouseEvent = dynamic_cast<QMouseEvent *>(event);
    if(mouseEvent != nullptr)
    {
      QToolTip::showText(QCursor::pos(), label->toolTip(), label);
      return true;
    }
  }

  return QObject::eventFilter(object, event);
}

} // namespace gui
} // namespace atools
