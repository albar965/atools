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
 * Message box class which allows to add buttons like help which do not close the dialog window.
 * Clicking the help button will open the related URL but not close the dialog.
 */
class MessageBox :
  public QDialog
{
  Q_OBJECT

public:
  explicit MessageBox(QWidget *parent, const QString& title, const QString& helpBaseUrlParam);
  virtual ~MessageBox() override;

  /* Adjusts contents before exec() */
  virtual int exec() override;

  void setHelpOnlineUrl(const QString& value)
  {
    helpOnlineUrl = value;
  }

  void setHelpLanguageOnline(const QString& value)
  {
    helpLanguageOnline = value;
  }

  /* Add a standard button which will call accept() when clicked */
  void addAcceptButton(QDialogButtonBox::StandardButton button);

  /* Add a standard button which will call reject() when clicked */
  void addRejectButton(QDialogButtonBox::StandardButton button);

  /* Add a standard button which will keep the dialog open when clicked. Help triggers the help URL. */
  void addButton(QDialogButtonBox::StandardButton button);

  /* Set text for right aligned label */
  void setText(const QString& text);

  /* Set a standard icon for the top left label.  */
  void setIcon(QMessageBox::Icon dialogIcon);

private:
  Ui::MessageBox *ui;
  void buttonBoxClicked(QAbstractButton *button);

  QVector<QDialogButtonBox::StandardButton> acceptButtons;
  QVector<QDialogButtonBox::StandardButton> rejectButtons;

  QString helpBaseUrl, settingsPrefix, helpOnlineUrl, helpLanguageOnline;

};

} // namespace gui
} // namespace atools

#endif // ATOOLS_MESSAGEBOX_H
