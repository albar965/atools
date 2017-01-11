/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

#include "fs/bgl/ap/helipad.h"
#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

QString Helipad::helipadTypeToStr(helipad::HelipadType type)
{
  switch(type)
  {
    case atools::fs::bgl::helipad::NONE:
      return "NONE";

    case atools::fs::bgl::helipad::H:
      return "H";

    case atools::fs::bgl::helipad::SQUARE:
      return "SQUARE";

    case atools::fs::bgl::helipad::CIRCLE:
      return "CIRCLE";

    case atools::fs::bgl::helipad::MEDICAL:
      return "MEDICAL";
  }
  qWarning().nospace().noquote() << "Invalid helipad type " << type;
  return "INVALID";
}

Helipad::Helipad()
{
}

Helipad::Helipad(const NavDatabaseOptions *options, BinaryStream *bs)
  : Record(options, bs)
{
  surface = static_cast<rw::Surface>(bs->readUByte() & rw::SURFACE_MASK);

  int flags = bs->readUByte();
  type = static_cast<helipad::HelipadType>(flags & 0xf);
  transparent = flags & (1 << 4);
  closed = flags & (1 << 5);

  bs->skip(4); // color

  position = BglPosition(bs, true, 1000.f);
  length = bs->readFloat();
  width = bs->readFloat();
  heading = bs->readFloat(); // TODO wiki heading is float degrees
}

QDebug operator<<(QDebug out, const Helipad& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record)
  << " Helipad[type " << Helipad::helipadTypeToStr(record.type)
  << ", surface " << Runway::surfaceToStr(record.surface) << endl
  << ", length " << record.length
  << ", width " << record.width
  << ", heading " << record.heading
  << ", transparent " << record.transparent
  << ", closed " << record.closed
  << ", " << record.position
  << "]";

  return out;
}

Helipad::~Helipad()
{
}

} // namespace bgl
} // namespace fs
} // namespace atools
