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

#ifndef ATOOLS_GUI_IMAGEDIALOG
#define ATOOLS_GUI_IMAGEDIALOG

#include <QDialog>

namespace Ui {
class ImageDialog;
}

namespace atools {
namespace gui {

/* Simple dialog showing an image with close button. */
class ImageDialog :
  public QDialog
{
  Q_OBJECT

public:
  /* title is dialog title and tooltip is image label tooltip */
  explicit ImageDialog(QWidget *parent, const QPixmap& pixmap, const QString& title, const QString& tooltip);
  virtual ~ImageDialog();

private:
  Ui::ImageDialog *ui;
};

} // namespace gui
} // namespace atools

#endif // ATOOLS_GUI_IMAGEDIALOG
