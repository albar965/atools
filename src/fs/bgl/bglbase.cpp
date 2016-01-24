/*
 * BglBase.cpp
 *
 *  Created on: 19.04.2015
 *      Author: alex
 */

#include "bglbase.h"

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

} // namespace bgl
} // namespace fs
} // namespace atools
