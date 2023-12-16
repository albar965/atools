/*****************************************************************************
* Copyright 2015-2023 Alexander Barthel alex@littlenavmap.org
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

#include "gui/messagebox.h"
#include "ui_messagebox.h"

#include "gui/helphandler.h"

#include <QAbstractButton>
#include <QStyle>

namespace atools {
namespace gui {

MessageBox::MessageBox(QWidget *parent, const QString& title)
  : QDialog(parent), ui(new Ui::MessageBox)
{
  ui->setupUi(this);

  setWindowTitle(title);
  setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setWindowModality(Qt::ApplicationModal);

  // Hide icon per default
  ui->labelIcon->hide();

  connect(ui->label, &QLabel::linkActivated, this, &MessageBox::linkActivated);
  connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &MessageBox::buttonBoxClicked);
}

MessageBox::~MessageBox()
{
  delete ui;
}

void MessageBox::addAcceptButton(QDialogButtonBox::StandardButton button)
{
  acceptButtons.append(button);
  ui->buttonBox->addButton(button);
}

void MessageBox::addRejectButton(QDialogButtonBox::StandardButton button)
{
  rejectButtons.append(button);
  ui->buttonBox->addButton(button);
}

void MessageBox::addButton(QDialogButtonBox::StandardButton button)
{
  ui->buttonBox->addButton(button);
}

void MessageBox::setText(const QString& text)
{
  ui->label->setText(text);
}

void MessageBox::setIcon(QMessageBox::Icon dialogIcon)
{
  // Get standard icon from style
  QStyle *style = QApplication::style();
  QIcon icon;
  switch(dialogIcon)
  {
    case QMessageBox::Information:
      icon = style->standardIcon(QStyle::SP_MessageBoxInformation);
      break;

    case QMessageBox::Warning:
      icon = style->standardIcon(QStyle::SP_MessageBoxWarning, nullptr, this);
      break;

    case QMessageBox::Critical:
      icon = style->standardIcon(QStyle::SP_MessageBoxCritical, nullptr, this);
      break;

    case QMessageBox::Question:
      icon = style->standardIcon(QStyle::SP_MessageBoxQuestion, nullptr, this);
      break;

    case QMessageBox::NoIcon:
      break;
  }

  if(!icon.isNull())
  {
    // Set icon to label
    int iconSize = style->pixelMetric(QStyle::PM_MessageBoxIconSize, nullptr, this);
    ui->labelIcon->setPixmap(icon.pixmap(iconSize, iconSize));
    ui->labelIcon->show();
  }
  else
  {
    // Otherwise clear and hide
    ui->labelIcon->clear();
    ui->labelIcon->hide();
  }
}

int MessageBox::exec()
{
  // Layout contents before
  adjustSize();

  return QDialog::exec();
}

void MessageBox::buttonBoxClicked(QAbstractButton *button)
{
  QDialogButtonBox::StandardButton buttonType = ui->buttonBox->standardButton(button);

  if(acceptButtons.contains(buttonType))
    accept();
  else if(rejectButtons.contains(buttonType))
    reject();
  else if(buttonType == QDialogButtonBox::Help && !helpDocument.isEmpty())
    atools::gui::HelpHandler::openHelpUrlWeb(this, helpOnlineUrl + helpDocument, helpLanguageOnline);
}

} // namespace gui
} // namespace atools
