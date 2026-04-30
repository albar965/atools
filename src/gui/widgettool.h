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

#ifndef ATOOLS_GUI_WIDGETTOOL_H
#define ATOOLS_GUI_WIDGETTOOL_H

#include <QList>

class QWidget;

namespace atools {
namespace gui {

/*
 * Set state to a list of widgets. Does not take ownership.
 */
class WidgetTool
{
public:
  explicit WidgetTool(QList<QWidget *> widgetsParam)
    : widgets(widgetsParam)
  {

  }

  /* Apply to all widgets */
  void setHidden(bool hidden);
  void setVisible(bool visible);
  void setEnabled(bool enabled);
  void setDisabled(bool disabled);

  /* Applies to instances of QLabel QLineEdit, QTextEdit and QAction */
  void setText(const QString& text);

  /* Applies to instances of QLabel QLineEdit, QTextEdit and QAction */
  void clear();

private:
  const QList<QWidget *> widgets;
};

} // namespace gui
} // namespace atools

#endif // ATOOLS_GUI_WIDGETTOOL_H
