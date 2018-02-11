/*****************************************************************************
* Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_SQL_SQLEXPORT_H
#define ATOOLS_SQL_SQLEXPORT_H

#include "sql/sqlquery.h"

#include <QString>

namespace atools {
namespace sql {

class SqlDatabase;

/*
 * Allows to export CSV files according to RFC:
 * https://tools.ietf.org/html/rfc4180
 *
 */
class SqlExport
{
public:
  SqlExport()
  {
  }

  /*
   * Writes the fully result set from the given query to the output stream.
   *
   * @param query A prepared and valid query to read values from
   * @param out QDebug, QTextStream or std::iostream object. The stream needs
   * to have an operator<< for QString.
   */
  template<class OUT>
  void printResultSet(SqlQuery& query, OUT& out);

  /* Write a header containing the column names from the result set or not */
  void setHeader(bool value)
  {
    header = value;
  }

  /* Write only this amount of rows. Default is -1 which means
   * write all rows.
   */
  void setMaxValues(int value)
  {
    maxValues = value;
  }

  /* Field separator character. According to RFC this is "," (the default),
   * but MS Excel needs ";" to allow opening the file. */
  void setSeparatorChar(QChar value)
  {
    separator = value;
  }

  /* Character used to wrap all string fields, that contain the separator
   * char, a linefeed or a carriage return. */
  void setEscapeChar(QChar value)
  {
    escapeString = value;
  }

  /* String to use to null database values. Empty string by default. */
  void setNullValue(const QString& value)
  {
    nullValue = value;
  }

  /* Print a end line after each row or not. Usefult if one of the
   * getResultSetHeader or getResultSetRow methods is used. Also useful for
   * streams like QDebug that insert newlines automatically. */
  void setEndline(bool value)
  {
    endline = value;
  }

  /* Build a header row from the given SQL record */
  QString getResultSetHeader(const SqlRecord& record) const;

  /* Build a header row from the given string list */
  QString getResultSetHeader(const QStringList& strings) const;

  /* Build a data row from the given SQL record */
  QString getResultSetRow(const SqlRecord& record) const;

  /* Build a data row from the given QVariant list */
  QString getResultSetRow(const QVariantList& values) const;

private:
  QString printEndl() const;
  QString buildString(QString value) const;
  QString printValue(QVariant value) const;

  bool endline = true, header = true;
  int maxValues = -1;
  QChar separator = ',', escapeString = '"';
  QString nullValue = "";
};

// -----------------------------------------------------------

template<class OUT>
void SqlExport::printResultSet(SqlQuery& query, OUT& out)
{
  bool first = true;

  int i = 0;
  while(query.next())
  {
    if(first && header)
    {
      first = false;
      out << getResultSetHeader(query.record());
    }
    if(i > maxValues && maxValues != -1)
      break;

    out << getResultSetRow(query.record());
    i++;
  }
}

} // namespace sql
} // namespace atools

#endif // ATOOLS_SQL_SQLEXPORT_H
