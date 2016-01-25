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

#include "fs/bgl/bglposition.h"

#include "fs/bgl/converter.h"
#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {
using atools::io::BinaryStream;

BglPosition::BglPosition(BinaryStream *bs, float altitudeFactor, bool hasAltitude)
{
  lonX = converter::intToLonX(bs->readInt());
  latY = converter::intToLatY(bs->readInt());
  if(hasAltitude)
    altitude = static_cast<float>(bs->readInt()) / altitudeFactor;
  else
    altitude = 0.f;
}

BglPosition::~BglPosition()
{
}

QDebug operator<<(QDebug out, const BglPosition& pos)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << "Pos[lonX " << pos.lonX << ", latY " << pos.latY <<
  ", alt " << pos.altitude << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
