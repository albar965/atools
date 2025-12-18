/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#include "fs/bgl/section.h"

#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

Section::Section(const atools::fs::NavDatabaseOptions *options, BinaryStream *stream)
  : BglBase(options, stream)
{
  type = static_cast<section::SectionType>(stream->readUInt());
  unsigned int sizeFlag = stream->readUInt();

  // Most subsections have a size of 16 bytes. Only subsections for terrain seasons have a size of 20 bytes
  subsectionSize = ((sizeFlag & 0x10000) | 0x40000) >> 0x0E;
  numSubsections = stream->readUInt();
  firstSubsectionOffset = stream->readUInt();
  totalSubsectionSize = stream->readUInt();
}

Section::~Section()
{
}

QDebug operator<<(QDebug out, const Section& section)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const BglBase&>(section)
                          << " Section[type " << sectionTypeStr(section.type)
                          << ", size " << section.subsectionSize
                          << ", subsections " << section.numSubsections
                          << Qt::hex << ", offset 0x" << section.firstSubsectionOffset << Qt::dec
                          << ", total size " << section.totalSubsectionSize << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
