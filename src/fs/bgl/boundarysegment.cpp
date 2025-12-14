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

#include "fs/bgl/boundarysegment.h"
#include "io/binarystream.h"
#include "fs/bgl/converter.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

QString BoundarySegment::boundarylineTypeToStr(boundaryline::PointType type)
{
  switch(type)
  {
    case atools::fs::bgl::boundaryline::UNKNOWN:
      return "UNKNOWN";

    case atools::fs::bgl::boundaryline::START:
      return "S";

    case atools::fs::bgl::boundaryline::LINE:
      return "L";

    case atools::fs::bgl::boundaryline::ORIGIN:
      return "O";

    case atools::fs::bgl::boundaryline::ARC_CW:
      return "CW";

    case atools::fs::bgl::boundaryline::ARC_CCW:
      return "CCW";

    case atools::fs::bgl::boundaryline::CIRCLE:
      return "C";
  }
  qWarning().nospace().noquote() << "Invalid BOUNDARYLINE type " << type;
  return "INVALID";
}

BoundarySegment::BoundarySegment()
  : type(boundaryline::UNKNOWN)
{
}

BoundarySegment::BoundarySegment(const NavDatabaseOptions *options, BinaryStream *stream)
  : BglBase(options, stream)
{
  type = static_cast<boundaryline::PointType>(stream->readUShort() & 0x7);

  if(type == boundaryline::CIRCLE)
  {
    stream->skip(4); // Skip unused latitude
    radius = stream->readFloat();
  }
  else
  {
    float latY = converter::intToLatY(stream->readInt());
    float lonX = converter::intToLonX(stream->readInt());
    position.setLatY(latY);
    position.setLonX(lonX);
    position.setAltitude(0.f);
  }
}

QDebug operator<<(QDebug out, const BoundarySegment& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const BglBase&>(record)
                          << " Boundaryline[type " << BoundarySegment::boundarylineTypeToStr(record.type)
                          << ", pos " << record.position
                          << "]";

  return out;
}

BoundarySegment::~BoundarySegment()
{
}

} // namespace bgl
} // namespace fs
} // namespace atools
