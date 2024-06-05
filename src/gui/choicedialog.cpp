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

#include "choicedialog.h"
#include "ui_choicedialog.h"
#include "gui/helphandler.h"
#include "gui/widgetstate.h"
#include  "settings/settings.h"

#include <QMimeData>
#include <QPushButton>
#include <QClipboard>
#include <QDebug>
#include <QTextDocumentFragment>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>

namespace atools {
namespace gui {

const static char ID_PROPERTY[] = "button_id";

ChoiceDialog::ChoiceDialog(QWidget *parent, const QString& title, const QString& description, const QString& settingsPrefixParam,
                           const QString& helpBaseUrlParam)
  : QDialog(parent), ui(new Ui::ChoiceDialog), helpBaseUrl(helpBaseUrlParam), settingsPrefix(settingsPrefixParam)
{
  ui->setupUi(this);
  setWindowTitle(title);
  setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setWindowModality(Qt::ApplicationModal);

  ui->labelChoiceDescription->setVisible(!description.isEmpty());
  ui->labelChoiceDescription->setText(description);

  if(helpBaseUrl.isEmpty())
    // Remove help button if not requested
    ui->buttonBoxChoice->removeButton(ui->buttonBoxChoice->button(QDialogButtonBox::Help));

  ui->buttonBoxChoice->button(QDialogButtonBox::Ok)->setDefault(true);

  connect(ui->buttonBoxChoice, &QDialogButtonBox::clicked, this, &ChoiceDialog::buttonBoxClicked);
  updateButtonBoxState();
}

ChoiceDialog::~ChoiceDialog()
{
  // Save only dialog dimensions
  atools::gui::WidgetState widgetState(settingsPrefix, false);
  widgetState.save(this);

  delete ui;
}

void ChoiceDialog::addCheckBoxHiddenInt(int id)
{
  addCheckBoxInt(id, QString(), QString(), false /* checked*/, true /* disabled */, true /* hidden */);
}

void ChoiceDialog::addCheckBoxInt(int id, const QString& text, const QString& tooltip, bool checked, bool disabled,
                                  bool hidden)
{
  QCheckBox *button = new QCheckBox(text, this);
  button->setToolTip(tooltip);
  button->setStatusTip(tooltip);
  button->setProperty(ID_PROPERTY, id);
  button->setChecked(checked);
  button->setDisabled(disabled);
  button->setHidden(hidden);
  index.insert(id, button);
  connect(button, &QCheckBox::toggled, this, &ChoiceDialog::buttonToggledInternal);

  // Add widget before the button box and verticalSpacerChoice
  ui->verticalLayoutScrollArea->insertWidget(-1, button);
}

void ChoiceDialog::addRadioButtonHiddenInt(int id, int groupId)
{
  addRadioButtonInt(id, groupId, QString(), QString(), false /* checked*/, true /* disabled */, true /* hidden */);
}

void ChoiceDialog::addRadioButtonInt(int id, int groupId, const QString& text, const QString& tooltip, bool checked, bool disabled,
                                     bool hidden)
{
  QRadioButton *button = new QRadioButton(text, this);
  button->setToolTip(tooltip);
  button->setStatusTip(tooltip);
  button->setProperty(ID_PROPERTY, id);
  button->setChecked(checked);
  button->setDisabled(disabled);
  button->setHidden(hidden);
  index.insert(id, button);
  connect(button, &QRadioButton::toggled, this, &ChoiceDialog::buttonToggledInternal);

  QButtonGroup *buttonGroup = buttonGroups.value(groupId);
  if(buttonGroup == nullptr)
  {
    buttonGroup = new QButtonGroup(this);
    buttonGroups.insert(groupId, buttonGroup);
  }

  buttonGroup->addButton(button);

  // Add widget before the button box and verticalSpacerChoice
  ui->verticalLayoutScrollArea->insertWidget(-1, button);
}

void ChoiceDialog::addLine()
{
  QFrame *line;
  line = new QFrame(this);
  line->setFrameShape(QFrame::HLine);
  line->setFrameShadow(QFrame::Sunken);
  ui->verticalLayoutScrollArea->insertWidget(-1, line);
}

void ChoiceDialog::addLabel(const QString& text)
{
  ui->verticalLayoutScrollArea->insertWidget(-1, new QLabel(text, this));
}

void ChoiceDialog::addSpacer()
{
  ui->verticalLayoutScrollArea->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding)); // Move button up with spacer
}

const QVector<std::pair<int, bool> > ChoiceDialog::getCheckState() const
{
  QVector<std::pair<int, bool> > ids;
  for(QAbstractButton *button : index)
    ids.append(std::make_pair(button->property(ID_PROPERTY).toInt(), button->isChecked()));
  return ids;
}

bool ChoiceDialog::isCheckedInt(int id) const
{
  return index.value(id)->isChecked() && index.value(id)->isEnabled();
}

QAbstractButton *ChoiceDialog::getButtonInt(int id) const
{
  return index.value(id);
}

void ChoiceDialog::buttonBoxClicked(QAbstractButton *button)
{
  QDialogButtonBox::StandardButton buttonType = ui->buttonBoxChoice->standardButton(button);

  if(buttonType == QDialogButtonBox::Ok)
  {
    saveState();
    accept();
  }
  else if(buttonType == QDialogButtonBox::Cancel)
    reject();
  else if(buttonType == QDialogButtonBox::Help)
  {
    if(!helpBaseUrl.isEmpty())
      atools::gui::HelpHandler::openHelpUrlWeb(this, helpOnlineUrl + helpBaseUrl, helpLanguageOnline);
  }
}

void ChoiceDialog::buttonToggledInternal(bool checked)
{
  updateButtonBoxState();
  QAbstractButton *button = dynamic_cast<QAbstractButton *>(sender());
  if(button != nullptr)
    emit buttonToggled(button->property(ID_PROPERTY).toInt(), checked);
}

void ChoiceDialog::restoreState()
{
  atools::gui::WidgetState widgetState(settingsPrefix, false);
  widgetState.restore(this);

  // Restore buttons
  QStringList ids = atools::settings::Settings::instance().valueStrList(settingsPrefix + "ButtonStates");
  for(int i = 0; i < ids.size(); i += 2)
  {
    int id = ids.at(i).toInt();
    bool checked = ids.at(i + 1).toInt() > 0;

    if(index.value(id) != nullptr)
    {
      index.value(id)->setChecked(checked);
      emit buttonToggled(id, checked);
    }
  }
  updateButtonBoxState();
}

void ChoiceDialog::saveState() const
{
  atools::gui::WidgetState widgetState(settingsPrefix, false);
  widgetState.save(this);

  // Save buttons in a list of id 1, checked 1, id 2, checked 2, ...
  QStringList ids;
  for(const std::pair<int, bool>& state : getCheckState())
    ids << QString::number(state.first) << QString::number(state.second);

  atools::settings::Settings::instance().setValue(settingsPrefix + "ButtonStates", ids);
}

void ChoiceDialog::updateButtonBoxState()
{
  if(!required.isEmpty())
  {
    bool found = false;
    for(int i : qAsConst(required))
    {
      if(index.contains(i) && index.value(i)->isChecked())
        found = true;
    }

    ui->buttonBoxChoice->button(QDialogButtonBox::Ok)->setEnabled(found);
  }
  else
    ui->buttonBoxChoice->button(QDialogButtonBox::Ok)->setEnabled(true);
}

} // namespace gui
} // namespace atools
