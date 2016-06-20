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

#ifndef ATOOLS_WIDGETTOOLS_H
#define ATOOLS_WIDGETTOOLS_H

#include <QList>

class QLayout;
class QWidget;
class QAction;
class QObject;

namespace atools {
namespace gui {

class WidgetTools
{
public:
  static void showHideLayoutElements(const QList<QLayout *> layouts, bool visible,
                                     const QList<QWidget *>& otherWidgets);

  static bool anyWidgetChanged(const QList<const QObject *>& widgets);

  static bool allChecked(const QList<const QAction *>& actions);
  static bool noneChecked(const QList<const QAction *>& actions);

  static void changeStarIndication(QAction *action, bool changed);

};

} // namespace gui
} // namespace atools

#endif // ATOOLS_WIDGETTOOLS_H
