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

#ifndef FLAGS_H
#define FLAGS_H

#include <QDataStream>
#include <initializer_list>

namespace atools {
namespace util {

typedef quint64 FlagType;

/*
 * Flags class wrapping a 64 bit class enum.
 */
template<typename ENUM>
class Flags
{
public:
  typedef ENUM EnumType;

  /* Constructors ======================================= */
  Flags()
    : value(0L)
  {
  }

  Flags(ENUM enumValue)
    : value(FlagType(enumValue))
  {
  }

  Flags(FlagType flagValue)
    : value(flagValue)
  {
  }

  Flags(const Flags& other)
  {
    this->operator=(other);
  }

  Flags(std::initializer_list<ENUM> list)
    : value(0L)
  {
    for(ENUM val : list)
      *this |= val;
  }

  Flags(std::initializer_list<Flags> list)
    : value(0L)
  {
    for(Flags val : list)
      *this |= val;
  }

  /* Assignment ======================================= */
  Flags& operator=(ENUM other)
  {
    value = other;
    return *this;
  }

  /* Comparison ======================================= */
  bool operator==(const ENUM& other) const
  {
    return value == Flags(other).value;
  }

  bool operator!=(const ENUM& other) const
  {
    return !operator==(other);
  }

  /* Bit operators ======================================= */
  Flags& operator&=(int mask)
  {
    value &= mask;
    return *this;
  }

  Flags& operator&=(unsigned int mask)
  {
    value &= mask;
    return *this;
  }

  Flags& operator&=(ENUM mask)
  {
    value &= FlagType(mask);
    return *this;
  }

  Flags& operator|=(Flags other)
  {
    value |= other.value;
    return *this;
  }

  Flags& operator|=(ENUM other)
  {
    value |= FlagType(other);
    return *this;
  }

  Flags& operator^=(Flags other)
  {
    value ^= other.value;
    return *this;
  }

  Flags& operator^=(ENUM other)
  {
    value ^= FlagType(other);
    return *this;
  }

  /* Const bit operators ======================================= */
  Flags operator|(Flags other) const
  {
    return Flags(value | other.value);
  }

  Flags operator|(ENUM other) const
  {
    return Flags(value | FlagType(other));
  }

  Flags operator^(Flags other) const
  {
    return Flags(value ^ other.value);
  }

  Flags operator^(ENUM other) const
  {
    return Flags(value ^ FlagType(other));
  }

  Flags operator&(int mask) const
  {
    return Flags(value & mask);
  }

  Flags operator&(unsigned int mask) const
  {
    return Flags(value & mask);
  }

  Flags operator&(ENUM other) const
  {
    return Flags(value & FlagType(other));
  }

  /* Negate ======================================= */
  Flags operator~() const
  {
    return Flags(~value);
  }

  /* Not ======================================= */
  bool operator!() const
  {
    return !value;
  }

  /* Casts ======================================= */
  operator ENUM() const
  {
    return static_cast<ENUM>(value);
  }

  operator int() const
  {
    return static_cast<int>(value);
  }

  operator unsigned int() const
  {
    return static_cast<unsigned int>(value);
  }

  operator long() const
  {
    return static_cast<long>(value);
  }

  operator unsigned long() const
  {
    return static_cast<unsigned long>(value);
  }

  operator long long() const
  {
    return static_cast<long long>(value);
  }

  operator unsigned long long() const
  {
    return static_cast<unsigned long long>(value);
  }

  /* Set/get methods ======================================= */
  bool testFlag(ENUM flag) const
  {
    return (value & FlagType(flag)) == FlagType(flag) && (FlagType(flag) != 0 || value == FlagType(flag));
  }

  Flags& setFlag(ENUM flag, bool on = true)
  {
    if(on)
      value |= FlagType(flag);
    else
      value &= ~FlagType(flag);
    return *this;
  }

private:
  template<typename E>
  friend QDataStream& operator>>(QDataStream&, Flags<E>&);
  template<typename E>
  friend QDataStream& operator<<(QDataStream&, Flags<E> );

  FlagType value;
};

/* Definition macros ======================================= */
#define ATOOLS_DECLARE_FLAGS(FlagsParam, EnumParam) \
  typedef atools::util::Flags<EnumParam> FlagsParam;

#define ATOOLS_DECLARE_OPERATORS_FOR_FLAGS(FlagsParam) \
  inline atools::util::Flags<FlagsParam::EnumType> \
  operator|(FlagsParam::EnumType f1, FlagsParam::EnumType f2) \
  { \
    return atools::util::Flags<FlagsParam::EnumType>(f1) | f2; \
  } \
  inline atools::util::Flags<FlagsParam::EnumType> \
  operator|(FlagsParam::EnumType f1, atools::util::Flags<FlagsParam::EnumType> f2) \
  { \
    return f2 | f1; \
  }

/* Stream I/O ======================================= */
template<typename E>
QDataStream& operator>>(QDataStream& stream, Flags<E>& flags)
{
  FlagType value;
  stream >> value;
  flags.value = value;
  return stream;
}

template<typename E>
QDataStream& operator<<(QDataStream& stream, atools::util::Flags<E> flags)
{
  stream << static_cast<FlagType>(flags.value);
  return stream;
}

} // namespace util
} // namespace atools

#endif // FLAGS_H
