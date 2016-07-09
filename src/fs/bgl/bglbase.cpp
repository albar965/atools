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

#include "fs/bgl/bglbase.h"
#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

BglBase::BglBase()
  : startOffset(0), bs(nullptr)
{
}

BglBase::BglBase(const atools::fs::BglReaderOptions *options, io::BinaryStream *stream)
  : startOffset(stream->tellg()), bs(stream), opts(options)
{
}

BglBase::~BglBase()
{
}

void BglBase::seekToStart()
{
  bs->seekg(startOffset);
}

QDebug operator<<(QDebug out, const BglBase& base)
{
  QDebugStateSaver saver(out);
  out.nospace().noquote() << hex << " BglBase[start 0x" << base.startOffset << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
