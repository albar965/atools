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

#ifndef ATOOLS_PROPS_H
#define ATOOLS_PROPS_H

#include <QMultiHash>
#include <QList>
#include <QDataStream>

class QVariant;

namespace atools {
namespace util {

namespace ptinternal {
/* Types to store in Prop. Internal use only. Not exposed to user. */
enum PropType : quint8
{
  INVALID, /* Default constructed */
  NONE, /* No value - just key */
  BOOL, /* getValueBool() - saved as qint8 */

  INT8, /* getValueByte() - saved as qint8 */
  UINT8, /* getValueByte() - saved as quint8 */
  INT16, /* getValueShort() - saved as qint16 */
  UINT16, /* getValueShort() - saved as quint16 */
  INT32, /* getValueInt()  - saved as qint32 */
  UINT32, /* getValueInt()  - saved as quint32 */
  INT64, /* getValueLongLong()  - saved as qint64 */

  FLOAT, /* getValueFloat() - saved as IEEE float, 4 bytes */
  DOUBLE, /* getValueDouble() - saved as IEEE double, 8 bytes */

  STRING8, /* getValueString() - UTF-8 string with quint8 size prefixed */
  STRING16, /* getValueString() - UTF-8 string with quint16 size prefixed */
  STRING32, /* getValueString() - UTF-8 string with quint32 size prefixed */

  BYTES8, /* getValueBytes() - Bytes with quint8 size prefixed */
  BYTES16, /* getValueBytes() - Bytes with quint16 size prefixed */
  BYTES32 /* getValueBytes() - Bytes with quint32 size prefixed */
};

}

/*
 * Efficient property value storage. Uses a simplified type system avoiding bloated QMetatype and QVariant classes
 * and allows to serialize values efficiently to a byte stream.
 * Key is an integer and should be defined by the user in an enumeration.
 *
 * Limitation for strings and byte arrays are a max of 2^32-1 characters.
 *
 * Types and storage size is set internally depending on values.
 *
 * Class is hashable and comparable.
 */
class Prop
{
public:
  /* Constructs an invalid value which is not saved to streams. */
  explicit Prop()
    : type(ptinternal::INVALID)
  {
  }

  /* Valid property without value. */
  explicit Prop(int keyParam)
    : key(keyParam), type(ptinternal::NONE)
  {
  }

  /* Takes value from variant and type by using best guess. */
  explicit Prop(int keyParam, const QVariant& valueParam);

  /* Boolean value */
  explicit Prop(int keyParam, bool valueParam)
    : key(keyParam), type(ptinternal::BOOL)
  {
    number.value = valueParam;
  }

  /* Width and type of numeric integer values is set by number to get the smallest number of bytes to save. */
  explicit Prop(int keyParam, char valueParam)
    : key(keyParam)
  {
    number.value = valueParam;
    setTypeForInt();
  }

  explicit Prop(int keyParam, unsigned char valueParam)
    : key(keyParam)
  {
    number.value = valueParam;
    setTypeForInt();
  }

  explicit Prop(int keyParam, short valueParam)
    : key(keyParam)
  {
    number.value = valueParam;
    setTypeForInt();
  }

  explicit Prop(int keyParam, unsigned short valueParam)
    : key(keyParam)
  {
    number.value = valueParam;
    setTypeForInt();
  }

  explicit Prop(int keyParam, int valueParam)
    : key(keyParam)
  {
    number.value = valueParam;
    setTypeForInt();
  }

  explicit Prop(int keyParam, unsigned int valueParam)
    : key(keyParam)
  {
    number.value = valueParam;
    setTypeForInt();
  }

  explicit Prop(int keyParam, long long valueParam)
    : key(keyParam)
  {
    number.value = valueParam;
    setTypeForInt();
  }

  /* Values where user has to define type */
  explicit Prop(int keyParam, float valueParam)
    : key(keyParam), type(ptinternal::FLOAT)
  {
    number.floatValue = valueParam;
  }

  explicit Prop(int keyParam, double valueParam)
    : key(keyParam), type(ptinternal::DOUBLE)
  {
    number.doubleValue = valueParam;
  }

  /* Length prefix is determined by string length */
  explicit Prop(int keyParam, const QString& valueParam)
    : key(keyParam)
  {
    bytes = valueParam.toUtf8();
    setTypeForString();
  }

