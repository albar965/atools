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

#include "gui/widgettool.h"

#include <QAction>
#include <QLabel>
#include <QLineEdit>
#include <QStringBuilder>
#include <QTextEdit>
#include <QWidget>

namespace atools {
namespace gui {

void WidgetTool::setHidden(bool hidden)
{
  for(QWidget *widget : widgets)
    widget->setHidden(hidden);
}

void WidgetTool::setVisible(bool visible)
{
  for(QWidget *widget : widgets)
    widget->setVisible(visible);
}

void WidgetTool::setEnabled(bool enabled)
{
  for(QWidget *widget : widgets)
    widget->setEnabled(enabled);
}

void WidgetTool::setDisabled(bool disabled)
{
  for(QWidget *widget : widgets)
    widget->setDisabled(disabled);
}

void WidgetTool::setText(const QString& text)
{
  for(QWidget *widget : widgets)
  {
    if(QLabel *label = dynamic_cast<QLabel *>(widget))
      label->setText(text);
    else if(QLineEdit *lineEdit = dynamic_cast<QLineEdit *>(widget))
      lineEdit->setText(text);
    else if(QTextEdit *textEdit = dynamic_cast<QTextEdit *>(widget))
      textEdit->setText(text);
    else if(QAction *action = dynamic_cast<QAction *>(widget))
      action->setText(text);
  }
}

void WidgetTool::clear()
{
  for(QWidget *widget : widgets)
  {
    if(QLabel *label = dynamic_cast<QLabel *>(widget))
      label->clear();
    else if(QLineEdit *lineEdit = dynamic_cast<QLineEdit *>(widget))
      lineEdit->clear();
    else if(QTextEdit *textEdit = dynamic_cast<QTextEdit *>(widget))
      textEdit->clear();
    else if(QAction *action = dynamic_cast<QAction *>(widget))
      action->setText(QStringLiteral());
  }
}

} // namespace gui
} // namespace atools
