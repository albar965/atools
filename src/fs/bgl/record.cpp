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

#include "fs/bgl/record.h"
#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {
using atools::io::BinaryStream;

Record::Record(const BglReaderOptions *options, BinaryStream *bs)
  : BglBase(options, bs)
{
  id = bs->readShort();
  size = bs->readInt();
}

Record::~Record()
{
}

void Record::seekToEnd() const
{
  bs->seekg(startOffset + size);
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
