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

#include "sql/sqltransaction.h"

#include "sql/sqldatabase.h"

#include <QDebug>
#include <QSqlError>

namespace atools {
namespace sql {

SqlTransaction::~SqlTransaction()
{
  if(rollbackNeeded)
  {
    // Use rollback in Qt database to avoid nested exceptions
    if(!db->db.rollback())
      qWarning() << Q_FUNC_INFO << "Rollback failed"
                 << db->lastError().databaseText() << db->lastError().driverText();

    if(db->isAutomaticTransactions())
    {
      // Reopen
      if(!db->db.transaction())
        qWarning() << Q_FUNC_INFO << "Transaction failed"
                   << db->lastError().databaseText() << db->lastError().driverText();
    }
  }
}

void SqlTransaction::start()
{
  // Does nothing for automatic transactions since already open
  rollbackNeeded = true;
  db->transaction();
}

void SqlTransaction::commit()
{
  rollbackNeeded = false;
  db->commit();
}

void SqlTransaction::rollback()
{
  rollbackNeeded = false;
  db->rollback();
}

} // namespace sql
} // namespace atools
