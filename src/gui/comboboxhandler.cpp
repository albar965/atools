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

#include "gui/comboboxhandler.h"

#include "settings/settings.h"

#include <QComboBox>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>

namespace atools {
namespace gui {

using atools::settings::Settings;

ComboBoxHandler::ComboBoxHandler(QComboBox *comboBoxParam, QPushButton *deleteButtonParam, const QString& settingsKeyParam)
  : QObject(comboBoxParam), comboBox(comboBoxParam), deleteButton((deleteButtonParam)), settingsKey(settingsKeyParam)
{
  // Cannot be set in designer
  comboBox->lineEdit()->setClearButtonEnabled(true);

  // Set in case it is not correct in Qt Designer
  comboBox->setContextMenuPolicy(Qt::CustomContextMenu);

  // Called when pressing return or leaving focus
  connect(comboBox->lineEdit(), &QLineEdit::editingFinished, this, &ComboBoxHandler::editingFinished);
  connect(comboBox->lineEdit(), &QLineEdit::textChanged, this, &ComboBoxHandler::updateButtonState);

  // Called on change
  connect(comboBox, &QComboBox::currentIndexChanged, this, &ComboBoxHandler::currentIndexChanged);
  connect(comboBox, &QComboBox::customContextMenuRequested, this, &ComboBoxHandler::showContextMenu);

  if(deleteButton != nullptr)
    connect(deleteButton, &QPushButton::clicked, this, &ComboBoxHandler::deleteClicked);
}

void ComboBoxHandler::showContextMenu(const QPoint& point)
{
  qDebug() << Q_FUNC_INFO << point;

  // Get standard menu to extend
  QMenu *menu = comboBox->lineEdit()->createStandardContextMenu();
  menu->setToolTipsVisible(menuTooltipsVisible);

  // Delete action, child of menu
  QAction *deleteCurrentAction = new QAction(tr("Delete &current List Entry %1").arg(comboBox->currentText()), menu);
  deleteCurrentAction->setEnabled(comboBox->count() > 0 && !comboBox->currentText().isEmpty());

  // Delete all action, child of menu
  QAction *deleteAllAction = new QAction(tr("Delete &all List Entries"), menu);
  deleteAllAction->setEnabled(comboBox->count() > 0);

  menu->addSeparator();
  menu->addAction(deleteCurrentAction);
  menu->addAction(deleteAllAction);

  // Show menu
  QAction *action = menu->exec(comboBox->lineEdit()->mapToGlobal(point));
  if(action == deleteCurrentAction)
    comboBox->removeItem(comboBox->currentIndex());
  else if(action == deleteAllAction)
    comboBox->clear();

  // Delete child actions too
  delete menu;
}

void ComboBoxHandler::currentIndexChanged(int index)
{
#ifdef DEBUG_INFORMATION
  qDebug() << Q_FUNC_INFO << index << comboBox->currentIndex() << comboBox->currentText();
#endif

  editingFinished();
}

void ComboBoxHandler::updateButtonState()
{
  if(deleteButton != nullptr)
    deleteButton->setDisabled(comboBox->currentText().isEmpty());
}

void ComboBoxHandler::editingFinished()
{
#ifdef DEBUG_INFORMATION
  qDebug() << Q_FUNC_INFO << comboBox->currentIndex() << comboBox->currentText();
#endif

  const QString currentText = comboBox->currentText();
  if(!currentText.isEmpty())
  {
    // Look for current text in list
    int foundIndex = comboBox->findText(currentText, Qt::MatchExactly);

    if(foundIndex != -1)
    {
      // Move all entries from index one until current index up
      // .         1 2 3(4)5 6
      // fromIndex A B C X E F
      // toIndex   X A B C E F
      // SearchComboHistory=A, B, C, X, E, F
      for(int i = foundIndex; i > 0; i--)
        comboBox->setItemText(i, comboBox->itemText(i - 1));

      // Set current entry to top of list
      comboBox->setItemText(0, currentText);
    }
    else
      // Not found in list - insert at top
      comboBox->insertItem(0, currentText);

    // Select top entry
    comboBox->setCurrentIndex(0);
  }
}

void ComboBoxHandler::deleteClicked(bool)
{
#ifdef DEBUG_INFORMATION
  qDebug() << Q_FUNC_INFO << comboBox->currentIndex() << comboBox->currentText();
#endif

  comboBox->removeItem(comboBox->currentIndex());
}

void ComboBoxHandler::saveState()
{
  if(!settingsKey.isEmpty())
  {
    // Create a clean list
    QStringList entries;
    for(int i = 0; i < comboBox->count(); i++)
      entries.append(comboBox->itemText(i));

    // Remove duplicates and empty entries
    entries.removeDuplicates();
    entries.removeAll(QStringLiteral());

    Settings::instance().setValue(settingsKey, entries);
  }
}

void ComboBoxHandler::restoreState()
{
  if(!settingsKey.isEmpty())
  {
    const QStringList entries = Settings::instance().valueStrList(settingsKey);

    for(const QString& entry : entries)
      comboBox->addItem(entry);
  }

  editingFinished();
}

} // namespace gui
} // namespace atools
