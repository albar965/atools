/*
 * DeleteCom.cpp
 *
 *  Created on: 18.05.2015
 *      Author: alex
 */

#include "deletecom.h"
#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

DeleteCom::DeleteCom(BinaryStream *bs)
  : BglBase(bs)
{
  unsigned int flags = bs->readUInt();
  type = static_cast<com::ComType>((flags >> 28) & 0xf);
  frequency = (flags & 0x0fffffff) / 1000;
}

QDebug operator<<(QDebug out, const DeleteCom& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const BglBase&>(record)
  << "DeleteCom[type " << record.type
  << ", frequency " << record.frequency << "]";
  return out;
}

DeleteCom::~DeleteCom()
{
}

} // namespace bgl
} // namespace fs
} // namespace atools
