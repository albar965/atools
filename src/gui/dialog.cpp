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
#include "gui/dialog.h"
#include "gui/tools.h"
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
                               const QString& path, const QString& filename, bool autoNumberFilename, int *filterIndex)
{
  Application::closeSplashScreen();

  if(filterIndex != nullptr)
    *filterIndex = -1;

  dlg.setNameFilter(filter);
  dlg.setWindowTitle(QCoreApplication::applicationName() + " - " + title);
  dlg.setWindowFlag(Qt::WindowContextHelpButtonHint, false);

  if(!defaultFileSuffix.isEmpty())
    dlg.setDefaultSuffix(defaultFileSuffix);

  QString settingName, settingNameDir;
  Settings& settings = Settings::instance();

  if(!settingsPrefix.isEmpty())
  {
    // Key for dialog state
    settingName = settingsPrefix + "FileDialog";
    // Key for dialog directory
    settingNameDir = settingName + "Dir";

    // Read state
    if(settings.contains(settingName))
      dlg.restoreState(settings.valueVar(settingName).toByteArray());
  }

  QString defaultDir;

  if(path.isEmpty())
    // Use documents as default if path not given
    defaultDir = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).value(0);
  else
    defaultDir = path;

  // Get path from settings use path or documents as default
  QFileInfo dir(settingsPrefix.isEmpty() ? defaultDir : settings.valueStr(settingNameDir, defaultDir));

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
      dir.setFile(dir.dir().path());

    dlg.setDirectory(dir.absoluteFilePath());
    qDebug() << dir.absoluteFilePath() << "corrected path";
  }

  QString name(filename);
  if(autoNumberFilename && dir.isDir() && !name.isEmpty())
  {
    // Choose separator depending on filename
    QString sep = name.contains(" ") ? " " : "_";
    QFileInfo base(dir.filePath() + QDir::separator() + name);
    QFileInfo fi(base);

    int i = 1;
    while(fi.exists() && i < 100)
      fi.setFile(dir.filePath() + QDir::separator() + base.baseName() + QStringLiteral("%1%2").arg(sep).arg(i++) + "." +
                 base.completeSuffix());

    name = fi.fileName();
  }

  if(!name.isEmpty())
    dlg.selectFile(name);

  if(dlg.exec() && !dlg.selectedFiles().isEmpty())
  {
    if(!settingsPrefix.isEmpty())
    {
      // if ok/select/save was pressed save state
      settings.setValueVar(settingName, dlg.saveState());
      settings.setValue(settingNameDir, dlg.directory().absolutePath());
      Settings::syncSettings();
    }

    if(filterIndex != nullptr)
      *filterIndex = dlg.nameFilters().indexOf(dlg.selectedNameFilter());

    QStringList files = dlg.selectedFiles();
    return files.isEmpty() ? QStringList({QString()}) : files;
  }
  return QStringList({QString()});
}

QString Dialog::openDirectoryDialog(const QString& title, const QString& settingsPrefix, const QString& path)
{
  QFileDialog dlg(parent);
  dlg.setFileMode(QFileDialog::Directory);
  dlg.setAcceptMode(QFileDialog::AcceptOpen);
  return fileDialog(dlg, title, QString(), settingsPrefix, QString(), path, QString(), false /* autonumber */).at(0);
}

QString Dialog::openFileDialog(const QString& title, const QString& filter, const QString& settingsPrefix, const QString& path)
{
  QFileDialog dlg(parent);
  dlg.setFileMode(QFileDialog::ExistingFile);
  dlg.setAcceptMode(QFileDialog::AcceptOpen);
  return fileDialog(dlg, title, filter, settingsPrefix, QString(), path, QString(), false /* autonumber */).at(0);
}

QStringList Dialog::openFileDialogMulti(const QString& title, const QString& filter, const QString& settingsPrefix, const QString& path)
{
  QFileDialog dlg(parent);
  dlg.setFileMode(QFileDialog::ExistingFiles);
  dlg.setAcceptMode(QFileDialog::AcceptOpen);
  return fileDialog(dlg, title, filter, settingsPrefix, QString(), path, QString(), false /* autonumber */);
}

QString Dialog::saveFileDialog(const QString& title, const QString& filter, const QString& defaultFileSuffix, const QString& settingsPrefix,
                               const QString& path, const QString& filename, bool dontComfirmOverwrite, bool autoNumberFilename,
                               int *filterIndex)
{
  QFileDialog dlg(parent);
  dlg.setFileMode(QFileDialog::AnyFile);
  dlg.setAcceptMode(QFileDialog::AcceptSave);
  dlg.setOption(QFileDialog::DontConfirmOverwrite, dontComfirmOverwrite);

  return fileDialog(dlg, title, filter, settingsPrefix, defaultFileSuffix, path, filename, autoNumberFilename, filterIndex).at(0);
}

void Dialog::showInfoMsgBox(const QString& settingsKey, const QString& message, const QString& checkBoxMessage)
{
  atools::gui::logMessageBox(parent, QMessageBox::Information, message);
  Settings& settings = Settings::instance();

  // show only if the key is true
  if(settingsKey.isEmpty() || settings.valueBool(settingsKey, true))
  {
    Application::closeSplashScreen();

    QMessageBox msg(QMessageBox::Information, QCoreApplication::applicationName(), message, QMessageBox::Ok, parent);
    if(!settingsKey.isEmpty() && !checkBoxMessage.isEmpty())
      msg.setCheckBox(new QCheckBox(checkBoxMessage, &msg));
    msg.setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    msg.setWindowModality(Qt::ApplicationModal);
    msg.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);

    msg.exec();

    if(!settingsKey.isEmpty() && msg.checkBox() != nullptr)
    {
      settings.setValue(settingsKey, !msg.checkBox()->isChecked());
      Settings::syncSettings();
    }
  }
}

