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
#include <QApplication>
#include <QMessageBox>
#include <QCheckBox>
#include <QStandardPaths>

namespace atools {
namespace gui {

using atools::settings::Settings;

QString Dialog::fileDialog(QFileDialog& dlg, const QString& title, const QString& filter,
                           const QString& settingsPrefix, const QString& defaultFileSuffix,
                           const QString& path, const QString& filename)
{
  dlg.setNameFilter(filter);
  dlg.setWindowTitle(QApplication::applicationName() + " - " + title);

  if(!defaultFileSuffix.isEmpty())
    dlg.setDefaultSuffix(defaultFileSuffix);

  QString settingName, settingNameDir;
  Settings& s = Settings::instance();

  if(!settingsPrefix.isEmpty())
  {
    // Key for dialog state
    settingName = settingsPrefix + "FileDialog";
    // Key for dialog directory
    settingNameDir = settingName + "Dir";

    // Read state
    if(s.contains(settingName))
      dlg.restoreState(s.valueVar(settingName).toByteArray());
  }

  QString defaultDir;

  if(path.isEmpty())
    // Use documents as default if path not given
    defaultDir = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).at(0);
  else
    defaultDir = path;

  // Get path from settings use path or documents as default
  QFileInfo dir = s.valueStr(settingNameDir, defaultDir);

  if(dir.exists())
  {
    if(dir.isDir())
      dlg.setDirectory(dir.absoluteFilePath());
    else if(dir.isFile())
      dlg.setDirectory(dir.absolutePath());
  }

  if(!filename.isEmpty())
    dlg.selectFile(filename);

  if(dlg.exec() && !dlg.selectedFiles().isEmpty())
  {
    if(!settingsPrefix.isEmpty())
    {
      // if ok/select/save was pressed save state
      s.setValueVar(settingName, dlg.saveState());
      s.setValue(settingNameDir, dlg.directory().absolutePath());
      s.syncSettings();
    }

    return dlg.selectedFiles().at(0);
  }
  return QString();

}

QString Dialog::openDirectoryDialog(const QString& title, const QString& settingsPrefix,
                                    const QString& path)
{
  QFileDialog dlg(parent);
  dlg.setFileMode(QFileDialog::DirectoryOnly);
  dlg.setAcceptMode(QFileDialog::AcceptOpen);
  return fileDialog(dlg, title, QString(), settingsPrefix, QString(), path, QString());
}

QString Dialog::openFileDialog(const QString& title, const QString& filter, const QString& settingsPrefix,
                               const QString& path)
{
  QFileDialog dlg(parent);
  dlg.setFileMode(QFileDialog::ExistingFile);
  dlg.setAcceptMode(QFileDialog::AcceptOpen);
  return fileDialog(dlg, title, filter, settingsPrefix, QString(), path, QString());
}

QString Dialog::saveFileDialog(const QString& title,
                               const QString& filter,
                               const QString& defaultFileSuffix,
                               const QString& settingsPrefix,
                               const QString& path,
                               const QString& filename)
{
  QFileDialog dlg(parent);
  dlg.setFileMode(QFileDialog::AnyFile);
  dlg.setAcceptMode(QFileDialog::AcceptSave);
  return fileDialog(dlg, title, filter, settingsPrefix, defaultFileSuffix, path, filename);
}

void Dialog::showInfoMsgBox(const QString& settingsKey, const QString& message,
                            const QString& checkBoxMessage)
{
  Settings& s = Settings::instance();

  // show only if the key is true
  if(s.valueBool(settingsKey, true))
  {
    QMessageBox msg(QMessageBox::Information,
                    QApplication::applicationName(), message, QMessageBox::Ok, parent);
    msg.setCheckBox(new QCheckBox(checkBoxMessage, &msg));
    msg.exec();
    s.setValue(settingsKey, !msg.checkBox()->isChecked());
    s.syncSettings();
  }
}

int Dialog::showQuestionMsgBox(const QString& settingsKey, const QString& message,
                               const QString& checkBoxMessage, DialogButtonList buttonList,
                               QMessageBox::StandardButton dialogDefaultButton,
                               QMessageBox::StandardButton defaultButton)
{
  int retval = defaultButton;
  Settings& s = Settings::instance();

  // show only if the key is true or empty
  if(settingsKey.isEmpty() || s.valueBool(settingsKey, true))
  {
    // Build button field
    QMessageBox::StandardButtons buttons = QMessageBox::NoButton;
    for(const DialogButton& db : buttonList)
      buttons |= db.button;

    QMessageBox msg(QMessageBox::Question, QApplication::applicationName(), message, buttons, parent);
    if(!checkBoxMessage.isEmpty())
      msg.setCheckBox(new QCheckBox(checkBoxMessage, &msg));
    msg.setDefaultButton(dialogDefaultButton);

    // Set the button texts
    for(const DialogButton& db : buttonList)
      if(!db.text.isEmpty())
        msg.setButtonText(db.button, db.text);

    retval = msg.exec();

    if(!settingsKey.isEmpty())
    {
      s.setValue(settingsKey, !msg.checkBox()->isChecked());
      s.syncSettings();
    }
  }
  return retval;
}

int Dialog::showQuestionMsgBox(const QString& settingsKey, const QString& message,
                               const QString& checkBoxMessage, QMessageBox::StandardButtons buttons,
                               QMessageBox::StandardButton dialogDefaultButton,
                               QMessageBox::StandardButton defaultButton)
{
  int retval = defaultButton;
  Settings& s = Settings::instance();

  // show only if the key is true or empty
  if(settingsKey.isEmpty() || s.valueBool(settingsKey, true))
  {
    QMessageBox msg(QMessageBox::Question, QApplication::applicationName(), message, buttons, parent);
    if(!checkBoxMessage.isEmpty())
      msg.setCheckBox(new QCheckBox(checkBoxMessage, &msg));
    msg.setDefaultButton(dialogDefaultButton);
    retval = msg.exec();

    if(!settingsKey.isEmpty())
    {
      s.setValue(settingsKey, !msg.checkBox()->isChecked());
      s.syncSettings();
    }
  }
  return retval;
}

} // namespace gui
} // namespace atools
