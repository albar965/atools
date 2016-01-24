/*
 * RouteEntry.cpp
 *
 *  Created on: 10.05.2015
 *      Author: alex
 */

#include "routeentry.h"
#include "../converter.h"

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
