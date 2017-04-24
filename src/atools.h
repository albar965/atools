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

#ifndef ATOOLS_ATOOLS_H
#define ATOOLS_ATOOLS_H

#include <cmath>

#include <QSet>
#include <QString>
#include <QVariant>
#include <QDebug>

namespace atools {

QString version();

QString gitRevision();

/* replace variables in a string like "${LANG}" */
QString replaceVar(QString str, const QString& name, const QVariant& value);
QString replaceVar(QString str, const QHash<QString, QVariant>& variableValues);

template<typename TYPE>
bool contains(const TYPE& name, const std::initializer_list<TYPE>& list)
{
  for(const TYPE& val : list)
    if(val == name)
      return true;

  return false;
}

bool contains(const QString& name, const std::initializer_list<QString>& list);

bool contains(const QString& name, const std::initializer_list<const char *>& list);

/* different to std::fmod and std::remainder. Sign follows the divisor or be Euclidean. Remainder of x/y */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE mod(TYPE x, TYPE y)
{
  return x - y *std::floor(x / y);
}

template<typename TYPE>
Q_DECL_CONSTEXPR bool inRange(const QList<TYPE>& list, int index)
{
  return index >= 0 && index < list.size();
}

template<typename TYPE>
Q_DECL_CONSTEXPR bool inRange(const QVector<TYPE>& list, int index)
{
  return index >= 0 && index < list.size();
}

template<typename TYPE>
const TYPE& at(const QList<TYPE>& list, int index, const TYPE& defaultType = TYPE())
{
  if(inRange(list, index))
    return list.at(index);
  else
    qWarning() << "index out of bounds:" << index << "list size" << list.size();
  return defaultType;
}

template<typename TYPE>
const TYPE& at(const QVector<TYPE>& list, int index, const TYPE& defaultType = TYPE())
{
  if(inRange(list, index))
    return list.at(index);
  else
    qWarning() << "index out of bounds:" << index << "list size" << list.size();
  return defaultType;
}

template<typename TYPE>
const TYPE& at(const QList<TYPE>& list, int index, const QString& msg, const TYPE& defaultType = TYPE())
{
  if(inRange(list, index))
    return list.at(index);
  else
    qWarning() << "index out of bounds:" << index << "list size" << list.size() << "message" << msg;
  return defaultType;
}

template<typename TYPE>
const TYPE& at(const QVector<TYPE>& list, int index, const QString& msg, const TYPE& defaultType = TYPE())
{
  if(inRange(list, index))
    return list.at(index);
  else
    qWarning() << "index out of bounds:" << index << "list size" << list.size() << "message" << msg;
  return defaultType;
}

/* Remove all special characters from the filename that can disturb any filesystem */
QString cleanFilename(const QString& filename);

Q_DECL_CONSTEXPR int absInt(int value)
{
  return value > 0 ? value : -value;
}

Q_DECL_CONSTEXPR long absLong(long value)
{
  return value > 0L ? value : -value;
}

Q_DECL_CONSTEXPR long long absLongLong(long long value)
{
  return value > 0L ? value : -value;
}

/* Round to integer value */
template<typename TYPE>
Q_DECL_CONSTEXPR int roundToInt(TYPE value)
{
  return static_cast<int>(round(value));
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
int sign(TYPE t)
{
  if(static_cast<double>(t) > 0.)
    return 1;
  else if(static_cast<double>(t) < 0.)
    return -1;
  else
    return 0;
}

template<typename TYPE>
Q_DECL_CONSTEXPR bool almostEqual(TYPE f1, TYPE f2)
{
  return std::abs(f1 - f2) <= std::numeric_limits<TYPE>::epsilon();
}

template<typename TYPE>
Q_DECL_CONSTEXPR bool almostNotEqual(TYPE f1, TYPE f2)
{
  return std::abs(f1 - f2) >= std::numeric_limits<TYPE>::epsilon();
}

template<typename TYPE>
Q_DECL_CONSTEXPR bool almostEqual(TYPE f1, TYPE f2, TYPE epsilon)
{
  return std::abs(f1 - f2) <= epsilon;
}

template<>
Q_DECL_CONSTEXPR bool almostEqual<int>(int f1, int f2, int epsilon)
{
  return atools::absInt(f1 - f2) <= epsilon;
}

template<>
Q_DECL_CONSTEXPR bool almostEqual<long>(long f1, long f2, long epsilon)
{
  return atools::absLong(f1 - f2) <= epsilon;
}

template<>
Q_DECL_CONSTEXPR bool almostEqual<long long>(long long f1, long long f2, long long epsilon)
{
  return atools::absLongLong(f1 - f2) <= epsilon;
}

template<typename TYPE>
Q_DECL_CONSTEXPR bool almostNotEqual(TYPE f1, TYPE f2, TYPE epsilon)
{
  return std::abs(f1 - f2) >= epsilon;
}

template<>
Q_DECL_CONSTEXPR bool almostNotEqual<int>(int f1, int f2, int epsilon)
{
  return atools::absInt(f1 - f2) >= epsilon;
}

template<>
Q_DECL_CONSTEXPR bool almostNotEqual<long>(long f1, long f2, long epsilon)
{
  return atools::absLong(f1 - f2) >= epsilon;
}

template<>
Q_DECL_CONSTEXPR bool almostNotEqual<long long>(long long f1, long long f2, long long epsilon)
{
  return atools::absLongLong(f1 - f2) >= epsilon;
}

} // namespace atools

#endif // ATOOLS_ATOOLS_H
