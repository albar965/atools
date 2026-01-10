/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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

#include "util/props.h"
#include "atools.h"

#include <QVariant>

namespace atools {
namespace util {

using namespace atools::util::ptinternal;

size_t qHash(const atools::util::Prop& prop)
{
  size_t hash = 43;
  switch(prop.type)
  {
    case BOOL:
    case INT8:
    case UINT8:
    case INT16:
    case UINT16:
    case INT32:
    case UINT32:
    case INT64:
      hash ^= ::qHash(prop.getValueLongLong());
      break;

    case FLOAT:
      hash ^= ::qHash(prop.getValueFloat());
      break;

    case DOUBLE:
      hash ^= ::qHash(prop.getValueDouble());
      break;

    case STRING8:
    case STRING16:
    case STRING32:
    case BYTES8:
    case BYTES16:
    case BYTES32:
      hash ^= ::qHash(prop.getValueBytes());
      break;

    case INVALID:
    case NONE:
      break;
  }

  return ::qHash(prop.key) ^ hash;
}

size_t qHash(const atools::util::Props& props)
{
  size_t retval = 43;
  for(auto it = props.constBegin(); it != props.constEnd(); ++it)
    retval ^= qHash(it.value());

  return retval;
}

QDataStream& operator<<(QDataStream& out, const Prop& prop)
{
  out << static_cast<quint8>(prop.key);
  out << static_cast<quint8>(prop.type);

  switch(prop.type)
  {
    case INT8:
      out << static_cast<qint8>(prop.number.value);
      break;

    case BOOL:
    case UINT8:
      out << static_cast<quint8>(prop.number.value);
      break;

    case INT16:
      out << static_cast<qint16>(prop.number.value);
      break;

    case UINT16:
      out << static_cast<quint16>(prop.number.value);
      break;

    case INT32:
      out << static_cast<qint32>(prop.number.value);
      break;

    case UINT32:
      out << static_cast<quint32>(prop.number.value);
      break;

    case INT64:
      out << static_cast<qint64>(prop.number.value);
      break;

    case FLOAT:
      out << prop.number.floatValue;
      break;

    case DOUBLE:
      out << prop.number.doubleValue;
      break;

    case BYTES8:
    case STRING8:
      out << static_cast<quint8>(prop.bytes.size());
      out.writeRawData(prop.bytes.constData(), prop.bytes.size());
      break;

    case BYTES16:
    case STRING16:
      out << static_cast<quint16>(prop.bytes.size());
      out.writeRawData(prop.bytes.constData(), prop.bytes.size());
      break;

    case BYTES32:
    case STRING32:
      out << static_cast<quint32>(prop.bytes.size());
      out.writeRawData(prop.bytes.constData(), prop.bytes.size());
      break;

    case NONE:
    case INVALID:
      break;
  }
  return out;
}

QDataStream& operator>>(QDataStream& in, Prop& prop)
{
  quint8 value8;

  in >> value8;
  prop.key = value8;

  in >> value8;
  prop.type = static_cast<ptinternal::PropType>(value8);

  switch(prop.type)
  {
    case INT8:
      Prop::readIntType<qint8>(in, prop);
      break;

    case BOOL:
    case UINT8:
      Prop::readIntType<quint8>(in, prop);
      break;

    case INT16:
      Prop::readIntType<qint16>(in, prop);
      break;

    case UINT16:
      Prop::readIntType<quint16>(in, prop);
      break;

    case INT32:
      Prop::readIntType<qint32>(in, prop);
      break;

    case UINT32:
      Prop::readIntType<quint32>(in, prop);
      break;

    case INT64:
      Prop::readIntType<qint64>(in, prop);
      break;

    case FLOAT:
      {
        float num;
        in >> num;
        prop.number.floatValue = num;
      }
      break;

    case DOUBLE:
      {
        double num;
        in >> num;
        prop.number.doubleValue = num;
      }
      break;

    case STRING8:
    case BYTES8:
      Prop::readBytesType<quint8>(in, prop);
      break;

    case STRING16:
    case BYTES16:
      Prop::readBytesType<quint16>(in, prop);
      break;

    case STRING32:
    case BYTES32:
      Prop::readBytesType<quint32>(in, prop);
      break;

    case INVALID:
    case NONE:
      break;
  }

  return in;
}

QDataStream& operator<<(QDataStream& out, const Props& props)
{
  int maxSize = atools::util::Props::MAX_PROPS_SIZE;
  int size = std::min(static_cast<qsizetype>(maxSize), static_cast<qsizetype>(props.size()));

  int write = 0;
  int idx = 0;
  for(auto it = props.constBegin(); it != props.constEnd(); ++it)
  {
    if(++idx > size)
      break;

    if(it.value().isValid())
      write++;
  }

  out << static_cast<Props::propsSizeType>(write);

  idx = 0;
  for(auto it = props.constBegin(); it != props.constEnd(); ++it)
  {
    if(++idx > size)
      break;

    if(it.value().isValid())
      out << it.value();
  }
  return out;
}

QDataStream& operator>>(QDataStream& in, Props& props)
{
  Props::propsSizeType size;
  in >> size;

  for(int i = 0; i < size; i++)
  {
    Prop prop;
    in >> prop;
    props.insert(prop.getKey(), prop);
  }

  return in;
}

Prop::Prop(int keyParam, const QVariant& valueParam)
  : key(keyParam)
{
  QMetaType::Type metatype = static_cast<QMetaType::Type>(valueParam.metaType().id());
  switch(metatype)
  {
    case QMetaType::Bool:
      type = ptinternal::BOOL;
      number.value = valueParam.toBool();
      break;

    case QMetaType::Char:
    case QMetaType::SChar:
    case QMetaType::UChar:
    case QMetaType::Short:
    case QMetaType::UShort:
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::Long:
    case QMetaType::ULong:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
      number.value = valueParam.toLongLong();
      setTypeForInt();
      break;

    case QMetaType::QString:
      bytes = valueParam.toString().toUtf8();
      setTypeForString();
      break;

    case QMetaType::QByteArray:
      bytes = valueParam.toByteArray();
      setTypeForBytes();
      break;

    case QMetaType::Double:
      type = ptinternal::DOUBLE;
      number.doubleValue = valueParam.toDouble();
      break;

    case QMetaType::Float:
      type = ptinternal::FLOAT;
      number.floatValue = valueParam.toFloat();
      break;

    default:
      type = ptinternal::INVALID;
      break;
  }
}

void Prop::setTypeForBytes()
{
  if(bytes.size() <= std::numeric_limits<qint8>::max())
    type = BYTES8;
  else if(bytes.size() <= std::numeric_limits<qint16>::max())
    type = BYTES16;
  else
    type = BYTES32;
}

void Prop::setTypeForString()
{
  if(bytes.size() <= std::numeric_limits<qint8>::max())
    type = STRING8;
  else if(bytes.size() <= std::numeric_limits<qint16>::max())
    type = STRING16;
  else
    type = STRING32;
}

void Prop::setTypeForInt()
{
  if(number.value >= std::numeric_limits<quint8>::min() &&
     number.value <= std::numeric_limits<quint8>::max())
    type = UINT8;
  else if(number.value >= std::numeric_limits<qint8>::min() &&
          number.value <= std::numeric_limits<qint8>::max())
    type = INT8;
  else if(number.value >= std::numeric_limits<quint16>::min() &&
          number.value <= std::numeric_limits<quint16>::max())
    type = UINT16;
  else if(number.value >= std::numeric_limits<qint16>::min() &&
          number.value <= std::numeric_limits<qint16>::max())
    type = INT16;
  else if(number.value >= std::numeric_limits<quint32>::min() &&
          number.value <= std::numeric_limits<quint32>::max())
    type = UINT32;
  else if(number.value >= std::numeric_limits<qint32>::min() &&
          number.value <= std::numeric_limits<qint32>::max())
    type = INT32;
  else
    type = INT64;
}

bool Prop::operator==(const Prop& other) const
{
  if(key == other.key)
  {
    switch(type)
    {
      case atools::util::BOOL:
      case atools::util::INT8:
      case atools::util::UINT8:
      case atools::util::INT16:
      case atools::util::UINT16:
      case atools::util::INT32:
      case atools::util::UINT32:
      case atools::util::INT64:
        return number.value == other.number.value;

      case atools::util::FLOAT:
        return atools::almostEqual(number.floatValue, other.number.floatValue);

      case atools::util::DOUBLE:
        return atools::almostEqual(number.doubleValue, other.number.doubleValue);

      case atools::util::STRING8:
      case atools::util::BYTES8:
      case atools::util::STRING16:
      case atools::util::BYTES16:
      case atools::util::STRING32:
      case atools::util::BYTES32:
        return bytes == other.bytes;

      case atools::util::NONE:
      case atools::util::INVALID:
        return true;
    }
  }
  return false;
}

QVariant Prop::getValueVariant() const
{
  switch(type)
  {
    case atools::util::NONE:
    case atools::util::INVALID:
      return QVariant();

    case atools::util::BOOL:
      return QVariant::fromValue(getValueBool());

    case atools::util::INT8:
    case atools::util::INT16:
    case atools::util::INT32:
      return QVariant::fromValue(getValueInt());

    case atools::util::UINT8:
    case atools::util::UINT16:
    case atools::util::UINT32:
      return QVariant::fromValue(getValueUInt());

    case atools::util::INT64:
      return QVariant::fromValue(getValueLongLong());

    case atools::util::FLOAT:
      return QVariant::fromValue(getValueFloat());

    case atools::util::DOUBLE:
      return QVariant::fromValue(getValueDouble());

    case atools::util::STRING8:
    case atools::util::STRING16:
    case atools::util::STRING32:
      return QVariant::fromValue(getValueString());

    case atools::util::BYTES8:
    case atools::util::BYTES16:
    case atools::util::BYTES32:
      return QVariant::fromValue(getValueBytes());

  }
  return QVariant();
}

QDebug operator<<(QDebug out, const atools::util::Prop& prop)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << "Prop[key " << prop.key << ", ";

