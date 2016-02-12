/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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

Subsection::Subsection(const atools::fs::BglReaderOptions *options,
                       BinaryStream *bs,
                       const Section& parentSect)
  : BglBase(options, bs), parent(&parentSect)
{
  id = bs->readInt();
  // int id2 = bs->readInt();
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
