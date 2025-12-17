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

#include "fs/bgl/record.h"
#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {

using Qt::hex;
using Qt::dec;

using atools::io::BinaryStream;

Record::Record(const NavDatabaseOptions *options, BinaryStream *stream)
  : BglBase(options, stream)
{
  id = stream->readUShort();
  size = stream->readUInt();
}

Record::~Record()
{
}

void Record::seekToEnd() const
{
  bs->seekg(startOffset + size);
}

bool Record::isFullyValid() const
{
  return id > 0 && startOffset > 0 && size > 0 && startOffset + size <= bs->getFileSize();
}

QString Record::getObjectName() const
{
  return QString(" Record[offset 0x%1, id 0x%2, size %3] ").
         arg(getStartOffset(), 0, 16).arg(id, 0, 16).arg(size);
}

bool Record::checkSubRecord(const Record& r)
{
  if(r.getSize() == 0)
  {
    qWarning().nospace().noquote() << "Ignoring record. Size of record is zero. 0x" << hex << r.getId<int>() << dec << getObjectName();

    // Stop reading when the first subrecord is already invalid
    seekToStart();
    excluded = true;
    return true;
  }
  return false;
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
