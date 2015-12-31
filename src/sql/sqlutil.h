/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_SQL_SQLUTIL_H
#define ATOOLS_SQL_SQLUTIL_H

#include "sql/sqldatabase.h"
#include "sql/sqlexception.h"

#include <functional>
#include <QTextStream>

namespace atools {
namespace sql {

class SqlUtil
{
public:
  /*
   * Create a util using the given database.
   */
  SqlUtil(SqlDatabase *sqlDb);

  /*
   * Prints row counts for all tables in the database.
   * @param out QDebug, QTextStream or std::iostream object. The stream needs
   * to have an operator<< for QString.
   * @param endline Print endline or not. Useful for streams that automatically
   * insert endlines like QDebug.
   */
  template<class OUT>
  void printTableStats(OUT& out, bool endline = true);

  /* Creates an insert statement including all columns for the given table. */
  QString buildInsertStatement(const QString& tablename);

  /* Creates a select statement including all columns for the given table. */
  QString buildSelectStatement(const QString& tablename);

  /* @return true if table exists and has rows */
  bool hasTableAndRows(const QString& tablename);

  /* Copy all values from one query to another
   * @param from a valid query as data source
   * @param to a valid prepared query having the same number of bind variables
   * as the from query columns
   * @param func Function that acts a filter. If return value is true the row is
   * inserted. Will be called after all variables are bound.
   * @return number of rows copied
   */
  static int copyResultValues(SqlQuery& from,
                              SqlQuery& to,
                              std::function<bool(SqlQuery& from, SqlQuery& to)> func);

  /*
   * Copies the values of one row from one statement to another.
   * @param from valid query as data source
   * @param to a valid prepared query having the same number of bind variables
   * as the from query columns
   */
  static void copyRowValues(const SqlQuery& from, SqlQuery& to);

private:
  SqlDatabase *db;

};

// -----------------------------------------------

template<class OUT>
void SqlUtil::printTableStats(OUT& out, bool endline)
{
  out << "Statistics for database (tables / rows):";
  if(endline)
    out << endl;

  SqlQuery query(db);

  int index = 1;

  for(QString name : db->tables())
  {
    query.exec("select count(1) as cnt from " + name);
    if(query.next())
    {
      out << "#" << (index++) << " " << name << ": "
          << query.value("cnt").toInt() << " rows";
      if(endline)
        out << endl;
    }
  }
}

} // namespace sql
} // namespace atools

#endif // ATOOLS_SQL_SQLUTIL_H
