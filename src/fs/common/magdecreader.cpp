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

#include "fs/common/magdecreader.h"
#include "io/binarystream.h"
#include "geo/pos.h"
#include "sql/sqldatabase.h"
#include "sql/sqlutil.h"
#include "atools.h"
#include "wmm/magdectool.h"
#include "exception.h"
#include "sql/sqlquery.h"

#include <QFile>
#include <QDebug>
#include <cmath>
#include <QDateTime>

using atools::geo::Pos;

namespace atools {
namespace fs {
namespace common {

MagDecReader::MagDecReader()
{
}

MagDecReader::~MagDecReader()
{
  clear();
}

void MagDecReader::readFromWmm()
{
  readFromWmm(QDate::currentDate());
}

void MagDecReader::readFromWmm(const QDate& date)
{
  readFromWmm(date.year(), date.month());
}

void MagDecReader::readFromWmm(int year, int month)
{
  clear();

  // Create WMM model data
  atools::wmm::MagDecTool magDecTool;
  magDecTool.init(year, month);

  referenceDate = magDecTool.getReferenceDate();
  numValues = 360 * 181;

  // Copy to internal representation that allows saving and loading
  magDecValues = new float[numValues];
  for(int latY = -90; latY <= 90; latY++)
  {
    for(int lonX = -180; lonX < 180; lonX++)
      magDecValues[offset(lonX, latY)] = magDecTool.getMagVar(lonX, latY);
  }
}

void MagDecReader::readFromBgl(const QString& filename)
{
  clear();

  QFile file(filename);

  if(file.open(QIODevice::ReadOnly))
  {
    // Offset	Length	Description	Content
    // 0x00	1 - BYTE	World set number	0X01
    // 0x01	127 - ?	Unknown	All 0, except 0x80 at offset 0x6E
    // 0x80	2 - WORD	Number of longitude values	0x168 (360)
    // 0x82	2 - WORD	Number of latitude values	0xB5 (181)
    // 0x84	1 - BYTE	Reference date day (?)	0x01
    // 0x85	1 - BYTE	Reference date month (?)	0x01 (FS2004) - 0x11 (FSX/P3D)
    // 0x86	2 - WORD	Reference date year (?)	0x1993 (FS2004) - 0x2006 (FSX/P3D)

    atools::io::BinaryStream stream(&file);
    int worldSet = stream.readByte();

    // skip unknown bytes
    stream.seekg(0x80);
    quint32 numLongValues = stream.readUShort();
    quint32 numLatValues = stream.readUShort();

    int referenceDay = stream.readByte();
    int referenceMonth = stream.readByte();
    int referenceYear = stream.readShort();

    // Convert the obscure hex notation to decimal
    int year = QString::number(referenceYear, 16).toInt();
    referenceDate.setDate(year, referenceMonth, referenceDay);

    qInfo() << Q_FUNC_INFO << "MagDec World Set" << worldSet
            << "day" << referenceDay << "month" << referenceMonth << "year" << Qt::hex << referenceYear << Qt::dec;

    if(numLongValues != 360)
    {
      qWarning() << "MagDecReader numLongValues is not valid" << numLongValues;
      throw atools::Exception(tr("Number of longitude values is not valid when reading magdec.bgl: %1").
                              arg(numLongValues));
    }

    if(numLatValues != 181)
    {
      qWarning() << "MagDecReader numLatValues is not valid" << numLatValues;
      throw atools::Exception(tr("Number of latitude values is not valid when reading magdec.bgl: %1").
                              arg(numLatValues));
    }

    numValues = numLongValues * numLatValues;

    magDecValues = new float[numValues];

    // Decode all values
    for(quint32 i = 0; i < numValues; i++)
      // East values are positive while West values are negative
      // As an example a E03.4° value will be coded as:
      // MV (E03.4°) = 65536*3.4/360 = 619 (0x26B)
      // A W01.1° value will be coded as:
      // MV (W01.1°) = 65536 - (65536*1.1/360) = 65336 (0xFF38)
      magDecValues[i] = static_cast<float>(stream.readShort()) / 65536.f * 360.f;

    file.close();
  }
  else
    throw atools::Exception(tr("Cannot read %1. Reason: %2").arg(file.fileName()).arg(file.errorString()));
}

void MagDecReader::readFromBytes(const QByteArray& bytes)
{
  clear();

  QDataStream in(bytes);
  in.setVersion(QDataStream::Qt_5_5);
  in.setFloatingPointPrecision(QDataStream::SinglePrecision);

  in >> numValues;
  magDecValues = new float[numValues];

  for(unsigned int i = 0; i < numValues; i++)
    in >> magDecValues[i];
}

void MagDecReader::writeToTable(sql::SqlDatabase& db) const
{
  if(!isValid())
    throw Exception("MagDecReader is invalid");

  db.exec("delete from magdecl");

  atools::sql::SqlQuery query(db);
  query.prepare("insert into magdecl (magdecl_id, reference_time, mag_var) values(:id, :time, :magvar)");

  query.bindValue(":id", 1);
  query.bindValue(":time", QDateTime(getReferenceDate(), QTime()).toSecsSinceEpoch());
  query.bindValue(":magvar", writeToBytes());
  query.exec();
}

bool MagDecReader::readFromTable(sql::SqlDatabase& db)
{
  if(atools::sql::SqlUtil(db).hasTable("magdecl"))
  {
    atools::sql::SqlQuery query(db);
    query.exec("select magdecl_id, reference_time, mag_var from magdecl");

    if(query.next())
    {
      readFromBytes(query.value("mag_var").toByteArray());

      referenceDate = QDateTime::fromSecsSinceEpoch(query.value("reference_time").toUInt()).date();

      qInfo() << Q_FUNC_INFO << db.databaseName() << "Reference date" << referenceDate;

      return true;
    }
    else
      throw atools::Exception(tr("Cannot read declination from database."));
  }
  return false;
}

void MagDecReader::clear()
{
  delete[] magDecValues;
  magDecValues = nullptr;
  referenceDate = QDate();
  wmmVersion.clear();
}

bool MagDecReader::isValid() const
{
  return magDecValues != nullptr;
}

QByteArray MagDecReader::writeToBytes() const
{
  if(!isValid())
    throw Exception("Magnetic declination values are invalid");

  QByteArray bytes;
  QDataStream out(&bytes, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_5);
  out.setFloatingPointPrecision(QDataStream::SinglePrecision);

  out << numValues;
  for(quint32 i = 0; i < numValues; i++)
    out << magDecValues[i];

  return bytes;
}

float MagDecReader::getMagVar(const geo::Pos& pos) const
{
  if(!isValid())
    throw Exception("MagDecReader is invalid");

  Pos posNorm(pos.normalized());
  float lonX = posNorm.getLonX();
  float latY = posNorm.getLatY();

  int minLonX1 = static_cast<int>(std::floor(lonX)), maxLonX2 = static_cast<int>(std::ceil(lonX)),
      minLatY1 = static_cast<int>(std::floor(latY)), maxLatY2 = static_cast<int>(std::ceil(latY));

  if(pos.nearGrid(atools::geo::Pos::POS_EPSILON_500M))
    // Exact or near degree - nothing to interpolate
    return magvar(offset(atools::roundToInt(pos.getLonX()), atools::roundToInt(pos.getLatY())));
  else
  {
    // Get four exact degree points around the coordinate
    int topRightOffsetQ12 = offset(minLonX1, maxLatY2), topLeftOffsetQ22 = offset(maxLonX2, maxLatY2),
        bottomRightOffsetQ11 = offset(maxLonX2, minLatY1), bottomLeftOffsetQ21 = offset(minLonX1, minLatY1);

    // Calculate magvar values for the four points
    float fQ12 = magvar(topRightOffsetQ12), fQ22 = magvar(topLeftOffsetQ22),
          fQ11 = magvar(bottomRightOffsetQ11), fQ21 = magvar(bottomLeftOffsetQ21);

    // Do a bilinear interpolation between the four points
    float fR1, fR2;
    float diffX = maxLonX2 - minLonX1;
    if(std::abs(diffX) > 0.f)
    {
      fR1 = (maxLonX2 - lonX) / diffX * fQ11 + (lonX - minLonX1) / diffX * fQ21;
      fR2 = (maxLonX2 - lonX) / diffX * fQ12 + (lonX - minLonX1) / diffX * fQ22;
    }
    else
    {
      fR1 = (fQ11 + fQ21) / 2.f;
      fR2 = (fQ12 + fQ22) / 2.f;
    }

    float diffY = maxLatY2 - minLatY1;
    if(std::abs(diffY) > 0.f)
      return (maxLatY2 - latY) / diffY * fR1 + (latY - minLatY1) / diffY * fR2;
    else
      return (fR1 + fR2) / 2.f;
  }
}

float MagDecReader::magvar(int offset) const
{
  if(offset >= 0 && offset < static_cast<int>(numValues))
    return magDecValues[offset];
  else
    throw Exception(QString("Wrong offset into magnetic declination %1").arg(offset));
}

// Latitude/Longitude table is 130,320 bytes length and starts at offset 0x88.
// Magnetic variations for all entire degree latitude/longitude values are stored as a WORD list starting at E000-S90.
// For each longitude value from E000 to W179, the list tabulates magnetic variations by increasing latitude from S90 to N90.
// This organization can be sumarized as follows:
// E000 (S90->S89->S88...->S01->N00->N01->...->N90), then
// E001 (S90->S89->S88...->S01->N00->N01->...->N90), followed by E002, E003 lines
// ...
// E180 (S90->S89->S88...->S01->N00->N01->...->N90), then
// W179 (S90->S89->S88...->S01->N00->N01->...->N90), followed by W178, W177 lines
// ...
// W001 (S90->S89->S88...->S01->N00->N01->...->N90) that is the last line
// Therefore, for each of the 360 longitude values, there is 181 latitude values for a total of 65,160 consecutive WORD values.
// First value of this table is at offset 0x88 (E000/S90) and last is at offset 0x1FD96 (W001/N90).
// File offset for a given Lat/Long can be obtained using the following formula
// For positive (East) longitudes (from 0 to 180):
// Offset = (Long*362)+(Lat*2)+316
// For negative (West) longitudes (from -1 to -179):
// Offset =((Long+360)*362)+(Lat*2)+316
// Note that North latitudes should be entered as positive values (0 to 90) and South latitudes as negative values (-1 to -90)
int MagDecReader::offset(int lonX, int latY) const
{
  if(lonX == -180)
    // Wrap around - other values should not appear on normalized coordinates
    lonX = 180;

  if(lonX >= 0 && lonX <= 180)
    // For positive (East) longitudes (from 0 to 180):
    // East: Offset = (Long*362)+(Lat*2)+180
    return ((lonX * 362) + (latY * 2) + 180) / 2;
  else if(lonX <= -1 && lonX >= -179)
    // For negative (West) longitudes (from -1 to -179):
    // West: Offset =((Long+360)*362)+(Lat*2)+180
    return (((lonX + 360) * 362) + (latY * 2) + 180) / 2;
  else
    qWarning() << "MagDecReader invalid coordinates in offset calculation" << lonX << latY;
  return 0;
}

} // namespace common
} // namespace fs
} // namespace atools
