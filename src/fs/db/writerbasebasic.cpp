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

#include "fs/db/writerbasebasic.h"
#include "fs/db/datawriter.h"
#include "sql/sqldatabase.h"
#include "sql/sqlutil.h"

namespace atools {
namespace fs {
namespace db {

using atools::sql::SqlUtil;
using atools::sql::SqlQuery;

WriterBaseBasic::WriterBaseBasic(atools::sql::SqlDatabase& sqlDb,
                                 DataWriter& writer,
                                 const QString& table,
                                 const QString& sqlParam)
  : sqlQuery(sqlDb), tablename(table), db(sqlDb), dataWriter(writer)
{
  if(sqlParam.isEmpty())
    sqlStatement = SqlUtil(&db).buildInsertStatement(tablename);
  else
    sqlStatement = sqlParam;
  sqlQuery = SqlQuery(db);

  sqlQuery.prepare(sqlStatement);
}

WriterBaseBasic::~WriterBaseBasic()
{
}

const NavDatabaseOptions& WriterBaseBasic::getOptions()
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

void WriterBaseBasic::bindBool(const QString& placeholder, bool val)
{
  return sqlQuery.bindValue(placeholder, val ? 1 : 0);
}

void WriterBaseBasic::bind(const QString& placeholder, const QVariant& val)
{
  return sqlQuery.bindValue(placeholder, val);
}

void WriterBaseBasic::bindIntOrNull(const QString& placeholder, const QVariant& val)
{
  if(val.toInt() == 0)
    bindNullInt(placeholder);
  else
    return sqlQuery.bindValue(placeholder, val);
}

void WriterBaseBasic::bindCoordinateList(const QString& placeholder,
                                         const QList<bgl::BglPosition>& coordinates,
                                         int precision)
{
  QStringList list;
  for(const bgl::BglPosition& pos : coordinates)
    list.append(QString::number(pos.getLonX(), 'g', precision) + " " +
                QString::number(pos.getLatY(), 'g', precision));
  bind(placeholder, list.join(", "));
}

void WriterBaseBasic::bindNullInt(const QString& placeholder)
{
  return sqlQuery.bindValue(placeholder, QVariant(QVariant::Int));
}

void WriterBaseBasic::bindNullFloat(const QString& placeholder)
{
  return sqlQuery.bindValue(placeholder, QVariant(QVariant::Double));
}

void WriterBaseBasic::bindNullString(const QString& placeholder)
{
  return sqlQuery.bindValue(placeholder, QVariant(QVariant::String));
}

void WriterBaseBasic::executeStatement()
{
  sqlQuery.exec();
  int numUpdated = sqlQuery.numRowsAffected();
  if(numUpdated == 0)
    throw atools::sql::SqlException("Noting inserted", sqlStatement);

  dataWriter.increaseNumObjects();
}

} // namespace writer
} // namespace fs
} // namespace atools
