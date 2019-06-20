/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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

#include "fs/common/morareader.h"
#include "sql/sqlquery.h"
#include "sql/sqldatabase.h"
#include "sql/sqlutil.h"
#include "geo/pos.h"
#include "exception.h"

#include <QDataStream>

using atools::sql::SqlQuery;
using atools::sql::SqlUtil;

namespace atools {
namespace fs {
namespace common {

MoraReader::MoraReader(sql::SqlDatabase *sqlDb)
  : db(sqlDb)
{

}

MoraReader::MoraReader(sql::SqlDatabase& sqlDb)
  : db(&sqlDb)
{

}

MoraReader::~MoraReader()
{

}

bool MoraReader::readFromTable(atools::sql::SqlDatabase& sqlDb)
{
  db = &sqlDb;
  return readFromTable();
}

bool MoraReader::readFromTable()
{
  clear();

  if(!SqlUtil(db).hasTableAndRows("mora_grid"))
  {
    qWarning() << Q_FUNC_INFO << "No MORA data found";
    return false;
  }

  SqlQuery moraReadQuery(db);
  moraReadQuery.exec("select * from mora_grid");

  if(moraReadQuery.next())
  {
    // mora_grid_id integer primary key,
    // version integer not null,
    // lonx_spacing integer not null,
    // laty_spacing integer not null,
    // geometry blob not null
    lonxColums = moraReadQuery.valueInt("lonx_columns");
    latyRows = moraReadQuery.valueInt("laty_rows");
    QByteArray bytes = moraReadQuery.value("geometry").toByteArray();

    // Read data from blob
    QDataStream in(&bytes, QIODevice::ReadOnly);
    in.setVersion(QDataStream::Qt_5_5);
    in.setFloatingPointPrecision(QDataStream::SinglePrecision);

    // Read and check header
    quint32 magicNumber, dataVersion;
    in >> magicNumber >> dataVersion;

    if(magicNumber != MAGIC_NUMBER_DATA)
      throw Exception("Invalid magic number in MORA data");
    if(dataVersion != DATA_VERSION)
      throw Exception("Invalid data version in MORA data");

    // Read blob into vector
    quint16 value;
    while(!in.atEnd())
    {
      in >> value;
      datagrid.append(value);
    }

    // Check size
    if(datagrid.size() != lonxColums * latyRows)
      throw Exception("Invalid data size in MORA data");

    qInfo() << Q_FUNC_INFO << db->databaseName() << "MORA data loaded"
            << lonxColums << "x *" << latyRows << "y" << bytes.size() << "bytes";

    dataAvailable = true;
    return true;
  }
  else
    qWarning() << Q_FUNC_INFO << "No MORA data found";

  dataAvailable = false;
  return false;
}

void MoraReader::writeToTable(const QVector<quint16>& grid, int columns, int rows)
{
  clear();

  datagrid = grid;
  lonxColums = columns;
  latyRows = rows;
  dataAvailable = true;

  SqlQuery moraWriteQuery(db);
  moraWriteQuery.prepare(SqlUtil(db).buildInsertStatement("mora_grid", QString(), {"mora_grid_id"}));

  QByteArray bytes;
  QDataStream out(&bytes, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_5);
  out.setFloatingPointPrecision(QDataStream::SinglePrecision);

  // Write header
  out << MAGIC_NUMBER_DATA << DATA_VERSION;

  // Write data as 16 bit values
  for(quint16 value : datagrid)
    out << value;

  // mora_grid_id integer primary key,
  // version integer not null,
  // lonx_columns integer not null,
  // laty_rows integer not null,
  // geometry blob not null
  moraWriteQuery.bindValue(":version", DATA_VERSION);
  moraWriteQuery.bindValue(":lonx_columns", lonxColums);
  moraWriteQuery.bindValue(":laty_rows", latyRows);
  moraWriteQuery.bindValue(":geometry", bytes);
  moraWriteQuery.exec();
}

bool MoraReader::isDataAvailable()
{
  return dataAvailable;
}

void MoraReader::clear()
{
  datagrid.clear();
  lonxColums = latyRows = 0;
  dataAvailable = false;
}

int MoraReader::getMoraFt(const geo::Pos& pos) const
{
  return getMoraFt(static_cast<int>(pos.getLonX()), static_cast<int>(pos.getLatY()));
}

int MoraReader::getMoraFt(int lonx, int laty) const
{
  // The field will contain values expressed in hundreds of feet, for example,
  // the value of 6000 feet is expressed as 060 and the value of 7100 feet is expressed as 071.
  // For geographical sections that are not surveyed, the field will contain the alpha characters UNK for Unknown.

  if(!dataAvailable)
    throw Exception("MORA data not available");

  // Coordinates are top left corner of rectangle.
  // -180 <= x <= 179
  // -89 <= y <= 90

  // Adjust values
  while(laty < -89)
    laty += 180;
  while(laty > 90)
    laty -= 180;

  // Avoid invalid values in far north and south regions
  if(laty > 85)
    return OCEAN;

  if(laty < -85)
    return UNKNOWN;

  // Adjust values
  while(lonx < -180)
    lonx += 360;
  while(lonx > 179)
    lonx -= 360;

  int pos = (-laty + 90) * 360 + lonx + 180;

  return datagrid.at(pos);
}

} // namespace common
} // namespace fs
} // namespace atools
