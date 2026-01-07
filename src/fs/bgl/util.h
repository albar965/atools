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

#ifndef ATOOLS_BGL_UTIL_H
#define ATOOLS_BGL_UTIL_H

#include <QString>
#include <QStringLiteral>

namespace atools {
namespace fs {
namespace bgl {
namespace util {

/*
 * Convert an enum to a string using some heuristics to find invalid or empty values
 * @param func Callback function to convert the enum to a string
 * @param value The enum value
 */

template<typename TYPE>
QString enumToStr(QString func(TYPE t), TYPE value)
{
  QString retval = func(value);
  if(retval.isEmpty())
    return QString();

  if(retval == QStringLiteral("NONE") || retval == QStringLiteral("NO") || retval == QStringLiteral("UNKNOWN") ||
     retval == QStringLiteral("INVALID") || retval.startsWith(QStringLiteral("UNKNOWN_")) || retval.endsWith(QStringLiteral("_UNKNOWN")))
    return QString();
  else
    return retval;
}

template<typename TYPE, typename TYPE2>
bool isFlagSet(TYPE bitfield, TYPE2 flag)
{
  return (bitfield & flag) == static_cast<TYPE>(flag);
}

template<typename TYPE, typename TYPE2>
bool isFlagNotSet(TYPE bitfield, TYPE2 flag)
{
  return (bitfield & flag) == 0;
}

} // namespace util
} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_UTIL_H
