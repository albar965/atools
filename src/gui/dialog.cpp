/*****************************************************************************
* Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
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
#include <QDebug>

namespace atools {
namespace gui {

using atools::settings::Settings;

QStringList Dialog::fileDialog(QFileDialog& dlg, const QString& title, const QString& filter,
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
  else
  {
    qDebug() << dir.absoluteFilePath() << "does not exist";
    // Go up the directory level until a valid dir is found - avoid endless iterations
    int i = 50;
    while(!dir.exists() && !dir.isRoot() && i-- > 0)
      dir = dir.dir().path();

    dlg.setDirectory(dir.absoluteFilePath());
    qDebug() << dir.absoluteFilePath() << "corrected path";
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

    QStringList files = dlg.selectedFiles();
    return files.isEmpty() ? QStringList({QString()}) : files;
  }
  return QStringList({QString()});
}

QString Dialog::openDirectoryDialog(const QString& title, const QString& settingsPrefix,
                                    const QString& path)
{
  QFileDialog dlg(parent);
  dlg.setFileMode(QFileDialog::DirectoryOnly);
  dlg.setAcceptMode(QFileDialog::AcceptOpen);
  return fileDialog(dlg, title, QString(), settingsPrefix, QString(), path, QString()).first();
}

QMessageBox::StandardButton Dialog::warning(QWidget *parentWidget, const QString& text,
                                            QMessageBox::StandardButtons buttons,
                                            QMessageBox::StandardButton defaultButton)
{
  qWarning() << Q_FUNC_INFO << text;
  return QMessageBox::warning(parentWidget, QApplication::applicationName(), text, buttons, defaultButton);
}

int Dialog::warning(QWidget *parentWidget, const QString& text, int button0, int button1, int button2)
{
  qWarning() << Q_FUNC_INFO << text;
  return QMessageBox::warning(parentWidget, QApplication::applicationName(), text, button0, button1, button2);
}

int Dialog::warning(QWidget *parentWidget, const QString& text, const QString& button0Text,
                    const QString& button1Text, const QString& button2Text, int defaultButtonNumber,
                    int escapeButtonNumber)
{
  qWarning() << Q_FUNC_INFO << text;
  return QMessageBox::warning(parentWidget, QApplication::applicationName(),
                              text, button0Text, button1Text, button2Text, defaultButtonNumber, escapeButtonNumber);
}

int Dialog::warning(QWidget *parentWidget, const QString& text,
                    QMessageBox::StandardButton button0, QMessageBox::StandardButton button1)
{
  qWarning() << Q_FUNC_INFO << text;
  return QMessageBox::warning(parentWidget, QApplication::applicationName(), text, button0, button1);
}

QString Dialog::openFileDialog(const QString& title, const QString& filter, const QString& settingsPrefix,
                               const QString& path)
{
  QFileDialog dlg(parent);
  dlg.setFileMode(QFileDialog::ExistingFile);
  dlg.setAcceptMode(QFileDialog::AcceptOpen);
  return fileDialog(dlg, title, filter, settingsPrefix, QString(), path, QString()).first();
}

QStringList Dialog::openFileDialogMulti(const QString& title, const QString& filter, const QString& settingsPrefix,
                                        const QString& path)
{
  QFileDialog dlg(parent);
  dlg.setFileMode(QFileDialog::ExistingFiles);
  dlg.setAcceptMode(QFileDialog::AcceptOpen);
  return fileDialog(dlg, title, filter, settingsPrefix, QString(), path, QString());
}

QString Dialog::saveFileDialog(const QString& title,
                               const QString& filter,
                               const QString& defaultFileSuffix,
                               const QString& settingsPrefix,
                               const QString& path,
                               const QString& filename,
                               bool dontComfirmOverwrite)
{
  QFileDialog dlg(parent);
  dlg.setFileMode(QFileDialog::AnyFile);
  dlg.setAcceptMode(QFileDialog::AcceptSave);
  dlg.setOption(QFileDialog::DontConfirmOverwrite, dontComfirmOverwrite);

  return fileDialog(dlg, title, filter, settingsPrefix, defaultFileSuffix, path, filename).first();
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
    msg.setWindowFlags(msg.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    msg.setWindowModality(Qt::ApplicationModal);

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
    msg.setWindowFlags(msg.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    msg.setWindowModality(Qt::ApplicationModal);

    // Set the button texts
    for(const DialogButton& db : buttonList)
      if(!db.text.isEmpty())
        msg.setButtonText(db.button, db.text);

    retval = msg.exec();

    if(retval != QMessageBox::Cancel && !settingsKey.isEmpty())
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
    msg.setWindowFlags(msg.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    msg.setWindowModality(Qt::ApplicationModal);
    retval = msg.exec();

    if(retval != QMessageBox::Cancel && !settingsKey.isEmpty())
    {
      s.setValue(settingsKey, !msg.checkBox()->isChecked());
      s.syncSettings();
    }
  }
  return retval;
}

} // namespace gui
} // namespace atools
