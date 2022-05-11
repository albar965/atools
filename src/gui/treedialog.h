/*****************************************************************************
* Copyright 2015-2021 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_TREEDIALOG_H
#define ATOOLS_TREEDIALOG_H

#include <QDialog>
#include <QSet>

namespace Ui {
class TreeDialog;
}

class QTreeWidgetItem;
class QAbstractButton;

namespace atools {
namespace gui {

class ItemViewZoomHandler;
class GridDelegate;

/*
 * A configurable dialog that shows the user a tree widget with checkboxes.
 * Uses one of two colums.
 *
 * Allows to use simple enumerations for item ids which have to be unique. Otherwise an exception is thown on insert.
 * Ids have to fit into an int.
 *
 * Allows only one tree level. Means: No branches in branches.
 */
class TreeDialog :
  public QDialog
{
  Q_OBJECT

public:
  /* settingsPrefixParam is used to save the dialog and item check state.
   * helpBaseUrlParam is the base URL of the help system. Help button will be hidden if empty.
   * showExpandCollapse: Show buttons. */
  TreeDialog(QWidget *parent, const QString& title, const QString& description, const QString& settingsPrefixParam,
             const QString& helpBaseUrlParam, bool showExpandCollapse);
  virtual ~TreeDialog() override;

  /* Do not allow copying */
  TreeDialog(const TreeDialog& other) = delete;
  TreeDialog& operator=(const TreeDialog& other) = delete;

  /* Set header labels. The widget will be resized to a column number according the number of headers. */
  void setHeader(const QStringList& header);

  /* Resize all columns to contents */
  void resizeToContents();

  /* Get the invisible root item */
  QTreeWidgetItem *getRootItem() const;

  /* Add a branch with checkbox and auto-tristate depending on children. Parent is always root.  */
  QTreeWidgetItem *addTopItem(const QStringList& text, const QString& tooltip = QString());

  QTreeWidgetItem *addTopItem1(const QString& text1, const QString& tooltip = QString())
  {
    return addTopItem({text1}, tooltip);
  }

  QTreeWidgetItem *addTopItem2(const QString& text1, const QString& text2, const QString& tooltip = QString())
  {
    return addTopItem({text1, text2}, tooltip);
  }

  /* Add an item to a branch or root. Has always a checkbox in the first column. */
  template<typename TYPE>
  QTreeWidgetItem *addItem(QTreeWidgetItem *parent, TYPE id, const QStringList& text, const QString& tooltip, bool checked = true)
  {
    return addItemInt(parent, static_cast<int>(id), text, tooltip, checked);
  }

  template<typename TYPE>
  QTreeWidgetItem *addItem(QTreeWidgetItem *parent, TYPE id, const QStringList& text, bool checked = true)
  {
    return addItemInt(parent, static_cast<int>(id), text, QString(), checked);
  }

  template<typename TYPE>
  QTreeWidgetItem *addItem2(QTreeWidgetItem *parent, TYPE id, const QString& text1, const QString& text2, const QString& tooltip,
                            bool checked = true)
  {
    return addItemInt(parent, static_cast<int>(id), {text1, text2}, tooltip, checked);
  }

  template<typename TYPE>
  QTreeWidgetItem *addItem2(QTreeWidgetItem *parent, TYPE id, const QString& text1, const QString& text2, bool checked = true)
  {
    return addItemInt(parent, static_cast<int>(id), {text1, text2}, QString(), checked);
  }

  /* True if the item with the given type exists and has check state checked */
  template<typename TYPE>
  bool isItemChecked(TYPE id) const
  {
    return isCheckedInt(static_cast<int>(id));
  }

  /* Set item state to checked or unchecked. Ignored if item does not exist. */
  template<typename TYPE>
  void setItemChecked(TYPE id, bool checked = true)
  {
    setCheckedInt(static_cast<int>(id), checked);
  }

  /* True if the item with the given type exists and has check state checked */
  template<typename TYPE>
  bool isItemDisabled(TYPE id) const
  {
    return isDisabledInt(static_cast<int>(id));
  }

  /* Set item state to enabled or disabled. Ignored if item does not exist. */
  template<typename TYPE>
  void setItemDisabled(TYPE id, bool disabled = true)
  {
    setDisabledInt(static_cast<int>(id), disabled);
  }

  /* Set all item states to checked or unchecked */
  void setAllChecked(bool checked = true);

  /* Call after adding all buttons to restore header, expand state, size and optionally check states */
  void restoreState(bool restoreCheckState, bool restoreExpandState);

  /* Save header, expand state, sizem optionally check states and expand state of branches.  */
  void saveState(bool saveCheckState, bool saveExpandState);

  /* For atools::gui::HelpHandler::openHelpUrlWeb() */
  void setHelpOnlineUrl(const QString& value)
  {
    helpOnlineUrl = value;
  }

  /* For atools::gui::HelpHandler::openHelpUrlWeb() */
  void setHelpLanguageOnline(const QString& value)
  {
    helpLanguageOnline = value;
  }

signals:
  /* Emitted once a checkbox state has changed. */
  void itemToggled(atools::gui::TreeDialog *treeDialog, int id, bool checked);

private:
  void buttonBoxClicked(QAbstractButton *button);
  void dataChanged(const QModelIndex& topLeft, const QModelIndex&, const QVector<int>& roles = QVector<int>());

  /* Untyped methods used after converting an enum to int */
  QTreeWidgetItem *addItemInt(QTreeWidgetItem *parent, int id, const QStringList& text, const QString& tooltip, bool checked);
  bool isCheckedInt(int id) const;
  void setCheckedInt(int id, bool checked);

  bool isDisabledInt(int id) const;
  void setDisabledInt(int id, bool disabled);

  /* Save header, expand state and size */
  void saveStateDialog(bool saveExpandState);

  QTreeWidgetItem *getItemInt(int id);
  void checkAll();
  void unCheckAll();

  Ui::TreeDialog *ui;
  QString helpBaseUrl, settingsPrefix, helpOnlineUrl, helpLanguageOnline;

  /* Maps user given id to item. Not top level items. */
  QHash<int, QTreeWidgetItem *> index;

  atools::gui::ItemViewZoomHandler *zoomHandler = nullptr;
  atools::gui::GridDelegate *gridDelegate = nullptr;

};

} // namespace gui
} // namespace atools

#endif // ATOOLS_TREEDIALOG_H
