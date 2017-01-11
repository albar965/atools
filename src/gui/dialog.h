/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_GUI_DIALOG_H
#define ATOOLS_GUI_DIALOG_H

#include <QMessageBox>

class QWidget;
class QFileDialog;

namespace atools {
namespace gui {

/* Allows to set a text for each button */
struct DialogButton
{
  QString text; /* Text for button */
  QMessageBox::StandardButtons button; /* Button type */
};

typedef QVector<DialogButton> DialogButtonList;

/* Provides multiple methods to show dialogs that save their status in a
 * settings object or file.
 * @see atools::settings::Settings
 */
class Dialog
{
public:
  /*
   * @param parentWidget widget for all dialogs
   */
  Dialog(QWidget *parentWidget)
    : parent(parentWidget)
  {
  }

  /*
   * Creates an open file dialog and returns the selected file otherwise an
   * empty string. Stores dialog settings and current directory in the settings
   * object.
   *
   * @param title Dialog title that will be preixed by "ApplicationName - "
   * @param fiter File filter like
   * "Document Files (*.doc *.mydoc;;All Files (*))"
   * @param settingsPrefix Settings of this dialog will be saved under
   * section/key settingsPrefix + more
   * @param path initial path to use
   * @return Selected filename or empty string if cancel was pressed
   * @see atools::settings::Settings
   */
  QString openFileDialog(const QString& title, const QString& filter,
                         const QString& settingsPrefix = QString(), const QString& path = QString());

  /*
   * Creates an open directory dialog and returns the selected directory otherwise an
   * empty string. Stores dialog settings and current directory in the settings
   * object.
   *
   * @param title Dialog title that will be preixed by "ApplicationName - "
   * @param settingsPrefix Settings of this dialog will be saved under
   * section/key settingsPrefix + more
   * @param path initial path to use
   *
   * @return Selected filename or empty string if cancel was pressed
   * @see atools::settings::Settings
   */
  QString openDirectoryDialog(const QString& title, const QString& settingsPrefix, const QString& path);

  /*
   * Creates an save file dialog and returns the selected file otherwise an
   * empty string. Stores dialog settings and current directory in the settings
   * object.
   *
   * @param title Dialog title that will be preixed by "ApplicationName - "
   * @param fiter File filter like
   * "Document Files (*.doc *.mydoc;;All Files (*))"
   * @param settingsPrefix Settings of this dialog will be saved under
   * section/key settingsPrefix + more
   * @param suffix Default suffix that will be added to a file if no suffix was
   * given
   * @param path initial path to use
   *
   * @return Selected filename or empty string if cancel was pressed
   * @see atools::settings::Settings
   */
  QString saveFileDialog(const QString& title, const QString& filter, const QString& defaultFileSuffix,
                         const QString& settingsPrefix = QString(),
                         const QString& path = QString(), const QString& filename = QString());

  /*
   * Shows a simple information message box that includes a checkbox which can
   * be used to disable the box in the future.
   * Stores the checkbox state in the settings object.
   *
   * @param settingsKey
   * @param message
   * @param checkBoxMessage
   * @return
   * @see atools::settings::Settings
   */
  void showInfoMsgBox(const QString& settingsKey, const QString& message, const QString& checkBoxMessage);

  /*
   * Shows a simple question message box that includes a checkbox which can be
   * used to disable the box in the future.
   * Stores the checkbox state in the settings object.
   *
   * @param settingsKey group/key that should be used to store the
   * checkbox state
   * @param message Dialog message
   * @param checkBoxMessage Text for the checkbox
   * @param buttons Standard buttons for the box
   * @param dialogdefaultButton Default button that will be active in the dialog.
   * @param defaultButton Default button. This will be returned if the dialog
   * was disabled.
   * @return Selected button number
   * @see atools::settings::Settings
   */
  int showQuestionMsgBox(const QString& settingsKey, const QString& message, const QString& checkBoxMessage,
                         QMessageBox::StandardButtons buttons,
                         QMessageBox::StandardButton dialogDefaultButton,
                         QMessageBox::StandardButton defaultButton);

  /* Same as above but allows to add a list of buttons and assign texts to all buttons */
  int showQuestionMsgBox(const QString& settingsKey, const QString& message, const QString& checkBoxMessage,
                         DialogButtonList buttonList,
                         QMessageBox::StandardButton dialogDefaultButton,
                         QMessageBox::StandardButton defaultButton);

private:
  QString fileDialog(QFileDialog& dlg, const QString& title, const QString& filter,
                     const QString& settingsPrefix, const QString& defaultFileSuffix,
                     const QString& path, const QString& filename);

  QWidget *parent = nullptr;
};

} // namespace gui
} // namespace atools

Q_DECLARE_TYPEINFO(atools::gui::DialogButton, Q_MOVABLE_TYPE);

#endif // ATOOLS_GUI_DIALOG_H
