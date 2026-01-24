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
class SimpleWaitDialog :
  public QDialog
{
  Q_OBJECT

public:
  explicit SimpleWaitDialog(QWidget *parent, const QString& message);
  virtual ~SimpleWaitDialog() override;

  /* Set label text inside box, refresh and update dialog size */
  void setLabelText(const QString& labelText);

private:
  Ui::SimpleWaitDialog *ui;
};

} // namespace gui
} // namespace atools

#endif // ATOOLS_SIMPLEWAITDIALOG_H
