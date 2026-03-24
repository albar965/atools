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

#ifndef ATOOLS_GUI_COMBOBOXHANDLER_H
#define ATOOLS_GUI_COMBOBOXHANDLER_H

#include <QObject>

class QPushButton;
class QComboBox;

namespace atools {
namespace gui {

/*
 * Saves, loads and sorts texts in an editable combo box.
 * Puts an activated entry to the top of the list and selects it.
 * Adds a context menu to the line edit to delete an entry or all list entries.
 * Handles an optional delete push button to remove active element.
 */
class ComboBoxHandler
  : public QObject
{
  Q_OBJECT

public:
  explicit ComboBoxHandler(QComboBox *comboBoxParam, QPushButton *deleteButtonParam = nullptr, const QString& settingsKeyParam = QString());
  virtual ~ComboBoxHandler() override
  {

  }

  /* Do not allow copying */
  ComboBoxHandler(const ComboBoxHandler& other) = delete;
  ComboBoxHandler& operator=(const ComboBoxHandler& other) = delete;

  /* Save or restore text entries of combo box list using given settingsKeyParam.
   * Does not save state or content of the line edit. This has to be done separately. */
  void saveState();
  void restoreState();

  /* Show tooltips in context menu */
  void setMenuTooltipsVisible(bool newMenuTooltipsVisible)
  {
    menuTooltipsVisible = newMenuTooltipsVisible;
  }

private:
  /* Custom context menu of line edit */
  void showContextMenu(const QPoint& point);

  /* Put current entry to top of list and activate it */
  void editingFinished();

  /* Remove current entry */
  void deleteClicked(bool = false);

  /* Put current entry to top of list and activate it */
  void currentIndexChanged(int index);

  /* Enable or disable optional delete push button */
  void updateButtonState();

  QComboBox *comboBox = nullptr;
  QPushButton *deleteButton = nullptr;
  QString settingsKey;
  bool menuTooltipsVisible = false;
};

} // namespace gui
} // namespace atools

#endif // ATOOLS_GUI_COMBOBOXHANDLER_H
