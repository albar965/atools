/*
 * Subsection.cpp
 *
 *  Created on: 19.04.2015
 *      Author: alex
 */

#include "fs/bgl/subsection.h"
#include "fs/bgl/section.h"

#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {
using atools::io::BinaryStream;

Subsection::Subsection(BinaryStream *bs, const Section& parentSect)
  : BglBase(bs), parent(&parentSect)
{
  id = bs->readInt();
  numDataRecords = bs->readInt();
  firstDataRecordOffset = bs->readInt();
  dataSize = bs->readInt();
}

Subsection::~Subsection()
{
}

QDebug operator<<(QDebug out, const Subsection& section)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const BglBase&>(section)
  << " Subsection[parent type " << sectionTypeStr(section.getParent().getType())
  << hex << ", id 0x" << section.id << dec
  << ", records " << section.numDataRecords
  << hex << ", offset 0x" << section.firstDataRecordOffset << dec
  << ", size " << section.dataSize << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
