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

#ifndef ATOOLS_BINARYUTIL_H
#define ATOOLS_BINARYUTIL_H

#include <QDataStream>
#include <QVector>

namespace atools {
namespace io {

/* Read vector with numeric types from byte array.
 * Array of values of TYPE is prefixed with size SIZETYPE. */
template<typename TYPE, typename SIZETYPE>
QVector<TYPE> readVector(QByteArray bytes)
{
  QDataStream out(&bytes, QIODevice::ReadOnly);
  out.setVersion(QDataStream::Qt_5_5);
  out.setFloatingPointPrecision(QDataStream::SinglePrecision);

  // Read size prefix
  SIZETYPE num;
  out >> num;

  // Read values
  QVector<TYPE> vector;
  for(SIZETYPE i = 0; i < num; i++)
  {
    TYPE value;
    out >> value;
    vector.append(value);
  }

  return vector;
}

/* Write vector with numeric types to byte array.
 * Array of values of TYPE is prefixed with size SIZETYPE.
 * Size is limited by max of SIZETYPE.
 * Values are sorted before writing. */
template<typename TYPE, typename SIZETYPE>
QByteArray writeVector(QVector<TYPE> vector)
{
  QByteArray bytes;
  QDataStream out(&bytes, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_5);
  out.setFloatingPointPrecision(QDataStream::SinglePrecision);

  int size = vector.size();
  if(size > std::numeric_limits<SIZETYPE>::max())
    size = std::numeric_limits<SIZETYPE>::max();

  // Write size prefix
  out << static_cast<SIZETYPE>(size);

  // Sort values
  std::sort(vector.begin(), vector.end());

  for(TYPE value : vector)
    out << static_cast<TYPE>(value);
  return bytes;
}

} // namespace io
} // namespace atools

#endif // ATOOLS_BINARYUTIL_H
