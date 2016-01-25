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

#include "fs/writer/writerbasebasic.h"
#include "fs/writer/datawriter.h"
#include "sql/sqldatabase.h"
#include "sql/sqlutil.h"

namespace atools {
namespace fs {
namespace writer {

using atools::sql::SqlUtil;
using atools::sql::SqlQuery;

WriterBaseBasic::WriterBaseBasic(atools::sql::SqlDatabase& sqlDb,
                                 DataWriter& writer,
                                 const QString& table,
                                 const QString& sqlParam)
  : tablename(table), db(sqlDb), dataWriter(writer)
{
  if(sqlParam.isEmpty())
    sql = SqlUtil(&db).buildInsertStatement(tablename);
  else
    sql = sqlParam;
  stmt = SqlQuery(db);
  stmt.prepare(sql);
}

WriterBaseBasic::~WriterBaseBasic()
{
}

const BglReaderOptions& WriterBaseBasic::getOptions()
{
  return dataWriter.getOptions();
}

RunwayIndex *WriterBaseBasic::getRunwayIndex()
{
  return dataWriter.getRunwayIndex();
}

AirportIndex *WriterBaseBasic::getAirportIndex()
{
  return dataWriter.getAirportIndex();
}

void WriterBaseBasic::executeStatement()
{
  stmt.exec();
  int numUpdated = stmt.numRowsAffected();
  if(numUpdated == 0)
    throw atools::sql::SqlException("Noting inserted", sql);
  // stmt.clear();

  dataWriter.increaseNumObjects();
}

void WriterBaseBasic::clearStatement()
{
  stmt.clear();
}

} // namespace writer
} // namespace fs
} // namespace atools
