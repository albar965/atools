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

#include "fs/bgl/subsection.h"
#include "fs/bgl/section.h"

#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

Subsection::Subsection(const atools::fs::NavDatabaseOptions *options, BinaryStream *stream, const Section& parentSect)
  : BglBase(options, stream), parent(&parentSect)
{
  id = stream->readInt();
  numDataRecords = stream->readInt();
  firstDataRecordOffset = stream->readInt();
  dataSize = stream->readInt();
}

Subsection::~Subsection()
{
}

QDebug operator<<(QDebug out, const Subsection& section)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const BglBase&>(section)
                          << " Subsection[parent type " << sectionTypeStr(section.getParent().getType())
                          << Qt::hex << ", id 0x" << section.id << Qt::dec
                          << ", records " << section.numDataRecords
                          << Qt::hex << ", offset 0x" << section.firstDataRecordOffset << Qt::dec
                          << ", size " << section.dataSize << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
