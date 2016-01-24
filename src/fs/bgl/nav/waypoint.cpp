/*
 * Waypoint.cpp
 *
 *  Created on: 25.04.2015
 *      Author: alex
 */

#include "waypoint.h"
#include "../converter.h"
#include "../bglposition.h"
#include "routeentry.h"

#include <iterator>

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

QString Waypoint::waypointTypeToStr(nav::WaypointType type)
{
  switch(type)
  {
    case nav::NAMED:
      return "NAMED";

    case nav::UNNAMED:
      return "UNNAMED";

    case nav::VOR:
      return "VOR";

    case nav::NDB:
      return "NDB";

    case nav::OFF_ROUTE:
      return "OFF_ROUTE";

    case nav::IAF:
      return "IAF";

    case nav::FAF:
      return "FAF";
  }
  qWarning().nospace().noquote() << "Unknown waypoint type " << type;
  return "";
}

Waypoint::Waypoint(BinaryStream *bs)
  : Record(bs)
{
  type = static_cast<nav::WaypointType>(bs->readByte());
  int numRoutes = bs->readByte();
  position = BglPosition(bs, 1.f, false);
  magVar = bs->readFloat();
  unsigned int identInt = bs->readUInt();
  ident = converter::intToIcao(identInt);

  unsigned int regionFlags = bs->readUInt();
  region = converter::intToIcao(regionFlags & 0x7ff, true);
  airportIdent = converter::intToIcao((regionFlags >> 11) & 0x1fffff, true);

  if(region.isEmpty())
    qWarning().nospace().noquote() << "Waypoint at " << position << " ident " << ident << " has no region";

  if(ident.isEmpty() && type != nav::UNNAMED)
    qWarning().nospace().noquote() << "Waypoint at " << position << " region " << region << " has no ident";

  for(int i = 0; i < numRoutes; i++)
    routes.push_back(RouteEntry(bs));
}

Waypoint::~Waypoint()
{
}

QDebug operator<<(QDebug out, const Waypoint& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record)
  << " Waypoint[type " << Waypoint::waypointTypeToStr(record.type)
  << ", " << record.position
  << ", magVar " << record.magVar
  << ", ident " << record.ident
  << ", region " << record.region
  << ", airport ID " << record.airportIdent << endl;
  out << record.routes;
  out << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
