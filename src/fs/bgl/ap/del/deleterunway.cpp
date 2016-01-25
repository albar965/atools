/*
 * DeleteRunwayX.cpp
 *
 *  Created on: 18.05.2015
 *      Author: alex
 */

#include "fs/bgl/ap/del/deleterunway.h"
#include "io/binarystream.h"
#include "fs/bgl/converter.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

DeleteRunway::DeleteRunway(BinaryStream *bs)
  : BglBase(bs), surface(rw::UNKNOWN)
{
  surface = static_cast<rw::Surface>(bs->readByte());
  int numPrim = bs->readByte();
  int numSec = bs->readByte();
  int desig = bs->readByte();
  primaryName = converter::runwayToStr(numPrim, desig & 0x0f);
  secondaryName = converter::runwayToStr(numSec, (desig >> 4) & 0x0f);
}

QDebug operator<<(QDebug out, const DeleteRunway& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const BglBase&>(record)
  << "DeleteRunway[prim name " << record.primaryName
  << ", sec name " << record.secondaryName
  << ", surface " << Runway::surfaceToStr(record.surface) << "]";
  return out;
}

DeleteRunway::~DeleteRunway()
{
}

} // namespace bgl
} // namespace fs
} // namespace atools
