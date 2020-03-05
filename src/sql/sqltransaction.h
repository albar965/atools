/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_SQL_SQLTRANSACTION_H
#define ATOOLS_SQL_SQLTRANSACTION_H

namespace atools {
namespace sql {

class SqlDatabase;

/*
 * Helper class that can be used if automatic transactions are off.
 * Opens a transaction on instantiation.
 *
 * Automatically rolls back in destructor if e.g. an exception is thrown.
 */
class SqlTransaction
{
public:
  SqlTransaction(atools::sql::SqlDatabase& sqlDb);
  SqlTransaction(atools::sql::SqlDatabase *sqlDb);
  ~SqlTransaction();

  void commit();
  void rollback();

private:
  atools::sql::SqlDatabase *db;

  /* False if user called commit or rollback */
  bool rollbackNeeded = true;
};

} // namespace sql
} // namespace atools

#endif // ATOOLS_SQL_SQLTRANSACTION_H