void Dialog::showWarnMsgBox(const QString& settingsKey, const QString& message, const QString& checkBoxMessage)
{
  atools::gui::logMessageBox(parent, QMessageBox::Warning, message);
  Settings& settings = Settings::instance();

  // show only if the key is true
  if(settingsKey.isEmpty() || settings.valueBool(settingsKey, true))
  {
    Application::closeSplashScreen();

    QMessageBox msg(QMessageBox::Warning, QCoreApplication::applicationName(), message, QMessageBox::Ok, parent);
    if(!settingsKey.isEmpty() && !checkBoxMessage.isEmpty())
      msg.setCheckBox(new QCheckBox(checkBoxMessage, &msg));
    msg.setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    msg.setWindowModality(Qt::ApplicationModal);
    msg.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);

    msg.exec();
    if(!settingsKey.isEmpty() && msg.checkBox() != nullptr)
    {
      settings.setValue(settingsKey, !msg.checkBox()->isChecked());
      Settings::syncSettings();
    }
  }
}

int Dialog::showQuestionMsgBox(const QString& settingsKey, const QString& message, const QString& checkBoxMessage,
                               DialogButtonList buttonList, QMessageBox::StandardButton dialogDefaultButton,
                               QMessageBox::StandardButton defaultButton)
{
  atools::gui::logMessageBox(parent, QMessageBox::Question, message);

  int retval = defaultButton;
  Settings& settings = Settings::instance();

  // show only if the key is true or empty
  if(settingsKey.isEmpty() || settings.valueBool(settingsKey, true))
  {
    Application::closeSplashScreen();

    // Build button field
    QMessageBox::StandardButtons buttons = QMessageBox::NoButton;
    for(const DialogButton& db : buttonList)
      buttons |= db.getButton();

    QMessageBox msg(QMessageBox::Question, QCoreApplication::applicationName(), message, buttons, parent);
    if(!settingsKey.isEmpty() && !checkBoxMessage.isEmpty())
      msg.setCheckBox(new QCheckBox(checkBoxMessage, &msg));
    msg.setDefaultButton(dialogDefaultButton);
    msg.setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    msg.setWindowModality(Qt::ApplicationModal);
    msg.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);

    // Set the button texts
    for(const DialogButton& db : buttonList)
      if(!db.getText().isEmpty())
        msg.setButtonText(db.getButton(), db.getText());

    retval = msg.exec();

    if(retval != QMessageBox::Cancel && retval != QMessageBox::Help && !settingsKey.isEmpty() && msg.checkBox() != nullptr)
    {
      settings.setValue(settingsKey, !msg.checkBox()->isChecked());
      Settings::syncSettings();
    }
  }
  return retval;
}

int Dialog::showQuestionMsgBox(const QString& settingsKey, const QString& message,
                               const QString& checkBoxMessage, QMessageBox::StandardButtons buttons,
                               QMessageBox::StandardButton dialogDefaultButton,
                               QMessageBox::StandardButton defaultButton)
{
  atools::gui::logMessageBox(parent, QMessageBox::Question, message);

  int retval = defaultButton;
  Settings& s = Settings::instance();

  // show only if the key is true or empty
  if(settingsKey.isEmpty() || s.valueBool(settingsKey, true))
  {
    Application::closeSplashScreen();

    QMessageBox msg(QMessageBox::Question, QCoreApplication::applicationName(), message, buttons, parent);
    if(!settingsKey.isEmpty() && !checkBoxMessage.isEmpty())
      msg.setCheckBox(new QCheckBox(checkBoxMessage, &msg));
    msg.setDefaultButton(dialogDefaultButton);
    msg.setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    msg.setWindowModality(Qt::ApplicationModal);
    msg.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
    retval = msg.exec();

    if(retval != QMessageBox::Cancel && !settingsKey.isEmpty() && msg.checkBox() != nullptr)
    {
      s.setValue(settingsKey, !msg.checkBox()->isChecked());
      Settings::syncSettings();
    }
  }
  return retval;
}

QMessageBox *Dialog::showSimpleProgressDialog(QWidget *parentWidget, const QString& message)
{
  Application::closeSplashScreen();

  QGuiApplication::setOverrideCursor(Qt::WaitCursor);

  QMessageBox *progressBox = new QMessageBox(QMessageBox::NoIcon, QCoreApplication::applicationName(), message,
                                             QMessageBox::NoButton, parentWidget);
  progressBox->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
  progressBox->setStandardButtons(QMessageBox::NoButton);
  progressBox->setWindowFlag(Qt::WindowContextHelpButtonHint, false);
  progressBox->setWindowModality(Qt::ApplicationModal);
  progressBox->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
  progressBox->show();

  Application::processEventsExtended();
  return progressBox;
}

void Dialog::deleteSimpleProgressDialog(QMessageBox *messageBox)
{
  messageBox->close();
  messageBox->deleteLater();

  QGuiApplication::restoreOverrideCursor();
}

QMessageBox::StandardButton Dialog::messageBox(QWidget *parent, QMessageBox::Icon icon, const QString& text,
                                               QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton)
{
  atools::gui::logMessageBox(parent, icon, text);

  Application::closeSplashScreen();

  QMessageBox box(icon, QCoreApplication::applicationName(), text, buttons, parent);
  box.setDefaultButton(defaultButton);
  box.setWindowFlag(Qt::WindowContextHelpButtonHint, false);
  box.setWindowModality(Qt::ApplicationModal);
  box.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
  return static_cast<QMessageBox::StandardButton>(box.exec());
}

} // namespace gui
} // namespace atools
