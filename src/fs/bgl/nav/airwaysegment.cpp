/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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

#include "fs/bgl/nav/airwaysegment.h"
#include "fs/bgl/converter.h"
#include "io/binarystream.h"
#include "fs/bgl/nav/waypoint.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

QString AirwaySegment::airwayTypeToStr(nav::AirwayType type)
{
  switch(type)
  {
    case nav::NONE:
      return "NONE";

    case nav::VICTOR:
      return "V";

    case nav::JET:
      return "J";

    case nav::BOTH:
      return "B";
  }
  qWarning().nospace().noquote() << "Invalid airway type " << type;
  return "INVALID";
}

AirwaySegment::AirwaySegment(const atools::fs::NavDatabaseOptions *options, BinaryStream *bs,
                             const atools::fs::bgl::Waypoint& waypoint)
  : BglBase(options, bs)
{
  type = static_cast<nav::AirwayType>(bs->readUByte());
  name = bs->readString(8, atools::io::LATIN1);

  mid = AirwayWaypoint(waypoint);
  next = AirwayWaypoint(options, bs);
  previous = AirwayWaypoint(options, bs);
}

AirwaySegment::~AirwaySegment()
{
}

bool AirwaySegment::hasNextWaypoint() const
{
  return !next.getIdent().isEmpty();
}

bool AirwaySegment::hasPreviousWaypoint() const
{
  return !previous.getIdent().isEmpty();
}

QDebug operator<<(QDebug out, const AirwaySegment& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const BglBase&>(record)
                          << " AirwayEntry[type " << AirwaySegment::airwayTypeToStr(record.type)
                          << ", name " << record.name;

  if(record.hasNextWaypoint())
    out << ", next " << record.next;
  if(record.hasPreviousWaypoint())
    out << ", previous " << record.previous;

  out << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
