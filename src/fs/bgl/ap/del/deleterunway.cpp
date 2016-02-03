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

#include "fs/bgl/ap/del/deleterunway.h"
#include "io/binarystream.h"
#include "fs/bgl/converter.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

DeleteRunway::DeleteRunway(const atools::fs::BglReaderOptions *options, BinaryStream *bs)
  : BglBase(options, bs), surface(rw::UNKNOWN)
{
  surface = static_cast<rw::Surface>(bs->readUByte());
  int numPrim = bs->readUByte();
  int numSec = bs->readUByte();
  int desig = bs->readUByte();
  primaryName = converter::runwayToStr(numPrim, desig & 0x0f);
  secondaryName = converter::runwayToStr(numSec, (desig >> 4) & 0x0f);
}

QDebug operator<<(QDebug out, const DeleteRunway& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const BglBase&>(record)
  << "DeleteRunway[prim name " << record.primaryName
  << ", sec name " << record.secondaryName
  << ", surface " << Runway::surfaceToStr(record.surface) << "]";
  return out;
}

DeleteRunway::~DeleteRunway()
{
}

} // namespace bgl
} // namespace fs
} // namespace atools
