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

#include "gui/messagebox.h"

#include "gui/desktopservices.h"
#include "gui/helphandler.h"
#include "settings/settings.h"
#include "application.h"
#include "ui_messagebox.h"

#include <QAbstractButton>
#include <QStyle>
#include <QUrl>
#include <QDebug>

namespace atools {
namespace gui {

MessageBox::MessageBox(QWidget *parent, const QString& title, const QString& settingsKeyParam, const QString& checkBoxMessage,
                       bool openLinkAuto)
  : QDialog(parent), ui(new Ui::MessageBox), settingsKey(settingsKeyParam)
{
  ui->setupUi(this);

  if(title.isEmpty())
    setWindowTitle(QCoreApplication::applicationName());
  else
    setWindowTitle(title);

  setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setWindowModality(Qt::ApplicationModal);

  // Hide icon per default
  ui->labelIcon->hide();

  if(checkBoxMessage.isEmpty())
    ui->checkBox->hide();
  else
  {
    ui->checkBox->setText(checkBoxMessage);
    checkBox = true;
  }

  connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &MessageBox::buttonBoxClicked);

  if(openLinkAuto)
    connect(ui->labelText, &QLabel::linkActivated, this, &MessageBox::linkActivatedAuto);
  else
    connect(this, &atools::gui::MessageBox::linkActivated, this, &MessageBox::linkActivated);
}

MessageBox::~MessageBox()
{
  delete ui;
}

void MessageBox::linkActivatedAuto(const QString& link)
{
  atools::gui::DesktopServices::openUrl(this, link, showInFileManager);
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

void MessageBox::setMessage(const QString& text)
{
  ui->labelText->setText(text);
}

void MessageBox::setIcon(QMessageBox::Icon dialogIconParam)
{
  dialogIcon = dialogIconParam;

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
  qInfo().noquote().nospace() << Q_FUNC_INFO << ui->labelText->text();

  if(acceptButtons.isEmpty())
  {
    // Set default button layout
    switch(dialogIcon)
    {
      case QMessageBox::NoIcon:
      case QMessageBox::Information:
      case QMessageBox::Warning:
      case QMessageBox::Critical:
        addAcceptButton(QDialogButtonBox::Ok);
        break;

      case QMessageBox::Question:
        addAcceptButton(QDialogButtonBox::Yes);
        addRejectButton(QDialogButtonBox::No);
        break;
    }
  }

  QDialogButtonBox::StandardButton retval = defaultButton;
  atools::settings::Settings& settings = atools::settings::Settings::instance();

  // show only if the key is true or not given
  if(settingsKey.isEmpty() || settings.valueBool(settingsKey, true))
  {
    Application::closeSplashScreen();

    // Layout contents before
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    adjustSize();

    QDialog::exec();
    retval = clickedButton;

    if(acceptButtons.contains(retval) && retval != QDialogButtonBox::Help && !settingsKey.isEmpty() && checkBox)
    {
      settings.setValue(settingsKey, !ui->checkBox->isChecked());
      atools::settings::Settings::syncSettings();
    }
  }
  return retval;
}

void MessageBox::setHelpUrl(const QString& url, const QString& language)
{
  helpUrl = url;
  helpLanguage = language;
  addButton(QDialogButtonBox::Help);
}

void MessageBox::buttonBoxClicked(QAbstractButton *button)
{
  clickedButton = ui->buttonBox->standardButton(button);

  if(acceptButtons.contains(clickedButton))
    accept();
  else if(rejectButtons.contains(clickedButton))
    reject();
  else if(clickedButton == QDialogButtonBox::Help && !helpUrl.isEmpty())
    atools::gui::HelpHandler::openHelpUrlWeb(this, helpUrl, helpLanguage);
}

} // namespace gui
} // namespace atools
