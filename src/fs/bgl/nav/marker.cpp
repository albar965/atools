/*
 * Marker.cpp
 *
 *  Created on: 25.04.2015
 *      Author: alex
 */

#include "fs/bgl/nav/marker.h"
#include "fs/bgl/converter.h"
#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

QString Marker::markerTypeToStr(nav::MarkerType type)
{
  switch(type)
  {
    case nav::INNER:
      return "INNER";

    case nav::MIDDLE:
      return "MIDDLE";

    case nav::OUTER:
      return "OUTER";

    case nav::BACKCOURSE:
      return "BACKCOURSE";
  }
  qWarning().nospace().noquote() << "Unknown marker type " << type;
  return "";
}

Marker::Marker(BinaryStream *bs)
  : Record(bs)
{
  // unsigned int heading = bs->readByte();
  bs->skip(1);
  type = static_cast<nav::MarkerType>(bs->readByte());

  position = BglPosition(bs);
  ident = converter::intToIcao(bs->readUInt());

  region = converter::intToIcao(bs->readUShort(), true);
  bs->skip(2);
}

Marker::~Marker()
{
}

QDebug operator<<(QDebug out, const Marker& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record)
  << " Marker[type "
  << Marker::markerTypeToStr(record.type)
  << ", " << record.position
  << ", ident " << record.ident
  << ", region " << record.region
  << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
