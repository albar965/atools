/*
 * WriterBaseBasic.cpp
 *
 *  Created on: 01.06.2015
 *      Author: alex
 */

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