  switch(prop.type)
  {
    case atools::util::INVALID:
      out << "INVALID";
      break;
    case atools::util::NONE:
      out << "NONE";
      break;
    case atools::util::BOOL:
      out << "BOOL, " << prop.getValueBool();
      break;
    case atools::util::INT8:
      out << "INT8, " << prop.getValueInt();
      break;
    case atools::util::UINT8:
      out << "UINT8, " << prop.getValueUInt();
      break;
    case atools::util::INT16:
      out << "INT16, " << prop.getValueShort();
      break;
    case atools::util::UINT16:
      out << "UINT16, " << prop.getValueUShort();
      break;
    case atools::util::INT32:
      out << "INT32, " << prop.getValueInt();
      break;
    case atools::util::UINT32:
      out << "UINT32, " << prop.getValueUInt();
      break;
    case atools::util::INT64:
      out << "INT64, " << prop.getValueLongLong();
      break;
    case atools::util::FLOAT:
      out << "FLOAT, " << prop.getValueFloat();
      break;
    case atools::util::DOUBLE:
      out << "DOUBLE, " << prop.getValueDouble();
      break;
    case atools::util::STRING8:
      out << "STRING8, " << prop.getValueString();
      break;
    case atools::util::STRING16:
      out << "STRING16, " << prop.getValueString();
      break;
    case atools::util::STRING32:
      out << "STRING32, " << prop.getValueString();
      break;
    case atools::util::BYTES8:
      out << "BYTES8, " << prop.getValueBytes().toHex(',');
      break;
    case atools::util::BYTES16:
      out << "BYTES16, " << prop.getValueBytes().toHex(',');
      break;
    case atools::util::BYTES32:
      out << "BYTES32, " << prop.getValueBytes().toHex(',');
      break;
  }

  out << "]";

  return out;
}

}
}
