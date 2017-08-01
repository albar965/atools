/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

#include <QFile>
#include <QDebug>
#include <cmath>

#include "sql/sqlquery.h"

using atools::geo::Pos;

namespace atools {
namespace fs {
namespace common {

MagDecReader::MagDecReader()
{
}

MagDecReader::~MagDecReader()
{
  delete[] magDeclValues;
}

void MagDecReader::readFromBgl(const QString& filename)
{
  delete[] magDeclValues;
  magDeclValues = nullptr;

  QFile file(filename);

  if(file.exists())
  {
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

      qInfo() << "MagDec World Set" << worldSet
              << "day" << referenceDay << "month" << referenceMonth << "year" << hex << referenceYear << dec;

      if(numLongValues != 360)
      {
        qWarning() << "MagDecReader numLongValues is not valid" << numLongValues;
        return;
      }

      if(numLatValues != 181)
      {
        qWarning() << "MagDecReader numLatValues is not valid" << numLatValues;
        return;
      }

      numValues = numLongValues * numLatValues;

      magDeclValues = new float[numValues];

      // Decode all values
      for(quint32 i = 0; i < numValues; i++)
        // East values are positive while West values are negative
        // As an example a E03.4° value will be coded as:
        // MV (E03.4°) = 65536*3.4/360 = 619 (0x26B)
        // A W01.1° value will be coded as:
        // MV (W01.1°) = 65536 - (65536*1.1/360) = 65336 (0xFF38)
        magDeclValues[i] = static_cast<float>(stream.readShort()) / 65536.f * 360.f;

      file.close();
    }
  }
}

void MagDecReader::readFromBytes(const QByteArray& bytes)
{
  delete[] magDeclValues;
  magDeclValues = nullptr;

  QDataStream in(bytes);
  in.setVersion(QDataStream::Qt_5_5);
  in.setFloatingPointPrecision(QDataStream::SinglePrecision);

  in >> numValues;
  magDeclValues = new float[numValues];

  for(unsigned int i = 0; i < numValues; i++)
    in >> magDeclValues[i];
}

void MagDecReader::writeToTable(sql::SqlDatabase& db) const
{
  if(!isValid())
    throw new Exception("MagDecReader is invalid");

  db.exec("delete from magdecl");

  atools::sql::SqlQuery query(db);
  query.prepare("insert into magdecl (magdecl_id, reference_time, mag_var) values(:id, :time, :magvar)");

  query.bindValue(":id", 1);
  query.bindValue(":time", QDateTime(getReferenceDate()).toTime_t());
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
      QByteArray bytes = query.value("mag_var").toByteArray();
      readFromBytes(bytes);

      QDateTime timestamp;
      timestamp.setTime_t(query.value("reference_time").toUInt());
      referenceDate = timestamp.date();
      return true;
    }
    else
      qWarning() << "Nothing found in table magdecl";
  }
  return false;
}

void MagDecReader::clear()
{
  delete[] magDeclValues;
  magDeclValues = nullptr;
}

QByteArray MagDecReader::writeToBytes() const
{
  if(!isValid())
    throw new Exception("MagDecReader is invalid");

  QByteArray bytes;
  QDataStream out(&bytes, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_5);
  out.setFloatingPointPrecision(QDataStream::SinglePrecision);

  out << numValues;
  for(quint32 i = 0; i < numValues; i++)
    out << magDeclValues[i];

  return bytes;
}

float MagDecReader::getMagVar(const geo::Pos& pos) const
{
  if(!isValid())
    throw new Exception("MagDecReader is invalid");

  Pos posNorm(pos.normalized());
  float lonX = posNorm.getLonX();
  float latY = posNorm.getLatY();

  int minLonX1 = static_cast<int>(std::floor(lonX)), maxLonX2 = static_cast<int>(std::ceil(lonX)),
      minLatY1 = static_cast<int>(std::floor(latY)), maxLatY2 = static_cast<int>(std::ceil(latY));

  if(minLonX1 == maxLonX2 && minLatY1 == maxLatY2)
    // Exact degree - nothing to interpolate
    return magvar(offset(minLonX1, minLatY1));
  else
  {
    // Get four exact degree points around the coordinate
    int topRightOffsetQ12 = offset(minLonX1, maxLatY2), topLeftOffsetQ22 = offset(maxLonX2, maxLatY2),
        bottomRightOffsetQ11 = offset(maxLonX2, minLatY1), bottomLeftOffsetQ21 = offset(minLonX1, minLatY1);

    // Calculate magvar values for the four points
    float fQ12 = magvar(topRightOffsetQ12), fQ22 = magvar(topLeftOffsetQ22),
          fQ11 = magvar(bottomRightOffsetQ11), fQ21 = magvar(bottomLeftOffsetQ21);

    // Do a bilinear interpolation between the four points
    float fR1 = (maxLonX2 - lonX) / (maxLonX2 - minLonX1) * fQ11 + (lonX - minLonX1) / (maxLonX2 - minLonX1) * fQ21;
    float fR2 = (maxLonX2 - lonX) / (maxLonX2 - minLonX1) * fQ12 + (lonX - minLonX1) / (maxLonX2 - minLonX1) * fQ22;
    float fP = (maxLatY2 - latY) / (maxLatY2 - minLatY1) * fR1 + (latY - minLatY1) / (maxLatY2 - minLatY1) * fR2;
    return fP;
  }
}

float MagDecReader::magvar(int offset) const
{
  if(offset >= 0 && offset < static_cast<int>(numValues))
    return magDeclValues[offset];
  else
    throw new Exception(QString("Wrong offset into magvar array %1").arg(offset));
}

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
