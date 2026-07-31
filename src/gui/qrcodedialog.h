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

class QAbstractButton;
namespace Ui {
class QrCodeDialog;
}

namespace atools {
namespace gui {

/*
 * Simple dialog showing a Qr Code with close and optional help button.
 * Shows a combo box if more than one Qr code is provided.
 */
class QrCodeDialog :
  public QDialog
{
  Q_OBJECT

public:
  /* title is dialog title and imageTitlesParam is combo box entry */
  explicit QrCodeDialog(QWidget *parent, const QStringList& qrCodesParam, const QStringList& imageTitlesParam,
                        const QStringList& imageLabelsParam, const QString& title, const QString& helpBaseUrlParam);
  virtual ~QrCodeDialog();

  void setHelpOnlineUrl(const QString& value)
  {
    helpOnlineUrl = value;
  }

  void setHelpLanguageOnline(const QString& value)
  {
    helpLanguageOnline = value;
  }

private:
  /* Update label and code image */
  void currentIndexChanged(int index);
  void buttonBoxClicked(QAbstractButton *button);

  /* Event filter for label to enshure width == height */
  bool eventFilter(QObject *object, QEvent *event);

  /* Update image from current combo index enshuring aspect ratio of one */
  void updateQrCodePixmap();

  QString helpBaseUrl, helpOnlineUrl, helpLanguageOnline;

  Ui::QrCodeDialog *ui;
  QStringList titles, labels, qrCodes;
};

} // namespace gui
} // namespace atools

#endif // ATOOLS_GUI_IMAGEDIALOG
