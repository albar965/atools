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

#include "gui/treedialog.h"

#include "atools.h"
#include "gui/griddelegate.h"
#include "gui/helphandler.h"
#include "gui/itemviewzoomhandler.h"
#include "gui/widgetstate.h"
#include "settings/settings.h"
#include "ui_treedialog.h"
#include "exception.h"

#include <QMimeData>
#include <QPushButton>
#include <QClipboard>
#include <QDebug>
#include <QCheckBox>

namespace atools {
namespace gui {

TreeDialog::TreeDialog(QWidget *parent, const QString& title, const QString& description, const QString& settingsPrefixParam,
                       const QString& helpBaseUrlParam, bool showExpandCollapse)
  : QDialog(parent), ui(new Ui::TreeDialog), helpBaseUrl(helpBaseUrlParam), settingsPrefix(settingsPrefixParam)
{
  ui->setupUi(this);
  setWindowTitle(title);
  setWindowFlag(Qt::WindowContextHelpButtonHint, false);
  setWindowModality(Qt::ApplicationModal);

  zoomHandler = new atools::gui::ItemViewZoomHandler(ui->treeWidget);
  gridDelegate = new atools::gui::GridDelegate(ui->treeWidget);
  ui->treeWidget->setItemDelegate(gridDelegate);

  // Hide label if text is empty
  ui->labelTreeDescription->setVisible(!description.isEmpty());
  ui->labelTreeDescription->setText(description);

  // Show expand/collapse buttons
  ui->pushButtonTreeExpandAll->setVisible(showExpandCollapse);
  ui->pushButtonTreeCollapseAll->setVisible(showExpandCollapse);

  if(helpBaseUrl.isEmpty())
    // Hide help button if not requested
    ui->buttonBoxTree->button(QDialogButtonBox::Help)->hide();

  // Ok button is default
  ui->buttonBoxTree->button(QDialogButtonBox::Ok)->setDefault(true);

  // Catch checkbox changes
  connect(ui->treeWidget->model(), &QAbstractItemModel::dataChanged, this, &TreeDialog::dataChanged);

  // Button box
  connect(ui->buttonBoxTree, &QDialogButtonBox::clicked, this, &TreeDialog::buttonBoxClicked);

  // Connect push buttons
  connect(ui->pushButtonTreeSelectAll, &QPushButton::clicked, this, &TreeDialog::checkAll);
  connect(ui->pushButtonTreeSelectNone, &QPushButton::clicked, this, &TreeDialog::unCheckAll);
  connect(ui->pushButtonTreeExpandAll, &QPushButton::clicked, ui->treeWidget, &QTreeWidget::expandAll);
  connect(ui->pushButtonTreeCollapseAll, &QPushButton::clicked, ui->treeWidget, &QTreeWidget::collapseAll);

  zoomHandler->zoomPercent(100);
}

TreeDialog::~TreeDialog()
{
  // Always save header, expand state and size
  saveStateDialog(true /* saveExpand State */);
  ui->treeWidget->setItemDelegate(nullptr);
  delete gridDelegate;
  delete zoomHandler;
  delete ui;
}

void TreeDialog::dataChanged(const QModelIndex& topLeft, const QModelIndex&, const QList<int>& roles)
{
  // Only check state changes
  if(roles.contains(Qt::CheckStateRole) && topLeft.isValid())
  {
    // Get id from user data. Only top left index since widget is single select anyway
    bool ok;
    int id = ui->treeWidget->model()->data(topLeft, Qt::UserRole).toInt(&ok);

    if(ok)
    {
      // Item from index
      const QTreeWidgetItem *item = index.value(id, nullptr);
      if(item != nullptr)
      {
#ifdef DEBUG_INFORMATION
        qDebug() << Q_FUNC_INFO << id << item->text(0) << item->checkState(0);
#endif

        // Notify receivers
        emit itemToggled(this, id, item->checkState(0) == Qt::Checked);
      }
    }
  }
}

void TreeDialog::checkAll()
{
  for(auto it = index.begin(); it != index.end(); ++it)
    it.value()->setCheckState(0, Qt::Checked);
}

void TreeDialog::unCheckAll()
{
  for(auto it = index.begin(); it != index.end(); ++it)
    it.value()->setCheckState(0, Qt::Unchecked);
}

void TreeDialog::setHeader(const QStringList& header)
{
  // resize columns and set header
  ui->treeWidget->setColumnCount(header.size());
  ui->treeWidget->setHeaderLabels(header);
}

void TreeDialog::resizeToContents()
{
  // Resize all columns
  for(int col = 0; col < ui->treeWidget->columnCount(); col++)
    ui->treeWidget->resizeColumnToContents(col);
}

QTreeWidgetItem *TreeDialog::getRootItem() const
{
  return ui->treeWidget->invisibleRootItem();
}

QTreeWidgetItem *TreeDialog::addTopItem(const QStringList& text, const QString& tooltip)
{
  QTreeWidgetItem *item = new QTreeWidgetItem(getRootItem(), text);

  // Set tooltip on all colums
  for(int col = 0; col < ui->treeWidget->columnCount(); col++)
  {
    item->setToolTip(col, tooltip);

    if(col == 0)
    {
      QFont font = item->font(col);
      font.setBold(true);
      item->setFont(col, font);
    }
  }

  // Check state depends on children
  item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsAutoTristate | Qt::ItemIsEnabled);
  return item;
}

void TreeDialog::setAllChecked(bool checked)
{
  for(auto it = index.begin(); it != index.end(); ++it)
    it.value()->setCheckState(0, checked ? Qt::Checked : Qt::Unchecked);
}

QTreeWidgetItem *TreeDialog::addItemInt(QTreeWidgetItem *parent, int id, const QStringList& text, const QString& tooltip, bool checked)
{
  if(index.contains(id))
    throw atools::Exception(tr("Duplicate id %1 found while adding item in TreeDialog").arg(id));

  QTreeWidgetItem *item = new QTreeWidgetItem(parent, text, id);
  item->setCheckState(0, checked ? Qt::Checked : Qt::Unchecked);

  // Set tooltip on all colums
  for(int col = 0; col < ui->treeWidget->columnCount(); col++)
    item->setToolTip(col, tooltip);

  item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
  item->setData(0, Qt::UserRole, id);
  item->setData(1, Qt::UserRole, id);

  // Add to index for saving and loading
  index.insert(id, item);

  return item;
}

bool TreeDialog::isCheckedInt(int id) const
{
  if(index.contains(id))
    return index.value(id)->checkState(0) == Qt::Checked;

  return false;
}

void TreeDialog::setCheckedInt(int id, bool checked)
{
  if(index.contains(id))
    index.value(id)->setCheckState(0, checked ? Qt::Checked : Qt::Unchecked);
}

bool TreeDialog::isDisabledInt(int id) const
{
  if(index.contains(id))
    return index.value(id)->isDisabled();

  return false;
}

void TreeDialog::setDisabledInt(int id, bool disabled)
{
  if(index.contains(id))
    index.value(id)->setDisabled(disabled);
}

void TreeDialog::buttonBoxClicked(QAbstractButton *button)
{
  QDialogButtonBox::StandardButton buttonType = ui->buttonBoxTree->standardButton(button);

  if(buttonType == QDialogButtonBox::Ok)
    accept();
  else if(buttonType == QDialogButtonBox::Cancel)
    reject();
  else if(buttonType == QDialogButtonBox::Help)
  {
    if(!helpBaseUrl.isEmpty())
      // Open help but keep dialog open
      atools::gui::HelpHandler::openHelpUrlWeb(this, helpOnlineUrl + helpBaseUrl, helpLanguageOnline);
  }
}

void TreeDialog::restoreState(bool restoreCheckState, bool restoreExpandState)
{
  atools::gui::WidgetState widgetState(settingsPrefix, false);

  // Reset size to contents if not previously saved
  bool reset = !widgetState.contains(ui->treeWidget);
  widgetState.restore({this, ui->treeWidget});

  if(restoreCheckState)
  {
    // Restore checkboxes from list of saved ids
    QStringList ids = atools::settings::Settings::instance().valueStrList(settingsPrefix + "TreeWidgetStates");
    for(int i = 0; i < ids.size(); i += 2)
    {
      int id = ids.at(i).toInt();
      if(index.contains(id))
        index.value(id)->setCheckState(0, static_cast<Qt::CheckState>(ids.at(i + 1).toInt()));
    }
  }

  if(restoreExpandState)
  {
    const QList<int> expandedIndexes = atools::strListToNumVector<int>(
      atools::settings::Settings::instance().valueStrList(settingsPrefix + "TreeWidgetExpandedStates"));

    QTreeWidgetItem *root = ui->treeWidget->invisibleRootItem();
    for(int i : expandedIndexes)
    {
      // Expand widgets from list of saved indexes
      QTreeWidgetItem *child = root->child(i);

      if(child != nullptr && child->childCount() > 0)
        child->setExpanded(true);
    }
  }

  if(reset)
    // Not previously saved
    resizeToContents();
}

void TreeDialog::saveStateDialog(bool saveExpandState)
{
  atools::gui::WidgetState widgetState(settingsPrefix, false);
  widgetState.save(QList<const QObject *>({this, ui->treeWidget}));

  if(saveExpandState)
  {
    // Save indexes of expanded widgets
    QTreeWidgetItem *root = ui->treeWidget->invisibleRootItem();
    QStringList expandedIndexes;
    for(int i = root->childCount() - 1; i >= 0; i--)
    {
      QTreeWidgetItem *child = root->child(i);

      if(child != nullptr && child->isExpanded())
        expandedIndexes.append(QString::number(i));
    }
    atools::settings::Settings::instance().setValue(settingsPrefix + "TreeWidgetExpandedStates", expandedIndexes);
  }
}

void TreeDialog::saveState(bool saveCheckState, bool saveExpandState)
{
  saveStateDialog(saveExpandState);

  if(saveCheckState)
  {
    // Save checkboxes in a list of id 1, checked 1, id 2, checked 2, ...
    QStringList ids;
    for(auto it = index.constBegin(); it != index.constEnd(); ++it)
      ids << QString::number(it.key()) << QString::number(it.value()->checkState(0));
    atools::settings::Settings::instance().setValue(settingsPrefix + "TreeWidgetStates", ids);
  }
}

} // namespace gui
} // namespace atools
