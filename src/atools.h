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

#ifndef ATOOLS_ATOOLS_H
#define ATOOLS_ATOOLS_H

#include <cmath>

#include <QSet>
#include <QString>

namespace atools {

QString version();

QString gitRevision();

inline Q_DECL_CONSTEXPR int absInt(int value)
{
  return value > 0 ? value : -value;
}

inline Q_DECL_CONSTEXPR long long absLong(long long value)
{
  return value > 0L ? value : -value;
}

/* Round to precision (e.g. roundToPrecision(1111, 2) -> 1100) */
template<typename TYPE>
int roundToPrecision(TYPE value, int precision = 0)
{
  if(precision == 0)
    return static_cast<int>(round(value));
  else
  {
    int factor = static_cast<int>(std::pow(10., precision));
    return static_cast<int>(round(value / factor)) * factor;
  }
}

/* To string with changing precision */
template<typename TYPE>
QString numberToString(TYPE value)
{
  int precision = 0;
  if(value < 10)
    precision = 2;
  else if(value < 100)
    precision = 1;
  return QString::number(value, 'f', precision);
}

/* Capitalize all words in the string with exceptions that are either forced to upper or lower */
QString capString(const QString& str, const QSet<QString>& toUpper = {}, const QSet<QString>& toLower = {},
                  const QSet<QString>& ignore = {});

/* Returns a string containing value number of stars and maxValue - value number of dashes */
QString ratingString(int value, int maxValue);

template<typename TYPE>
Q_DECL_CONSTEXPR bool almostEqual(TYPE f1, TYPE f2)
{
  return std::abs(f1 - f2) < std::numeric_limits<TYPE>::epsilon();
}

template<typename TYPE>
Q_DECL_CONSTEXPR bool almostNotEqual(TYPE f1, TYPE f2)
{
  return std::abs(f1 - f2) >= std::numeric_limits<TYPE>::epsilon();
}

template<typename TYPE>
Q_DECL_CONSTEXPR bool almostEqual(TYPE f1, TYPE f2, TYPE epsilon)
{
  return std::abs(f1 - f2) < epsilon;
}

template<typename TYPE>
Q_DECL_CONSTEXPR bool almostNotEqual(TYPE f1, TYPE f2, TYPE epsilon)
{
  return std::abs(f1 - f2) >= epsilon;
}

template<>
inline Q_DECL_CONSTEXPR bool almostNotEqual<int>(int f1, int f2, int epsilon)
{
  return atools::absInt(f1 - f2) >= epsilon;
}

template<>
inline Q_DECL_CONSTEXPR bool almostNotEqual<long long>(long long f1, long long f2, long long epsilon)
{
  return atools::absLong(f1 - f2) >= epsilon;
}

} // namespace atools

#endif // ATOOLS_ATOOLS_H
