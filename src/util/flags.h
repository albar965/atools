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
  constexpr Flags() noexcept
    : value(0L)
  {
  }

  constexpr Flags(ENUM enumValue) noexcept
    : value(FLAGTYPE(enumValue))
  {
  }

  constexpr Flags(FLAGTYPE flagValue) noexcept
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

  constexpr Flags(const Flags& other) noexcept
  {
    value = other.value;
  }

  constexpr Flags(std::initializer_list<ENUM> list) noexcept
    : value(0L)
  {
    for(ENUM val : list)
      *this |= val;
  }

  constexpr Flags(std::initializer_list<Flags> list) noexcept
    : value(0L)
  {
    for(Flags val : list)
      *this |= val;
  }

  /* Assignment ======================================= */
  constexpr Flags& operator=(ENUM other) noexcept
  {
    value = other;
    return *this;
  }

  constexpr Flags& operator=(Flags other) noexcept
  {
    value = other.value;
    return *this;
  }

  /* Comparison ======================================= */
  constexpr bool operator==(const ENUM& other) const noexcept
  {
    return value == other;
  }

  constexpr bool operator==(const Flags& other) const noexcept
  {
    return value == other.value;
  }

  constexpr bool operator!=(const ENUM& other) const noexcept
  {
    return value != other;
  }

  constexpr bool operator!=(const Flags& other) const noexcept
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

  constexpr Flags& operator&=(Flags other) noexcept
  {
    value &= other.value;
    return *this;
  }

  constexpr Flags& operator&=(ENUM mask) noexcept
  {
    value &= FLAGTYPE(mask);
    return *this;
  }

  constexpr Flags& operator|=(Flags other) noexcept
  {
    value |= other.value;
    return *this;
  }

  constexpr Flags& operator|=(ENUM other) noexcept
  {
    value |= FLAGTYPE(other);
    return *this;
  }

  constexpr Flags& operator^=(Flags other) noexcept
  {
    value ^= other.value;
    return *this;
  }

  constexpr Flags& operator^=(ENUM other) noexcept
  {
    value ^= FLAGTYPE(other);
    return *this;
  }

  /* Const bit operators ======================================= */
  constexpr Flags operator|(Flags other) const noexcept
  {
    return Flags(value | other.value);
  }

  constexpr Flags operator|(ENUM other) const noexcept
  {
    return Flags(value | FLAGTYPE(other));
  }

  constexpr Flags operator^(Flags other) const noexcept
  {
    return Flags(value ^ other.value);
  }

  constexpr Flags operator^(ENUM other) const noexcept
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

  constexpr Flags operator&(Flags other) const noexcept
  {
    return Flags(value & other.value);
  }

  constexpr Flags operator&(ENUM other) const noexcept
  {
    return Flags(value & FLAGTYPE(other));
  }

  /* Negate ======================================= */
  constexpr Flags operator~() const noexcept
  {
    return Flags(~value);
  }

  /* Not ======================================= */
  constexpr bool operator!() const noexcept
  {
    return !value;
  }

  /* Casts ======================================= */
  constexpr ENUM asEnum() const noexcept
  {
    return static_cast<ENUM>(value);
  }

  constexpr FLAGTYPE asFlagType() const noexcept
  {
    return value;
  }

  constexpr operator ENUM() const noexcept
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
  /* Returns true if the flag flag is set, otherwise false. */
  constexpr bool testFlag(ENUM flag) const noexcept
  {
    return testFlags(flag);
  }

  /* Returns true if this flags object matches the given flags */
  constexpr bool testFlags(atools::util::Flags<ENUM, FLAGTYPE> flags) const noexcept
  {
    return flags.value ? ((value & flags.value) == flags.value) : value == FLAGTYPE(0LL);
  }

  /* Returns true if any flag set in flag is also set in this flags object, otherwise false.
   * If flag has no flags set, the return will always be false. */
  constexpr bool testAnyFlag(ENUM flag) const noexcept
  {
    return testAnyFlags(flag);
  }

  /* Returns true if any flag set in flags is also set in this flags object,
   * otherwise false. If flags has no flags set, the return will always be false. */
  constexpr bool testAnyFlags(atools::util::Flags<ENUM, FLAGTYPE> flags) const noexcept
  {
    return (value & flags.value) != FLAGTYPE(0LL);
  }

  constexpr Flags& setFlag(ENUM flag, bool on = true) noexcept
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
 * Declare specific flags type using typedef.
 * Example within namespace: ATOOLS_DECLARE_FLAGS_32(MouseStates, ms::MouseState) */

