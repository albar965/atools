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

#include "gui/tableeditdelegate.h"

#include <QPlainTextEdit>
#include <QTableView>

namespace atools {
namespace gui {

TableEditDelegate::TableEditDelegate(QTableView *viewParam, QAbstractItemModel *modelParam)
  : QStyledItemDelegate(viewParam), view(viewParam), model(modelParam)
{
}

TableEditDelegate::~TableEditDelegate()
{
  delete plainTextEdit;
}

bool TableEditDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem& option,
                                    const QModelIndex& index)
{
#ifdef DEBUG_INFORMATION_TABLE_EDIT
  qDebug() << Q_FUNC_INFO << event->type() << index;
#endif

  if(event->type() == QEvent::MouseButtonDblClick && (!canEditFunction || canEditFunction(index.column(), index.row())))
  {
    emit editingStarted();

    if(plainTextEdit == nullptr)
    {
      // Create edit initially
      plainTextEdit = new QPlainTextEdit(view->viewport());
      plainTextEdit->setFont(option.font);
      plainTextEdit->setWordWrapMode(QTextOption::WordWrap);
      plainTextEdit->setTabChangesFocus(false);
      plainTextEdit->setFocusPolicy(Qt::StrongFocus);
      plainTextEdit->installEventFilter(this);
    }

    // Set text from model
    plainTextEdit->setPlainText(model->data(index, Qt::DisplayRole).toString());
    editingIndex = index;

    // Move and resize
    QRect rect = view->visualRect(index);
    plainTextEdit->move(view->visualRect(index).topLeft());
    plainTextEdit->resize(QSize(rect.width(), rect.height() * sizeFactor));

    // Show and focus
    plainTextEdit->show();
    plainTextEdit->raise();
    plainTextEdit->setFocus();
    return true;
  }

  return false;
}

bool TableEditDelegate::eventFilter(QObject *obj, QEvent *event)
{
#ifdef DEBUG_INFORMATION_TABLE_EDIT
  qDebug() << Q_FUNC_INFO << event->type() << (plainTextEdit != nullptr ? plainTextEdit->toPlainText() : "null");
#endif

  if(plainTextEdit != nullptr && plainTextEdit->isVisible())
  {
    bool done = false;
    if(event->type() == QEvent::FocusOut)
      // Lost focus
      done = true;
    else
    {
      QKeyEvent *keyEvent = dynamic_cast<QKeyEvent *>(event);
      if(keyEvent != nullptr && keyEvent->key() == Qt::Key_Escape)
        // Escape pressed
        done = true;
    }

    if(done)
    {
      if(plainTextEdit->toPlainText() != model->data(editingIndex, Qt::DisplayRole).toString())
      {
        // Model will send itemChanged()
        model->setData(editingIndex, plainTextEdit->toPlainText(), Qt::DisplayRole);
        emit editingEnded();
      }

      plainTextEdit->hide();

      // Activate view again
      view->raise();
      view->setFocus();
      view->selectRow(editingIndex.row());
      editingIndex = QModelIndex();
    }
  }

  return QStyledItemDelegate::eventFilter(obj, event);
}

} // namespace gui
} // namespace atools
