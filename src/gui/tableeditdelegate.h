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

#ifndef ATOOLS_GUI_TABLEEDITDELEGATE_H
#define ATOOLS_GUI_TABLEEDITDELEGATE_H

#include <QStyledItemDelegate>

class QPlainTextEdit;
class QTableView;

namespace atools {
namespace gui {

/* Callback with row and column which can return true it cell is editable */
typedef std::function<bool (int col, int row)> TableEditDelegateEditFuncType;

/*
 * Provides functions to open a larger QPlainTextEdit overlay to add multi-line comments
 * once user double clicks a cell for editing.
 * Cell does not need edit mode. Focus out or key Esc will end editing and model will send itemChanged().
 *
 * Edit starts on double click.
 */
class TableEditDelegate
  : public QStyledItemDelegate
{
  Q_OBJECT

public:
  explicit TableEditDelegate(QTableView *viewParam, QAbstractItemModel *modelParam);
  virtual ~TableEditDelegate() override;

  /* Set callback that receives row and column which can return true it cell is editable.
   All cells are edited if not set. */
  void setCanEditFunction(TableEditDelegateEditFuncType canEditFunctionParam)
  {
    canEditFunction = canEditFunctionParam;
  }

  /* true if currently editing */
  bool isEditing() const
  {
    return editingIndex.isValid();
  }

  /* Edit will be height of row by this factor */
  void setSizeFactor(int sizeFactorParam)
  {
    sizeFactor = sizeFactorParam;
  }

signals:
  /* Sent on start and end of edit mode */
  void editingStarted();
  void editingEnded();

private:
  virtual bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem& option, const QModelIndex& index) override;
  virtual bool eventFilter(QObject *obj, QEvent *event) override;

  int sizeFactor = 5;

  QTableView *view;
  QAbstractItemModel *model;

  /* Editor created on first edit */
  QPlainTextEdit *plainTextEdit = nullptr;

  /* Index is valid while editing */
  QModelIndex editingIndex;

  /* Callback to determined if column/row is editable */
  TableEditDelegateEditFuncType canEditFunction = nullptr;
};

} // namespace gui
} // namespace atools

#endif // ATOOLS_GUI_TABLEEDITDELEGATE_H
