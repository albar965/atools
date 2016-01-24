/*
 * Record.cpp
 *
 *  Created on: 19.04.2015
 *      Author: alex
 */

#include "record.h"

namespace atools {
namespace fs {
namespace bgl {
using atools::io::BinaryStream;

Record::Record(BinaryStream *bs)
  : BglBase(bs)
{
  id = bs->readShort();
  size = bs->readInt();
}

Record::~Record()
{
}

QDebug operator<<(QDebug out, const Record& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const BglBase&>(record)
  << hex << " Record[id 0x" << record.id << dec
  << ", size " << record.size << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
