/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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

#include "gui/dialog.h"
#include "settings/settings.h"

#include <QFileDialog>
#include <QSettings>
#include <QApplication>
#include <QMessageBox>
#include <QCheckBox>
#include <QStandardPaths>

namespace atools {
namespace gui {

using atools::settings::Settings;

QString Dialog::fileDialog(QFileDialog& dlg,
                           const QString& title,
                           const QString& filter,
                           const QString& settingsPrefix,
                           const QString& defaultFileSuffix)
{
  dlg.setNameFilter(filter);
  dlg.setWindowTitle(QApplication::applicationName() + " - " + title);

  if(!defaultFileSuffix.isEmpty())
    dlg.setDefaultSuffix(defaultFileSuffix);

  // Key for dialog state
  QString settingName = settingsPrefix + "FileDialog";
  // Key for dialog directory
  QString settingNameDir = settingName + "Dir";

  Settings& s = Settings::instance();
  // Read state
  if(s->contains(settingName))
    dlg.restoreState(s->value(settingName).toByteArray());

  // Use documents as default if directory was not saved
  QStringList documents = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
  dlg.setDirectory(s->value(settingNameDir, documents).toString());

  if(dlg.exec() && !dlg.selectedFiles().isEmpty())
  {
    // if ok/select/save was pressed save state
    s->setValue(settingName, dlg.saveState());
    s->setValue(settingNameDir, dlg.directory().absolutePath());
    s.syncSettings();

    return dlg.selectedFiles().at(0);
  }
  return QString();

}

QString Dialog::openFileDialog(const QString& title, const QString& filter, const QString& settingsPrefix)
{
  QFileDialog dlg(parent);
  dlg.setFileMode(QFileDialog::ExistingFile);
  dlg.setAcceptMode(QFileDialog::AcceptOpen);
  return fileDialog(dlg, title, filter, settingsPrefix, QString());
}

QString Dialog::saveFileDialog(const QString& title,
                               const QString& filter,
                               const QString& settingsPrefix,
                               const QString& defaultFileSuffix)
{
  QFileDialog dlg(parent);
  dlg.setFileMode(QFileDialog::AnyFile);
  dlg.setAcceptMode(QFileDialog::AcceptSave);
  return fileDialog(dlg, title, filter, settingsPrefix, defaultFileSuffix);
}

void Dialog::showInfoMsgBox(const QString& settingsKey,
                            const QString& message,
                            const QString& checkBoxMessage)
{
  Settings& s = Settings::instance();

  // show only if the key is true
  if(s->value(settingsKey, true).toBool())
  {
    QMessageBox msg(QMessageBox::Information,
                    QApplication::applicationName(), message, QMessageBox::Ok, parent);
    msg.setCheckBox(new QCheckBox(checkBoxMessage, &msg));
    msg.exec();
    s->setValue(settingsKey, !msg.checkBox()->isChecked());
    s.syncSettings();
  }
}

int Dialog::showQuestionMsgBox(const QString& settingsKey,
                               const QString& message,
                               const QString& checkBoxMessage,
                               QMessageBox::StandardButtons buttons,
                               QMessageBox::StandardButton defaultButton)
{
  int retval = defaultButton;
  Settings& s = Settings::instance();

  // show only if the key is true
  if(s->value(settingsKey, true).toBool())
  {
    QMessageBox msg(QMessageBox::Question, QApplication::applicationName(), message, buttons, parent);
    msg.setCheckBox(new QCheckBox(checkBoxMessage, &msg));
    retval = msg.exec();
    s->setValue(settingsKey, !msg.checkBox()->isChecked());
    s.syncSettings();
  }
  return retval;
}

} // namespace gui
} // namespace atools
