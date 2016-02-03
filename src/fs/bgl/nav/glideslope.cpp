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

#include "fs/bgl/nav/glideslope.h"

#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

Glideslope::Glideslope(const BglReaderOptions *options, BinaryStream *bs)
  : Record(options, bs)
{
  bs->skip(2); // Unknown
  position = BglPosition(bs, true, 1000.f);
  range = bs->readFloat();
  pitch = bs->readFloat();
}

Glideslope::~Glideslope()
{
}

QDebug operator<<(QDebug out, const Glideslope& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record)
  << " Glideslope["
  << "range " << record.range
  << ", pitch " << record.pitch
  << ", " << record.position
  << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