  /* Length prefix is determined by byte array length */
  explicit Prop(int keyParam, const QByteArray& valueParam)
    : key(keyParam)
  {
    bytes = valueParam;
    setTypeForBytes();
  }

  bool operator==(const Prop& other) const;

  bool operator!=(const Prop& other) const
  {
    return !operator==(other);
  }

  /* Key for accessing in multi hash */
  int getKey() const
  {
    return key;
  }

  template<typename KEY>
  KEY getKey() const
  {
    return static_cast<KEY>(key);
  }

  QVariant getValueVariant() const;

  /* Class does not check type on value getters. This is up to the user. */

  bool getValueBool() const
  {
    return static_cast<bool>(number.value);
  }

  char getValueByte() const
  {
    return static_cast<char>(number.value);
  }

  unsigned char getValueUByte() const
  {
    return static_cast<unsigned char>(number.value);
  }

  short getValueShort() const
  {
    return static_cast<short>(number.value);
  }

  unsigned short getValueUShort() const
  {
    return static_cast<unsigned short>(number.value);
  }

  int getValueInt() const
  {
    return static_cast<int>(number.value);
  }

  unsigned int getValueUInt() const
  {
    return static_cast<unsigned int>(number.value);
  }

  long long getValueLongLong() const
  {
    return number.value;
  }

  float getValueFloat() const
  {
    return number.floatValue;
  }

  double getValueDouble() const
  {
    return number.doubleValue;
  }

  QString getValueString() const
  {
    return QString(bytes);
  }

  QByteArray getValueBytes() const
  {
    return bytes;
  }

  /* false for default constructed property values */
  bool isValid() const
  {
    return type != ptinternal::INVALID;
  }

private:
  void setTypeForInt();
  void setTypeForBytes();
  void setTypeForString();

  template<typename TYPE>
  static void readIntType(QDataStream& in, Prop& prop);

  template<typename TYPE>
  static void readBytesType(QDataStream& in, Prop& prop);

  friend QDataStream& operator<<(QDataStream& out, const atools::util::Prop& prop);
  friend QDataStream& operator>>(QDataStream& in, atools::util::Prop& prop);
  friend size_t qHash(const atools::util::Prop& prop);
  friend QDebug operator<<(QDebug out, const atools::util::Prop& prop);

  /* Hash key as passed in by user in constructors */
  int key = 0;

  /* Value type */
  ptinternal::PropType type;

  /* Union for all integral data types */
  union
  {
    long long value;
    float floatValue;
    double doubleValue;
  } number;

  /* Byte array for string and bytes */
  QByteArray bytes;
};

/*
 *  Multi map for saving, reading and storing property values.
 *  Invalid values are not saved.
 *
 *  Maximum number of properties is limited to 65535.
 *
 * Class is hashable.
 */
class Props :
  public QMultiHash<int, Prop>
{
public:
  Props()
  {
  }

  Props(std::initializer_list<atools::util::Prop>& props)
  {
    for(const atools::util::Prop& prop : props)
      addProp(prop);
  }

  Props(const QList<atools::util::Prop>& props)
  {
    addProps(props);
  }

  atools::util::Prop getProp(int key) const
  {
    return value(key);
  }

  QList<atools::util::Prop> getProps(int key) const
  {
    return values(key);
  }

  void addProp(const atools::util::Prop& prop)
  {
    insert(prop.getKey(), prop);
  }

  void addProps(const QList<atools::util::Prop>& props)
  {
    for(const atools::util::Prop& prop:props)
      insert(prop.getKey(), prop);
  }

private:
  friend QDataStream& operator<<(QDataStream& out, const atools::util::Props& props);
  friend QDataStream& operator>>(QDataStream& in, atools::util::Props& props);

  typedef quint16 propsSizeType;
  const static int MAX_PROPS_SIZE = std::numeric_limits<propsSizeType>::max();
};

size_t qHash(const atools::util::Prop& prop);
size_t qHash(const atools::util::Props& props);

template<typename TYPE>
void Prop::readIntType(QDataStream& in, Prop& prop)
{
  TYPE num;
  in >> num;
  prop.number.value = num;
}

template<typename TYPE>
void Prop::readBytesType(QDataStream& in, Prop& prop)
{
  TYPE size;
  in >> size;
  prop.bytes.resize(size);
  in.readRawData(prop.bytes.data(), size);
}

} // namespace util
} // namespace atools

#endif // ATOOLS_PROPS_H
