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

#include "fs/common/binarygeometry.h"

#include <QDataStream>

namespace atools {
namespace fs {
namespace common {

BinaryGeometry::BinaryGeometry(const geo::LineString& value)
  : geometry(value)
{

}

BinaryGeometry::BinaryGeometry(const QByteArray& bytes)
{
  readFromByteArray(bytes);
}

BinaryGeometry::BinaryGeometry()
{

}

void BinaryGeometry::readFromByteArray(const QByteArray& bytes)
{
  geometry.clear();

  QDataStream in(bytes);
  in.setVersion(QDataStream::Qt_5_5);
  in.setFloatingPointPrecision(QDataStream::SinglePrecision);

  quint32 size;
  float lonx, laty;
  in >> size;
  for(unsigned int i = 0; i < size; i++)
  {
    in >> lonx >> laty;
    geometry.append(lonx, laty);
  }
}

QByteArray BinaryGeometry::writeToByteArray()
{
  QByteArray bytes;
  QDataStream out(&bytes, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_5);
  out.setFloatingPointPrecision(QDataStream::SinglePrecision);

  out << static_cast<quint32>(geometry.size());
  for(const atools::geo::Pos& pos : geometry)
    out << pos.getLonX() << pos.getLatY();
  return bytes;
}

} // namespace common
} // namespace fs
} // namespace atools
