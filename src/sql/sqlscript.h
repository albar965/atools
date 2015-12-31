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

#ifndef ATOOLS_SQL_SQLSCRIPT_H
#define ATOOLS_SQL_SQLSCRIPT_H

#include "sql/sqldatabase.h"

class QTextStream;

namespace atools {
namespace sql {

/*
 * Runs full SQL scripts. Allows SQL line comments "--" and C block comments
 * (slash-star).
 * Semicolon is used to mark end of statement and execute it. Commits are not
 * executed after each statement. These have to be added to the SQL script.
 *
 * The script commands and results are logged in the qInfo channel. SqlException
 * is thrown in case of error.
 *
 * Complex SQL as Oracle PL/SQL is not supported.
 */
class SqlScript
{
public:
  /*
   * @param sqlDb Database to use
   */
  SqlScript(SqlDatabase *sqlDb);

  /* Run a script provided in the given filename */
  void executeScript(const QString& filename);

  /* Read script from stream and execute it */
  void executeScript(QTextStream& script);

private:
  struct ScriptCmd
  {
    QString sql;
    int lineNumber;
  };

  /* Extract line number / SQL statement pairs from the script */
  void parseSqlScript(QTextStream& script, QList<ScriptCmd>& statements);

  SqlDatabase *db;
};

} // namespace sql
} // namespace atools

#endif // ATOOLS_SQL_SQLSCRIPT_H
