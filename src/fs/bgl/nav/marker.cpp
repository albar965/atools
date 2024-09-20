/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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

#include "fs/bgl/nav/marker.h"
#include "fs/bgl/converter.h"
#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

QString Marker::markerTypeToStr(nav::MarkerType type)
{
  switch(type)
  {
    case nav::INNER:
      return "INNER";

    case nav::MIDDLE:
      return "MIDDLE";

    case nav::OUTER:
      return "OUTER";

    case nav::BACKCOURSE:
      return "BACKCOURSE";
  }
  qWarning().nospace().noquote() << "Invalid marker type " << type;
  return "INVALID";
}

bool Marker::isValid() const
{
  return position.getPos().isValid() && !position.getPos().isPole() && !position.getPos().isNull();
}

QString Marker::getObjectName() const
{
  return Record::getObjectName() + QString("marker ident %1 region %2 position %3").
         arg(ident).arg(region).arg(position.getPos().toString());
}

Marker::Marker(const NavDatabaseOptions *options, BinaryStream *stream)
  : Record(options, stream)
{
  // TODO wiki clarify structure
  heading = static_cast<float>(stream->readUByte()) / 255.f * 360.f;
  type = static_cast<nav::MarkerType>(stream->readUByte());

  position = BglPosition(stream, true, 1000.f);
  ident = converter::intToIcao(stream->readUInt());
  region = converter::intToIcao(stream->readUInt()); // TODO wiki is always null
}

Marker::~Marker()
{
}

QDebug operator<<(QDebug out, const Marker& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record)
                          << " Marker[type "
                          << Marker::markerTypeToStr(record.type)
                          << ", " << record.position
                          << ", ident " << record.ident
                          << ", region " << record.region
                          << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
