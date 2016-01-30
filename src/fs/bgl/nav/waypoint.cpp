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

#include "fs/bgl/nav/waypoint.h"
#include "fs/bgl/converter.h"
#include "fs/bgl/nav/routeentry.h"
#include "io/binarystream.h"
#include "fs/bglreaderoptions.h"

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

Waypoint::Waypoint(const BglReaderOptions *options, BinaryStream *bs)
  : Record(options, bs)
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

  if(options->includeBglObject(type::ROUTE))
    for(int i = 0; i < numRoutes; i++)
      routes.push_back(RouteEntry(options, bs));
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
