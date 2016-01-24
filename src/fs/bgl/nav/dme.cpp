/*
 * Dme.cpp
 *
 *  Created on: 26.04.2015
 *      Author: alex
 */

#include "dme.h"

#include "../bglposition.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

Dme::Dme(BinaryStream *bs)
  : Record(bs)
{
  bs->skip(2); // Unknown
  position = BglPosition(bs, 1000.f);
  range = bs->readFloat();
}

Dme::~Dme()
{
}

QDebug operator<<(QDebug out, const Dme& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record)
  << " Dme["
  << "range " << record.range
  << ", " << record.position
  << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
