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

#ifndef ATOOLS_BINARYUTIL_H
#define ATOOLS_BINARYUTIL_H

#include <QDataStream>
#include <QList>
#include <QIODevice>

namespace atools {
namespace io {

/* Read list with numeric types from byte array.
 * Array of values of TYPE is prefixed with size SIZETYPE. */
template<typename TYPE, typename SIZETYPE>
QList<TYPE> readList(QByteArray bytes)
{
  QDataStream out(&bytes, QIODevice::ReadOnly);
  out.setVersion(QDataStream::Qt_5_5);
  out.setFloatingPointPrecision(QDataStream::SinglePrecision);

  // Read size prefix
  SIZETYPE num;
  out >> num;

  // Read values
  QList<TYPE> list;
  for(SIZETYPE i = 0; i < num; i++)
  {
    TYPE value;
    out >> value;
    list.append(value);
  }

  return list;
}

/* Write list with numeric types to byte array.
 * Array of values of TYPE is prefixed with size SIZETYPE.
 * Size is limited by max of SIZETYPE.
 * Values are sorted before writing. */
template<typename TYPE, typename SIZETYPE>
QByteArray writeList(QList<TYPE> list)
{
  QByteArray bytes;
  QDataStream out(&bytes, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_5);
  out.setFloatingPointPrecision(QDataStream::SinglePrecision);

  int size = list.size();
  if(size > std::numeric_limits<SIZETYPE>::max())
    size = std::numeric_limits<SIZETYPE>::max();

  // Write size prefix
  out << static_cast<SIZETYPE>(size);

  // Sort values
  std::sort(list.begin(), list.end());

  for(TYPE value : list)
    out << static_cast<TYPE>(value);
  return bytes;
}

} // namespace io
} // namespace atools

#endif // ATOOLS_BINARYUTIL_H
