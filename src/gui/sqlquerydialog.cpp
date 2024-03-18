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

#include "gui/sqlquerydialog.h"

#include "gui/helphandler.h"
#include "gui/itemviewzoomhandler.h"
#include "gui/widgetstate.h"
#include "sql/sqlcolumn.h"
#include "sql/sqldatabase.h"
#include "ui_sqlquerydialog.h"

#include <QPushButton>
#include <QSqlQueryModel>
#include <QDebug>
#include <QStringListModel>

namespace atools {
namespace gui {

/* Simple query model used to modify data values and alignment by callback */
class SqlQueryModel :
  public QSqlQueryModel
{
public:
  explicit SqlQueryModel(QObject *parent)
    : QSqlQueryModel(parent)
  {
  }

  SqlQueryDialogDataFunc dataFunc;

private:
  virtual QVariant data(const QModelIndex& index, int role) const override;

};

QVariant SqlQueryModel::data(const QModelIndex& index, int role) const
{
  if(index.isValid() && dataFunc && (role == Qt::TextAlignmentRole || role == Qt::DisplayRole))
    // Call callback
    return dataFunc(index.column(), QSqlQueryModel::data(index, Qt::DisplayRole), static_cast<Qt::ItemDataRole>(role));

  // Return default by base class
  return QSqlQueryModel::data(index, role);
}

// ====================================================================================================
SqlQueryDialog::SqlQueryDialog(QWidget *parent, const QString& title, const QString& description, const QString& settingsPrefixParam,
                               const QString& helpBaseUrlParam, const QString& confirmationButtonText)
  : QDialog(parent), ui(new Ui::SqlQueryDialog), helpBaseUrl(helpBaseUrlParam), settingsPrefix(settingsPrefixParam)
{
  ui->setupUi(this);

  setWindowTitle(title);
  setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setWindowModality(Qt::ApplicationModal);

  zoomHandler = new atools::gui::ItemViewZoomHandler(ui->tableView);
  zoomHandler->zoomPercent(100);

  // Hide label if text is empty
  ui->label->setVisible(!description.isEmpty());
  ui->label->setText(description);

  if(helpBaseUrl.isEmpty())
    // Remove help button if not requested
    ui->buttonBox->button(QDialogButtonBox::Help)->hide();

  // Close is only shown for empty result
  ui->buttonBox->button(QDialogButtonBox::Close)->hide();

  // Ok button is default
  ui->buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

  if(!confirmationButtonText.isEmpty())
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(confirmationButtonText);

  // Button box
  connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &SqlQueryDialog::buttonBoxClicked);

  restoreState();
}

SqlQueryDialog::~SqlQueryDialog()
{
  saveState();
  delete zoomHandler;
  delete ui;
}

void SqlQueryDialog::initQuery(atools::sql::SqlDatabase *db, const QString& queryString, const QVector<sql::SqlColumn>& columns,
                               const SqlQueryDialogDataFunc& dataFunc)
{
  // Build query model based on SQL string - view is parent which deletes model
  SqlQueryModel *sqlQueryModel = new SqlQueryModel(ui->tableView);
  sqlQueryModel->dataFunc = dataFunc; // Data and aligment modification callback
  sqlQueryModel->setQuery(queryString, db->getQSqlDatabase());

  if(sqlQueryModel->rowCount() > 0)
  {
    // Results found - build header ============================
    int i = 0;
    for(const atools::sql::SqlColumn& col: columns)
      sqlQueryModel->setHeaderData(i++, Qt::Horizontal, col.getDisplayName());

    ui->tableView->setModel(sqlQueryModel);
    ui->tableView->show();
    ui->tableView->resizeColumnsToContents();
  }
  else
  {
    // Nothing found ============================
    delete sqlQueryModel;

    // Remove selection, disable and hide headers
    ui->tableView->setSelectionMode(QAbstractItemView::NoSelection);
    ui->tableView->clearSelection();
    ui->tableView->setDisabled(true);

    ui->tableView->horizontalHeader()->hide();
    ui->tableView->verticalHeader()->hide();

    ui->buttonBox->button(QDialogButtonBox::Ok)->hide();
    ui->buttonBox->button(QDialogButtonBox::Cancel)->hide();
    ui->buttonBox->button(QDialogButtonBox::Close)->show();
    ui->buttonBox->button(QDialogButtonBox::Close)->setDefault(true);

    ui->label->setText(tr("Nothing found to delete."));
  }
}

void SqlQueryDialog::buttonBoxClicked(QAbstractButton *button)
{
  QDialogButtonBox::StandardButton buttonType = ui->buttonBox->standardButton(button);

  if(buttonType == QDialogButtonBox::Ok)
    accept();
  else if(buttonType == QDialogButtonBox::Cancel || buttonType == QDialogButtonBox::Close)
    // Nothing found case - always reject
    reject();
  else if(buttonType == QDialogButtonBox::Help)
  {
    if(!helpBaseUrl.isEmpty())
      // Open help but keep dialog open
      atools::gui::HelpHandler::openHelpUrlWeb(this, helpOnlineUrl + helpBaseUrl, helpLanguageOnline);
  }
}

void SqlQueryDialog::restoreState()
{
  atools::gui::WidgetState(settingsPrefix, false).restore(this);
}

void SqlQueryDialog::saveState() const
{
  atools::gui::WidgetState(settingsPrefix, false).save(this);
}

} // namespace gui
} // namespace atools
