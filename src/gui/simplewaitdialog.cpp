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

#include "application.h"
#include "simplewaitdialog.h"
#include "ui_simplewaitdialog.h"

namespace atools {
namespace gui {

SimpleWaitDialogWidget::SimpleWaitDialogWidget(QWidget *parent, const QString& labelText)
  : QDialog(parent), ui(new Ui::SimpleWaitDialog)
{
  Application::closeSplashScreen();

  QGuiApplication::setOverrideCursor(Qt::WaitCursor);

  ui->setupUi(this);

  setWindowTitle(QCoreApplication::applicationName());
  setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
  setLabelText(QStringLiteral("<big><b>") % labelText % QStringLiteral("</b></big>"));

  show();
  adjustSize();

  // Required - otherwise wwon't update text
  atools::gui::Application::processEventsExtended();
  repaint();
  atools::gui::Application::processEventsExtended();
}

SimpleWaitDialogWidget::~SimpleWaitDialogWidget()
{
  delete ui;

  QGuiApplication::restoreOverrideCursor();
}

void SimpleWaitDialogWidget::setLabelText(const QString& labelText)
{
  ui->labelSimpleWaitDialog->setText(labelText);
  adjustSize();

  // Required - otherwise won't update text
  atools::gui::Application::processEventsExtended();
  repaint();
  atools::gui::Application::processEventsExtended();
}

} // namespace gui
} // namespace atools
