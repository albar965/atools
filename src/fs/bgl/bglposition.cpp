/*
 * Pos.cpp
 *
 *  Created on: 21.04.2015
 *      Author: alex
 */

#include "bglposition.h"

#include "converter.h"
#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {
using atools::io::BinaryStream;

BglPosition::BglPosition(BinaryStream *bs, float altitudeFactor, bool hasAltitude)
{
  lonX = converter::intToLonX(bs->readInt());
  latY = converter::intToLatY(bs->readInt());
  if(hasAltitude)
    altitude = static_cast<float>(bs->readInt()) / altitudeFactor;
  else
    altitude = 0.f;
}

BglPosition::~BglPosition()
{
}

QDebug operator<<(QDebug out, const BglPosition& pos)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << "Pos[lonX " << pos.lonX << ", latY " << pos.latY <<
  ", alt " << pos.altitude << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
