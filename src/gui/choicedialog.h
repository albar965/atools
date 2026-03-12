/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_CHOICEDIALOG_H
#define ATOOLS_CHOICEDIALOG_H

#include <QAbstractButton>
#include <QDialog>
#include <QFrame>
#include <QSet>

namespace Ui {
class ChoiceDialog;
}

class QButtonGroup;
class QAbstractButton;
class QCheckBox;
class QRadioButton;

namespace atools {
namespace gui {

/*
 * A configurable dialog that shows the user a list of checkboxes and/or radio buttons.
 * Widgets are identified by an id/enum which should be constant since it is used to save the widget state.
 */
class ChoiceDialog :
  public QDialog
{
  Q_OBJECT

public:
  /* settingsPrefixParam is used to save the dialog and button state.
   * helpBaseUrlParam is the base URL of the help system. Help button will be hidden if empty.*/
  ChoiceDialog(QWidget *parent, const QString& title, const QString& description, const QString& settingsPrefixParam,
               const QString& helpBaseUrlParam);
  virtual ~ChoiceDialog() override;

  ChoiceDialog(const ChoiceDialog& other) = delete;
  ChoiceDialog& operator=(const ChoiceDialog& other) = delete;

  /* Basic id and group types big enough to allow flags. All template ids TYPE are cast to these. */
  typedef long long ChoiceDialogIdType;
  typedef int ChoiceDialogGroupType;

  /* Add a widget with the given id. ChoiceDialog does not take ownership of the widget but
   * its state and content is saved and restored. */
  template<typename TYPE>
  void addWidget(TYPE id, QWidget *widget)
  {
    addWidgetById(static_cast<ChoiceDialogIdType>(id), widget);
  }

  /* Add a checkbox with the given id, text and tooltip */
  template<typename TYPE>
  void addCheckBox(TYPE id, const QString& text, const QString& tooltip = QString(), bool checked = false,
                   bool disabled = false, bool hidden = false)
  {
    addCheckBoxById(static_cast<ChoiceDialogIdType>(id), text, tooltip, checked, disabled, hidden);
  }

  /* Shortcut to add a hidden, disabled and unchecked widget.
   * Useful if different configurations are saved in the same setting variable. */
  template<typename TYPE>
  void addCheckBoxHidden(TYPE id)
  {
    addCheckBoxHiddenById(static_cast<ChoiceDialogIdType>(id));
  }

  /* Add a radio button with the given id, group text and tooltip.
   * A new button group is created for each new groupId. */
  template<typename TYPE, typename GROUP>
  void addRadioButton(TYPE id, GROUP groupId, const QString& text, const QString& tooltip = QString(), bool checked = false,
                      bool disabled = false, bool hidden = false)
  {
    addRadioButtonById(static_cast<ChoiceDialogIdType>(id), static_cast<ChoiceDialogIdType>(groupId), text, tooltip, checked, disabled,
                       hidden);
  }

  /* Shortcut to add a hidden, disabled and unchecked widget.
   * Useful if different configurations are saved in the same setting variable. */
  template<typename TYPE, typename GROUP>
  void addRadioButtonHidden(TYPE id, GROUP groupId)
  {
    addRadioButtonHiddenById(static_cast<ChoiceDialogIdType>(id), static_cast<ChoiceDialogIdType>(groupId));
  }

  /* Enable or disable any widget added */
  template<typename TYPE>
  void enableWidget(TYPE id, bool enable = true)
  {
    getWidgetById(static_cast<ChoiceDialogIdType>(id))->setEnabled(enable);
  }

  template<typename TYPE>
  void disableWidget(TYPE id, bool disable = true)
  {
    enableWidget(id, !disable);
  }

  /* true if box for id is checked and enabled. Not for plain widgets added with addWidget(). */
  template<typename TYPE>
  bool isButtonChecked(TYPE id) const
  {
    return isCheckedById(static_cast<ChoiceDialogIdType>(id));
  }

  /* ok button is enabled if at least one button for these ids is checked.
   * Not for plain widgets added with addWidget(). */
  void setRequiredAnyChecked(QSet<ChoiceDialogIdType> ids)
  {
    required = ids;
  }

  /* Add a horizontal separator line. Use "QApplication::palette().color(QPalette::Mid)" for normal frame color. */
  void addLine(QFrame::Shadow shadow = QFrame::Sunken, int width = -1, const QColor& color = QColor());

  /* Add a vertical spacer */
  void addSpacer();

  /* Add label text */
  void addLabel(const QString& text);

  /* Call after adding all buttons to restore button state.
   * Sends buttonToggled() for each checkbox or radio button.
   * Uses assigned ids to identify widgets. */
  void restoreState();

  void setHelpOnlineUrl(const QString& value)
  {
    helpOnlineUrl = value;
  }

  void setHelpLanguageOnline(const QString& value)
  {
    helpLanguageOnline = value;
  }

signals:
  /* Emitted when a button is toggled */
  void buttonToggled(ChoiceDialogIdType id, bool checked);

private:
  void buttonBoxClicked(QAbstractButton *button);
  void buttonToggledInternal(bool checked);
  void saveState() const;
  void updateButtonBoxState();

  /* Internal methods using integer instead of enum as id */
  bool isCheckedById(ChoiceDialogIdType id) const;
  void addWidgetById(ChoiceDialogIdType id, QWidget *widget);
  void addCheckBoxById(ChoiceDialogIdType id, const QString& text, const QString& tooltip = QString(), bool checked = false,
                       bool disabled = false, bool hidden = false);
  void addCheckBoxHiddenById(ChoiceDialogIdType id);

  void addRadioButtonById(ChoiceDialogIdType id, ChoiceDialogGroupType groupId, const QString& text, const QString& tooltip = QString(),
                          bool checked = false, bool disabled = false, bool hidden = false);
  void addRadioButtonHiddenById(ChoiceDialogIdType id, ChoiceDialogGroupType groupId);

  QAbstractButton *getButtonById(ChoiceDialogIdType id) const
  {
    return dynamic_cast<QAbstractButton *>(getWidgetById(id));
  }

  QWidget *getWidgetById(ChoiceDialogIdType id) const
  {
    return index.value(id, nullptr);
  }

  Ui::ChoiceDialog *ui;
  QString helpBaseUrl, settingsPrefix, helpOnlineUrl, helpLanguageOnline;

  /* Maps user given id to widget. */
  QHash<ChoiceDialogIdType, QWidget *> index;

  /* Maps groupId to button groups */
  QHash<ChoiceDialogGroupType, QButtonGroup *> buttonGroups;

  /* List of all buttons that have to be checked to enable the ok button */
  QSet<ChoiceDialogIdType> required;
};

} // namespace gui
} // namespace atools

#endif // ATOOLS_CHOICEDIALOG_H