#define ATOOLS_DECLARE_FLAGS_8(FlagsParam, EnumParam) \
        typedef atools::util::Flags<EnumParam, quint8> FlagsParam;

#define ATOOLS_DECLARE_FLAGS_16(FlagsParam, EnumParam) \
        typedef atools::util::Flags<EnumParam, quint16> FlagsParam;

#define ATOOLS_DECLARE_FLAGS_32(FlagsParam, EnumParam) \
        typedef atools::util::Flags<EnumParam, quint32> FlagsParam;

#define ATOOLS_DECLARE_FLAGS_64(FlagsParam, EnumParam) \
        typedef atools::util::Flags<EnumParam, quint64> FlagsParam;

/* ============================================================================
 * Declare operator|(), qHash() and stream operators operator>> and operator<<.
 * Example within namespace: ATOOLS_DECLARE_OPERATORS_FOR_FLAGS(ms::MouseStates) */

#define ATOOLS_DECLARE_OPERATORS_FOR_FLAGS(FlagsParam) \
        inline atools::util::Flags<FlagsParam::EnumType, FlagsParam::FlagType> \
        constexpr operator|(FlagsParam::EnumType f1, FlagsParam::EnumType f2) \
        { \
          return atools::util::Flags<FlagsParam::EnumType, FlagsParam::FlagType>(f1) | f2; \
        } \
 \
        constexpr inline atools::util::Flags<FlagsParam::EnumType, FlagsParam::FlagType> \
        operator|(FlagsParam::EnumType f1, atools::util::Flags<FlagsParam::EnumType, FlagsParam::FlagType> f2) \
        { \
          return f2 | f1; \
        } \
 \
        constexpr inline size_t qHash(const FlagsParam& flags, size_t seed) \
        { \
          return ::qHash(flags.asFlagType(), seed); \
        } \
 \
        inline QDataStream& operator>>(QDataStream& stream, FlagsParam& flags) \
        { \
          FlagsParam::FlagType value; \
          stream >> value; \
          flags = value; \
          return stream; \
        } \
 \
        inline QDataStream& operator<<(QDataStream& stream, FlagsParam flags) \
        { \
          stream << flags.asFlagType(); \
          return stream; \
        }

/* ============================================================================
 * Helper macros to build flag to string functions
 *
 * Example within namespace:
 * QStringList mapAirspaceTypeToString(const map::MapAirspaceTypes& flags)
 * {
 *   ATOOLS_FLAGS_TO_STR_BEGIN(AIRSPACE_NONE);
 *   ATOOLS_FLAGS_TO_STR(CLASS_A);
 *   ATOOLS_FLAGS_TO_STR(CLASS_B);
 *   ATOOLS_FLAGS_TO_STR_END;
 * }
 */

#define ATOOLS_FLAGS_TO_STR_BEGIN(flag) QStringList list; if(flags == flag) list.append(QStringLiteral(# flag)); else {
#define ATOOLS_FLAGS_TO_STR(flag) if(flags.testFlag(flag)) list.append(QStringLiteral(# flag));
#define ATOOLS_FLAGS_TO_STR_END } return list;

/* ============================================================================
 * Declare debug operator>> and operator<<.
 * Example within namespace: ATOOLS_DECLARE_DEBUG_OPERATORS_FOR_FLAGS(MouseStates, ms::MouseState)
 * and
 * ATOOLS_DEFINE_DEBUG_OPERATORS_FOR_FLAGS(MouseStates, MouseState, mapMouseStateToString)
 */
#define ATOOLS_DECLARE_DEBUG_OPERATORS_FOR_FLAGS(FlagsParam, EnumParam) \
        QDebug operator<<(QDebug out, EnumParam type); \
        QDebug operator<<(QDebug out, const FlagsParam& type);

#define ATOOLS_DEFINE_DEBUG_OPERATORS_FOR_FLAGS(FlagsParam, EnumParam, ConversionFunction) \
        QDebug operator<<(QDebug out, EnumParam type) \
        { \
          out << FlagsParam(type); \
          return out; \
        } \
 \
        QDebug operator<<(QDebug out, const FlagsParam& type) \
        { \
          QDebugStateSaver saver(out); \
          out.nospace().noquote() << ConversionFunction(type).join("|"); \
          return out; \
        }

} // namespace util
} // namespace atools

#endif // ATOOLS_FLAGS_H
