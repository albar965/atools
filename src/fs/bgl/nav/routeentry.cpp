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

#include "fs/bgl/nav/routeentry.h"
#include "fs/bgl/converter.h"
#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

QString RouteEntry::routeTypeToStr(nav::RouteType type)
{
  switch(type)
  {
    case nav::NONE:
      return "NONE";

    case nav::VICTOR:
      return "VICTOR";

    case nav::JET:
      return "JET";

    case nav::BOTH:
      return "BOTH";
  }
  qWarning().nospace().noquote() << "Unknown route type " << type;
  return "";
}

RouteEntry::RouteEntry(BinaryStream *bs)
  : BglBase(bs)
{
  type = static_cast<nav::RouteType>(bs->readByte());
  name = bs->readString(8);

  next = RouteWaypoint(bs);
  previous = RouteWaypoint(bs);
}

RouteEntry::~RouteEntry()
{
}

bool RouteEntry::hasNextWaypoint() const
{
  return next.getType() != nav::ROUTE_WP_NONE;
}

bool RouteEntry::hasPreviousWaypoint() const
{
  return previous.getType() != nav::ROUTE_WP_NONE;
}

QDebug operator<<(QDebug out, const RouteEntry& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const BglBase&>(record)
  << " RouteEntry[type " << RouteEntry::routeTypeToStr(record.type)
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
