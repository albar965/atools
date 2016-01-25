/*
 * Section.cpp
 *
 *  Created on: 19.04.2015
 *      Author: alex
 */

#include "fs/bgl/section.h"

#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {
using atools::io::BinaryStream;

Section::Section(BinaryStream *bs)
  : BglBase(bs)
{
  type = static_cast<section::SectionType>(bs->readInt());
  size = ((bs->readInt() & 0x10000) | 0x40000) >> 0x0E;
  numSubsections = bs->readInt();
  firstSubsectionOffset = bs->readInt();
  totalSubsectionSize = bs->readInt();
}

Section::~Section()
{
}

QDebug operator<<(QDebug out, const Section& section)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const BglBase&>(section)
  << " Section[type " << sectionTypeStr(section.type)
  << ", size " << section.size
  << ", subsections " << section.numSubsections
  << hex << ", offset 0x" << section.firstSubsectionOffset << dec
  << ", total size " << section.totalSubsectionSize << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
