/*
 * BglBase.cpp
 *
 *  Created on: 19.04.2015
 *      Author: alex
 */

#include "fs/bgl/bglbase.h"
#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

QDebug operator<<(QDebug out, const BglBase& base)
{
  QDebugStateSaver saver(out);
  out.nospace().noquote() << hex << " BglBase[start 0x" << base.startOffset << "]";
  return out;
}

BglBase::~BglBase()
{
}

void BglBase::seekToStart()
{
  bs->seekg(startOffset);
}

BglBase::BglBase()
  : startOffset(0), bs(nullptr)
{
}

BglBase::BglBase(io::BinaryStream *stream)
  : startOffset(stream->tellg()), bs(stream)
{
}

} // namespace bgl
} // namespace fs
} // namespace atools
