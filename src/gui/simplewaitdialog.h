/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_SIMPLEWAITDIALOG_H
#define ATOOLS_SIMPLEWAITDIALOG_H

#include <QDialog>

namespace Ui {
class SimpleWaitDialog;
}

namespace atools {
namespace gui {

/* Show a simple message box for showing action instead of progress.
* The box has no close button and does not close on Esc.
* Also sets wait cursor. */
class SimpleWaitDialogWidget :
  public QDialog
{
  Q_OBJECT

public:
  explicit SimpleWaitDialogWidget(QWidget *parent, const QString& labelText);
  virtual ~SimpleWaitDialogWidget() override;

  /* Do not allow copying */
  SimpleWaitDialogWidget(const SimpleWaitDialogWidget& other) = delete;
  SimpleWaitDialogWidget& operator=(const SimpleWaitDialogWidget& other) = delete;

  /* Set label text inside box, refresh and update dialog size */
  void setLabelText(const QString& labelText);

private:
  Ui::SimpleWaitDialog *ui;
};

/*
 * Closes the dialog automatically when leaving scope.
 */
class SimpleWaitDialog
{
public:
  explicit SimpleWaitDialog(QWidget *parent, const QString& labelText)
    :  dialog(new SimpleWaitDialogWidget(parent, labelText))
  {
  }

  SimpleWaitDialog()
    : dialog(nullptr)
  {
  }

  ~SimpleWaitDialog()
  {
    close();
  }

  /* Do not allow copying */
  SimpleWaitDialog(const SimpleWaitDialog& other) = delete;
  SimpleWaitDialog& operator=(const SimpleWaitDialog& other) = delete;

  void init(QWidget *parent, const QString& labelText)
  {
    dialog = new SimpleWaitDialogWidget(parent, labelText);
  }

  /* Set label text inside box, refresh and update dialog size */
  void setLabelText(const QString& labelText)
  {
    dialog->setLabelText(labelText);
  }

  void close()
  {
    delete dialog;
    dialog = nullptr;
  }

private:
  SimpleWaitDialogWidget *dialog;

};

} // namespace gui
} // namespace atools

#endif // ATOOLS_SIMPLEWAITDIALOG_H
