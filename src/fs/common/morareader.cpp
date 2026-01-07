/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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
#include <QIODevice>

using atools::sql::SqlQuery;
using atools::sql::SqlUtil;

namespace atools {
namespace fs {
namespace common {

MoraReader::MoraReader(sql::SqlDatabase *sqlDb1, sql::SqlDatabase *sqlDb2)
{
  assignDatabase(sqlDb1, sqlDb2);
}

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

bool MoraReader::readFromTable(sql::SqlDatabase *sqlDbNav, sql::SqlDatabase *sqlDbSim)
{
  assignDatabase(sqlDbNav, sqlDbSim);
  return readFromTable();
}

bool MoraReader::readFromTable()
{
  clear();

  if(db == nullptr)
  {
    qWarning() << Q_FUNC_INFO << "No MORA database found";
    return false;
  }

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

void MoraReader::fillDbFromQuery(sql::SqlQuery *moraQuery, int fileId)
{
  const static QString MORA_FIELD_NAME("mora%1");

  // create table tbl_grid_mora (
  // starting_latitude  integer (3),
  // starting_longitude integer (4),
  // mora01             text (3),
  // ...
  // mora30             text (3) );

  QList<QStringList> lines;
  moraQuery->exec();

  // The Grid MORA Table will contain records describing the MORA for each Latitude and Longitude block.
  // Each record will contain thirty blocks and the “Starting Latitude” field defines the
  // lower left corner for the first block of each record.
  while(moraQuery->next())
  {
    QStringList line;
    line.append(moraQuery->valueStr("starting_latitude")); // 89 to -90
    line.append(moraQuery->valueStr("starting_longitude")); // -180 to -150

    for(int i = 0; i < 30; i++)
      line.append(moraQuery->valueStr(MORA_FIELD_NAME.arg(i + 1, 2, 10, QChar('0'))));
    lines.append(line);
  }

  fillDbFromFile(lines, fileId);
}

void MoraReader::fillDbFromFile(const QList<QStringList>& lines, int fileId)
{
  if(db == nullptr)
  {
    qWarning() << Q_FUNC_INFO << "No MORA database found";
    return;
  }

  quint16 initialValue = MoraReader::OCEAN;
  QList<quint16> grid(360 * 180, initialValue);

  int carryover = 0;
  int lastpos = -1;

  // The Grid MORA Table will contain records describing the MORA for each Latitude and Longitude block.
  // Each record will contain thirty blocks and the “Starting Latitude” field defines the
  // lower left corner for the first block of each record.
  for(const QStringList& line :lines)
  {
    if(line.size() != 32)
      throw atools::Exception(QStringLiteral("Line too short in MORA grid \"%1\"").arg(line.join(" ")));

    bool ok;
    int startLatY = line.at(0).toInt(&ok); // 89 to -90
    if(!ok)
      throw atools::Exception(QStringLiteral("Invalid latitude value in MORA grid \"%1\"").arg(line.join(" ")));

    int startLonX = line.at(1).toInt(&ok); // -180 to -150
    if(!ok)
      throw atools::Exception(QStringLiteral("Invalid longitude value in MORA grid \"%1\"").arg(line.join(" ")));

    // Change to top left corner
    int pos = (-startLatY + 89) * 360 + startLonX + 180; // 0 - 64800-1

    if(pos == lastpos)
    {
      // Still same data strip
      carryover += 30;
      pos += carryover;
    }
    else
      // New data strip
      carryover = 0;

    for(int i = 0; i < 30; i++)
    {
      QString valueStr = line.at(i + 2);
      quint16 value;

      if(valueStr == "UNK" /* DSF */ || valueStr == "000" /* X-Plane */)
        // Not surveyed
        value = MoraReader::UNKNOWN;
      else
      {
        value = static_cast<quint16>(valueStr.toInt(&ok));
        if(!ok)
          value = MoraReader::ERROR;
      }

      grid[pos + i] = value;
    }
  }

#ifdef DEBUG_MORA
  debugPrint(grid);
#endif

  writeToTable(grid, 360, 180, fileId);
  db->commit();
}

void MoraReader::debugPrint(const QList<quint16>& grid)
{
  QString text;
  for(int laty = 90; laty > -90; laty--)
  {
    QString line;
    line += QStringLiteral("%1 ").arg(laty, 2, 10, QChar('0'));
    for(int lonx = -180; lonx < 180; lonx++)
    {
      int pos = (-laty + 90) * 360 + lonx + 180;
      quint16 val = grid.at(pos);

      if(val == MoraReader::ERROR)
        line += "E";
      else if(val == MoraReader::OCEAN)
        line += "O";
      else if(val == MoraReader::UNKNOWN)
        line += "U";
      else if(val > 10)
        line += "X";
      else
        line += " ";
    }
    text.append(line).append("\n");
  }
  qDebug() << Q_FUNC_INFO << "=================================================================================";
  qDebug().noquote().nospace() << text;
  qDebug() << Q_FUNC_INFO << "=================================================================================";
}

void MoraReader::preDatabaseLoad()
{
  clear();
}

void MoraReader::postDatabaseLoad()
{
  readFromTable();
}

void MoraReader::writeToTable(const QList<quint16>& grid, int columns, int rows, int fileId)
{
  clear();

  if(db == nullptr)
  {
    qWarning() << Q_FUNC_INFO << "No MORA database found";
    return;
  }

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
  for(quint16 value : std::as_const(datagrid))
    out << value;

  // mora_grid_id integer primary key,
  // version integer not null,
  // lonx_columns integer not null,
  // laty_rows integer not null,
  // geometry blob not null
  if(SqlUtil(db).hasTableAndColumn("mora_grid", "file_id"))
    moraWriteQuery.bindValue(":file_id", fileId);
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

void MoraReader::assignDatabase(sql::SqlDatabase *sqlDb1, sql::SqlDatabase *sqlDb2)
{
  db = SqlUtil::getDbWithTableAndRows("mora_grid", {sqlDb1, sqlDb2});
  navdata = sqlDb1 == db;
}

} // namespace common
} // namespace fs
} // namespace atools
