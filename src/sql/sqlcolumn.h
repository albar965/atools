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

#ifndef ATOOLS_SQLCOLUMN_H
#define ATOOLS_SQLCOLUMN_H

#include <QString>

namespace atools {
namespace sql {

/*
 * Defines a SQL result or table column with id, column name and header.
 */
class SqlColumn
{
public:
  SqlColumn()
    : id(-1)
  {
  }

  SqlColumn(int idParam, const QString& nameParam, const QString& displayNameParam) :
    id(idParam), name(nameParam), displayName(displayNameParam)
  {
  }

  SqlColumn(const QString& nameParam, const QString& displayNameParam) :
    id(-1), name(nameParam), displayName(displayNameParam)
  {
  }

  template<typename ID>
  ID getId() const
  {
    return static_cast<ID>(id);
  }

  /* Get statement for this column like 'column_name as "Column Title"' */
  QString getSelectStmt() const;

  /* Get statement for this column list like 'column_name_1 as "Column Title 1", column_name_2 as "Column Title 2"' */
  //static QString getColumnList(const QList<atools::sql::SqlColumn>& columns);
  static QString getColumnList(const QVector<atools::sql::SqlColumn>& columns);

  int getId() const
  {
    return id;
  }

  /* Database column name */
  const QString& getName() const
  {
    return name;
  }

  /* Title for display like table view headers, etc. */
  const QString& getDisplayName() const
  {
    return displayName;
  }

  bool isValid() const
  {
    return !name.isEmpty();
  }

private:
  int id;
  QString name, displayName;
};

} // namespace sql
} // namespace atools

#endif // ATOOLS_SQLCOLUMN_H
