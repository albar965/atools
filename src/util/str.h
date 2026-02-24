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

#ifndef ATOOLS_STR_H
#define ATOOLS_STR_H

#include <QString>
#include <tuple>
#include <cstring>

namespace atools {
namespace util {

/*
 * Fast string implementations for hash maps and sets. Fixed string length defined using a template parameter.
 *
 * Limited to Latin1 characters. Objects are comparable, sortable and hashable.
 *
 * Use Q_DECLARE_TYPEINFO to declare instances as Q_PRIMITIVE_TYPE
 */
template<int SIZE>
class Str
{
public:
  explicit Str(const char *strParam)
  {
    std::strncpy(str, strParam, SIZE - 1); // Limit to SIZE
    str[SIZE - 1] = '\0'; // Ensure null termination
  }

  explicit Str(const QString& strParam)
  {
    std::strncpy(str, strParam.toLatin1().constData(), SIZE - 1); // Limit to SIZE
    str[SIZE - 1] = '\0'; // Ensure null termination
  }

  Str()
  {
    str[0] = '\0';
  }

  Str(const Str<SIZE>& other)
  {
    this->operator=(other);
  }

  Str& operator=(const Str<SIZE>& other)
  {
    std::strcpy(str, other.str);
    return *this;
  }

  friend bool operator==(const Str<SIZE>& str1, const Str<SIZE>& str2)
  {
    return std::strcmp(str1.str, str2.str) == 0;
  }

  friend bool operator!=(const Str<SIZE>& str1, const Str<SIZE>& str2)
  {
    return !operator==(str1, str2);
  }

  friend bool operator<(const Str<SIZE>& str1, const Str<SIZE>& str2)
  {
    return std::strcmp(str1.str, str2.str) < 0;
  }

  QLatin1String getString() const
  {
    return QLatin1String(str);
  }

  const char *getStr() const
  {
    return str;
  }

  const static int size = SIZE;

private:
  char str[SIZE];
};

/*
 * Same as above but for a pair of strings
 */
template<int SIZE>
class StrPair
{
public:
  explicit StrPair(const QString& firstParam, const QString& secondParam)
    : first(firstParam), second(secondParam)
  {
  }

  explicit StrPair(const Str<SIZE>& firstParam, const Str<SIZE>& secondParam)
    : first(firstParam), second(secondParam)
  {
  }

  explicit StrPair(const char *firstParam, const char *secondParam)
    : first(firstParam), second(secondParam)
  {
  }

  StrPair()
  {
  }

  StrPair(const StrPair<SIZE>& other)
  {
    this->operator=(other);
  }

  StrPair& operator=(const StrPair<SIZE>& other)
  {
    first = other.first;
    second = other.second;
    return *this;
  }

  friend bool operator==(const StrPair<SIZE>& str1, const StrPair<SIZE>& str2)
  {
    return std::tie(str1.first, str1.second) == std::tie(str2.first, str2.second);
  }

  friend bool operator!=(const StrPair<SIZE>& str1, const StrPair<SIZE>& str2)
  {
    return !operator==(str1, str2);
  }

  friend bool operator<(const StrPair<SIZE>& str1, const StrPair<SIZE>& str2)
  {
    return std::tie(str1.first, str1.second) < std::tie(str2.first, str2.second);
  }

  const Str<SIZE>& getFirst() const
  {
    return first;
  }

  const Str<SIZE>& getSecond() const
  {
    return second;
  }

  const static int size = SIZE;

private:
  Str<SIZE> first, second;
};

/*
 * Same as above but for three strings
 */
template<int SIZE>
class StrTriple
{
public:
  explicit StrTriple(const QString& firstParam, const QString& secondParam, const QString& thirdParam)
    : first(firstParam), second(secondParam), third(thirdParam)
  {
  }

  explicit StrTriple(const Str<SIZE>& firstParam, const Str<SIZE>& secondParam, const Str<SIZE>& thirdParam)
    : first(firstParam), second(secondParam), third(thirdParam)
  {
  }

  explicit StrTriple(const char *firstParam, const char *secondParam, const char *thirdParam)
    : first(firstParam), second(secondParam), third(thirdParam)
  {
  }

  StrTriple()
  {
  }

  StrTriple(const StrTriple<SIZE>& other)
  {
    this->operator=(other);
  }

  StrTriple& operator=(const StrTriple<SIZE>& other)
  {
    first = other.first;
    second = other.second;
    third = other.third;
    return *this;
  }

  friend bool operator==(const StrTriple<SIZE>& str1, const StrTriple<SIZE>& str2)
  {
    return std::tie(str1.first, str1.second, str1.third) == std::tie(str2.first, str2.second, str2.third);
  }

  friend bool operator!=(const StrTriple<SIZE>& str1, const StrTriple<SIZE>& str2)
  {
    return !operator==(str1, str2);
  }

  friend bool operator<(const StrTriple<SIZE>& str1, const StrTriple<SIZE>& str2)
  {
    return std::tie(str1.first, str1.second, str1.third) < std::tie(str2.first, str2.second, str2.third);
  }

  const Str<SIZE>& getFirst() const
  {
    return first;
  }

  const Str<SIZE>& getSecond() const
  {
    return second;
  }

  const Str<SIZE>& getThird() const
  {
    return third;
  }

  const static int size = SIZE;

private:
  Str<SIZE> first, second, third;
};

template<int SIZE>
inline size_t qHash(const atools::util::Str<SIZE>& str, size_t seed)
{
  return qHash(str.getString(), seed);
}

template<int SIZE>
inline size_t qHash(const atools::util::StrPair<SIZE>& str, size_t seed)
{
  return qHashMulti(seed, str.getFirst().getString(), str.getSecond().getString());
}

template<int SIZE>
inline size_t qHash(const atools::util::StrTriple<SIZE>& str, size_t seed)
{
  return qHashMulti(seed, str.getFirst().getString(), str.getSecond().getString(), str.getThird().getString());
}

/* Functions without seed are needed to avoid compilation issues with qHashMulti */
template<int SIZE>
inline size_t qHash(const atools::util::Str<SIZE>& str)
{
  return qHash(str.getString(), 147397);
}

template<int SIZE>
inline size_t qHash(const atools::util::StrPair<SIZE>& str)
{
  return qHashMulti(147397, str.getFirst().getString(), str.getSecond().getString());
}

template<int SIZE>
inline size_t qHash(const atools::util::StrTriple<SIZE>& str)
{
  return qHashMulti(147397, str.getFirst().getString(), str.getSecond().getString(), str.getThird().getString());
}

} // namespace util
} // namespace atools

#endif // ATOOLS_STR_H
