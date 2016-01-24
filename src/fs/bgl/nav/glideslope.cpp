/*
 * Glideslope.cpp
 *
 *  Created on: 26.04.2015
 *      Author: alex
 */

#include "glideslope.h"

#include "../bglposition.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

Glideslope::Glideslope(BinaryStream *bs)
  : Record(bs)
{
  bs->skip(2); // Unknown
  position = BglPosition(bs, 1000.f);
  range = bs->readFloat();
  pitch = bs->readFloat();
}

Glideslope::~Glideslope()
{
}

QDebug operator<<(QDebug out, const Glideslope& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record)
  << " Glideslope["
  << "range " << record.range
  << ", pitch " << record.pitch
  << ", " << record.position
  << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
