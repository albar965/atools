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

#include "gui/qrcodedialog.h"

#include "gui/helphandler.h"
#include "qrcode/qrcodegenerator.h"
#include "ui_qrcodedialog.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QResizeEvent>

namespace atools {
namespace gui {

QrCodeDialog::QrCodeDialog(QWidget *parent, const QStringList& qrCodesParam, const QStringList& imageTitlesParam,
                           const QStringList& imageLabelsParam, const QString& title, const QString& helpBaseUrlParam)
  : QDialog(parent), helpBaseUrl(helpBaseUrlParam), ui(new Ui::QrCodeDialog), titles(imageTitlesParam), labels(imageLabelsParam),
  qrCodes(qrCodesParam)
{
  ui->setupUi(this);
  setWindowFlag(Qt::WindowContextHelpButtonHint, false);
  setWindowModality(Qt::ApplicationModal);
  setWindowTitle(title);

  Q_ASSERT(qrCodes.size() == titles.size());
  Q_ASSERT(qrCodes.size() == labels.size());

  // Hide box if not needed
  if(qrCodes.size() == 1)
    ui->comboBox->setHidden(true);

  // Install filter for image size
  ui->labelImage->installEventFilter(this);

  ui->labelHeader->setText(labels.constFirst());
  ui->comboBox->addItems(titles);
  updateQrCodePixmap();

  if(helpBaseUrl.isEmpty())
    // Remove help button if not requested
    ui->buttonBox->removeButton(ui->buttonBox->button(QDialogButtonBox::Help));

  connect(ui->comboBox, &QComboBox::currentIndexChanged, this, &QrCodeDialog::currentIndexChanged);
  connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &QrCodeDialog::buttonBoxClicked);

  updateGeometry();
}

QrCodeDialog::~QrCodeDialog()
{
  ui->labelImage->removeEventFilter(this);
  delete ui;
}

bool QrCodeDialog::eventFilter(QObject *object, QEvent *event)
{
  if(object == ui->labelImage && event->type() == QEvent::Resize)
    updateQrCodePixmap();

  return QObject::eventFilter(object, event);
}

void QrCodeDialog::updateQrCodePixmap()
{
  if(ui->comboBox->currentIndex() != -1)
  {
    int imageSize = std::min(ui->labelImage->size().height(), ui->labelImage->size().width()) - ui->labelImage->margin();
    ui->labelImage->setPixmap(QPixmap::fromImage(atools::qrcode::QrCodeGenerator(this).generateQr(qrCodes.at(ui->comboBox->currentIndex()),
                                                                                                  imageSize)));

    // Reset minimum size since widget cannot be scaled below pixmap size
    ui->labelImage->setMinimumSize(ui->comboBox->height() * 5, ui->comboBox->height() * 5);
  }
}

void QrCodeDialog::buttonBoxClicked(QAbstractButton *button)
{
  QDialogButtonBox::StandardButton buttonType = ui->buttonBox->standardButton(button);

  if(buttonType == QDialogButtonBox::Close)
    accept();
  else if(buttonType == QDialogButtonBox::Help && !helpBaseUrl.isEmpty())
    // Show help without closing dialog
    atools::gui::HelpHandler::openHelpUrlWeb(this, helpOnlineUrl + helpBaseUrl, helpLanguageOnline);
}

void QrCodeDialog::currentIndexChanged(int index)
{
  ui->labelHeader->setText(labels.at(index));
  updateQrCodePixmap();
}

} // namespace gui
} // namespace atools
