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

#ifndef LNM_SQLQUERYDIALOG_H
#define LNM_SQLQUERYDIALOG_H

#include <QDialog>

class QAbstractButton;

namespace Ui {
class SqlQueryDialog;
}

namespace atools {
namespace sql {
class SqlDatabase;
class SqlColumn;
}

namespace gui {

class SqlQueryModel;
class ItemViewZoomHandler;

/* Callback to modify value display or alignment. Only Qt::TextAlignmentRole and Qt::DisplayRole  */
typedef std::function<QVariant(int column, const QVariant& data, Qt::ItemDataRole role)> SqlQueryDialogDataFunc;

/*
 * Dialog class which allows to show the result of a SQL query in a table view. No editing.
 */
class SqlQueryDialog :
  public QDialog
{
  Q_OBJECT

public:
  /* settingsPrefixParam is used to save the dialog and item check state.
   * description: Label text above table.
   * helpBaseUrlParam is the base URL of the help system. Help button will be hidden if empty.
   * confirmationButtonText: Ok button text. */
  explicit SqlQueryDialog(QWidget *parent, const QString& title, const QString& description, const QString& settingsPrefixParam,
                          const QString& helpBaseUrlParam = QString(), const QString& confirmationButtonText = QString());
  virtual ~SqlQueryDialog() override;

  /* Initializes query and prepares dialog to show. Uses given columns and column headers.
   * Shows nothing if query result count is zero. Dialog shows a close button and returns rejected in this case.
   * Call this before exec(). Dialog can only be used once. */
  void initQuery(atools::sql::SqlDatabase *db, const QString& queryString, const QVector<atools::sql::SqlColumn>& columns,
                 const SqlQueryDialogDataFunc& dataFunc = nullptr);

private:
  void buttonBoxClicked(QAbstractButton *button);
  void restoreState();
  void saveState();

  Ui::SqlQueryDialog *ui;

  QString helpBaseUrl, settingsPrefix, helpOnlineUrl, helpLanguageOnline;

  atools::gui::ItemViewZoomHandler *zoomHandler = nullptr;
};

} // namespace gui
} // namespace atools

#endif // LNM_SQLQUERYDIALOG_H
