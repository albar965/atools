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

#ifndef ATOOLS_FLAGS_H
#define ATOOLS_FLAGS_H

#include <QDataStream>
#include <QHash>
#include <initializer_list>

namespace atools {
namespace util {

/*
 * Flags class wrapping an integer class enum. Can be quint8, quint16, quint32 or quint64.
 */
template<typename ENUM, typename FLAGTYPE>
class Flags
{
  /* Print error message at compile time if size of enum and template parameter FLAGTYPE differs */
  static_assert(sizeof(ENUM) == sizeof(FLAGTYPE), "atools::util::Flags: sizeof(ENUM) != sizeof(FLAGTYPE)");
  static_assert(std::is_convertible<FLAGTYPE, quint64>(), "std::is_convertible<FLAGTYPE, quint64>()");

public:
  /* Create types for template parameters. */
  typedef ENUM EnumType;
  typedef FLAGTYPE FlagType;

  /* Constructors ======================================= */
  Flags()
    : value(0L)
  {
  }

  Flags(ENUM enumValue)
    : value(FLAGTYPE(enumValue))
  {
  }

  Flags(FLAGTYPE flagValue)
    : value(flagValue)
  {
  }

#ifdef ATOOLS_FLAGS_NO_INT
  Flags(int flagValue)
    : value(flagValue)
  {
  }

  Flags(long flagValue)
    : value(flagValue)
  {
  }

  Flags(long long flagValue)
    : value(flagValue)
  {
  }

#endif

  Flags(const Flags& other)
  {
    value = other.value;
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

  Flags& operator=(Flags other)
  {
    value = other.value;
    return *this;
  }

  /* Comparison ======================================= */
  bool operator==(const ENUM& other) const
  {
    return value == other;
  }

  bool operator==(const Flags& other) const
  {
    return value == other.value;
  }

  bool operator!=(const ENUM& other) const
  {
    return value != other;
  }

  bool operator!=(const Flags& other) const
  {
    return value != other.value;
  }

  /* Bit operators ======================================= */
#ifdef ATOOLS_FLAGS_NO_INT
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

#endif

  Flags& operator&=(Flags other)
  {
    value &= other.value;
    return *this;
  }

  Flags& operator&=(ENUM mask)
  {
    value &= FLAGTYPE(mask);
    return *this;
  }

  Flags& operator|=(Flags other)
  {
    value |= other.value;
    return *this;
  }

  Flags& operator|=(ENUM other)
  {
    value |= FLAGTYPE(other);
    return *this;
  }

  Flags& operator^=(Flags other)
  {
    value ^= other.value;
    return *this;
  }

  Flags& operator^=(ENUM other)
  {
    value ^= FLAGTYPE(other);
    return *this;
  }

  /* Const bit operators ======================================= */
  Flags operator|(Flags other) const
  {
    return Flags(value | other.value);
  }

  Flags operator|(ENUM other) const
  {
    return Flags(value | FLAGTYPE(other));
  }

  Flags operator^(Flags other) const
  {
    return Flags(value ^ other.value);
  }

  Flags operator^(ENUM other) const
  {
    return Flags(value ^ FLAGTYPE(other));
  }

#ifdef ATOOLS_FLAGS_INT_OPERATOR

  Flags operator&(int mask) const
  {
    return Flags(value & mask);
  }

  Flags operator&(unsigned int mask) const
  {
    return Flags(value & mask);
  }

#endif

  Flags operator&(Flags other) const
  {
    return Flags(value & other.value);
  }

  Flags operator&(ENUM other) const
  {
    return Flags(value & FLAGTYPE(other));
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
  ENUM asEnum() const
  {
    return static_cast<ENUM>(value);
  }

  FLAGTYPE asFlagType() const
  {
    return value;
  }

  operator ENUM() const
  {
    return static_cast<ENUM>(value);
  }

#ifdef ATOOLS_FLAGS_INT_CAST
  operator int() const
  {
    return static_cast<int>(value);
  }

  operator unsigned int() const
  {
    return static_cast<unsigned int>(value);
  }
#endif

#ifdef ATOOLS_FLAGS_CAST

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
#endif

  /* Set/get methods ======================================= */
  bool testFlag(ENUM flag) const
  {
    return (value & FLAGTYPE(flag)) == FLAGTYPE(flag) && (FLAGTYPE(flag) != 0 || value == FLAGTYPE(flag));
  }

  Flags& setFlag(ENUM flag, bool on = true)
  {
    if(on)
      value |= FLAGTYPE(flag);
    else
      value &= ~FLAGTYPE(flag);
    return *this;
  }

private:
  FLAGTYPE value;
};

/* Definition macros ======================================= */

/* ============================================================================
 * Declare specific flags type using typedef */

#define ATOOLS_DECLARE_FLAGS_8(FlagsParam, EnumParam) \
        typedef atools::util::Flags<EnumParam, quint8> FlagsParam;

#define ATOOLS_DECLARE_FLAGS_16(FlagsParam, EnumParam) \
        typedef atools::util::Flags<EnumParam, quint16> FlagsParam;

#define ATOOLS_DECLARE_FLAGS_32(FlagsParam, EnumParam) \
        typedef atools::util::Flags<EnumParam, quint32> FlagsParam;

#define ATOOLS_DECLARE_FLAGS_64(FlagsParam, EnumParam) \
        typedef atools::util::Flags<EnumParam, quint64> FlagsParam;

/* ============================================================================
 * Declare operator|(), qHash() and stream operators operator>> and operator<< */

#define ATOOLS_DECLARE_OPERATORS_FOR_FLAGS(FlagsParam) \
        inline atools::util::Flags<FlagsParam::EnumType, FlagsParam::FlagType> \
        operator|(FlagsParam::EnumType f1, FlagsParam::EnumType f2) \
        { \
          return atools::util::Flags<FlagsParam::EnumType, FlagsParam::FlagType>(f1) | f2; \
        } \
        inline atools::util::Flags<FlagsParam::EnumType, FlagsParam::FlagType> \
        operator|(FlagsParam::EnumType f1, atools::util::Flags<FlagsParam::EnumType, FlagsParam::FlagType> f2) \
        { \
          return f2 | f1; \
        } \
        inline size_t qHash(const FlagsParam& flags, size_t seed) \
        { \
          return ::qHash(flags.asFlagType(), seed); \
        } \
        inline QDataStream& operator>>(QDataStream& stream, FlagsParam& flags) \
        { \
          FlagsParam::FlagType value; \
          stream >> value; \
          flags = value; \
          return stream; \
        } \
        inline QDataStream& operator<<(QDataStream& stream, FlagsParam flags) \
        { \
          stream << flags.asFlagType(); \
          return stream; \
        }

} // namespace util
} // namespace atools

#endif // ATOOLS_FLAGS_H
