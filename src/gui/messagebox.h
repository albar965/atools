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

#ifndef ATOOLS_MESSAGEBOX_H
#define ATOOLS_MESSAGEBOX_H

#include <QDialog>
#include <QDialogButtonBox>
#include <QMessageBox>

class QLabel;
class QAbstractButton;

namespace Ui {
class MessageBox;
}

namespace atools {
namespace gui {

/*
 * More versatile message box class which allows to add buttons like help which do not close the dialog window.
 * Clicking the help button will open the related URL but not close this one.
 * Additionally the linkActivated() signal is sent if a link is clicked in the label and openLinkAuto is false.
 * openLinkAuto = true opens links automatically.
 * If settingsKeyParam and checkBoxMessage are given the state is stored.
 */
class MessageBox :
  public QDialog
{
  Q_OBJECT

public:
  explicit MessageBox(QWidget *parent, const QString& title = QString(), const QString& settingsKeyParam = QString(),
                      const QString& checkBoxMessage = QString(), bool openLinkAuto = true);
  virtual ~MessageBox() override;

  /* Adjusts contents before exec().
   * Returns one of QDialogButtonBox::StandardButton or QMessageBox::StandardButton. */
  virtual int exec() override;

  /* Full URL with optional language placeholder like "https://www.littlenavmap.org/manuals/littlenavmap/release/latest/${LANG}/CRASHREPORT.html"
   * Language like "en" for online access.
   * Adds help button.*/
  void setHelpUrl(const QString& url, const QString& language);

  /* Add a standard button which will call accept() when clicked */
  void addAcceptButton(QDialogButtonBox::StandardButton button);

  /* Add a standard button which will call reject() when clicked */
  void addRejectButton(QDialogButtonBox::StandardButton button);

  /* Add a standard button which will keep the dialog open when clicked. Help triggers the help URL. */
  void addButton(QDialogButtonBox::StandardButton button);

  /* Set text for right aligned label */
  void setMessage(const QString& text);

  /* Set a standard icon for the top left label.  */
  void setIcon(QMessageBox::Icon dialogIconParam);

  /* Standard button function if "do not show again" is set */
  void setDefaultButton(QDialogButtonBox::StandardButton value)
  {
    defaultButton = value;
  }

  /* true if file links are to be shown in a file manager instead of opening */
  void setShowInFileManager(bool value = true)
  {
    showInFileManager = value;
  }

signals:
  void linkActivated(const QString& link);

private:
  Ui::MessageBox *ui;
  void buttonBoxClicked(QAbstractButton *button);
  void linkActivatedAuto(const QString& link);

  QVector<QDialogButtonBox::StandardButton> acceptButtons;
  QVector<QDialogButtonBox::StandardButton> rejectButtons;
  QDialogButtonBox::StandardButton defaultButton = QDialogButtonBox::NoButton, clickedButton = QDialogButtonBox::NoButton;
  QMessageBox::Icon dialogIcon;

  QString helpUrl, helpLanguage, settingsPrefix, settingsKey;
  bool checkBox = false, showInFileManager = false;
};

} // namespace gui
} // namespace atools

#endif // ATOOLS_MESSAGEBOX_H
