/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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

#include "fs/bgl/nav/airwaywaypoint.h"

#include "fs/bgl/converter.h"
#include "fs/bgl/nav/waypoint.h"
#include "fs/bgl/recordtypes.h"
#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

AirwayWaypoint::AirwayWaypoint(const Waypoint& waypoint)
{
  ident = waypoint.getIdent();
  region = waypoint.getRegion();
  airportIdent = waypoint.getAirportIdent();
  minimumAltitude = 0.f;
  type = Waypoint::airwayWaypointTypeFromWaypointType(waypoint.getType());
}

AirwayWaypoint::AirwayWaypoint(const NavDatabaseOptions *options, atools::io::BinaryStream *stream, const Waypoint& waypoint)
  : BglBase(options, stream)
{
  if(waypoint.getId() == rec::WAYPOINT_MSFS2024)
  {
    ident = converter::intToIcaoLong(stream->readULong());
    region = converter::intToIcao(stream->readUInt() & 0x7ff, true);

    stream->skip(4); // float of maximum altitude? Alyway 0
    minimumAltitude = stream->readFloat();

    // Type not given in MSFS 2024
    type = nav::AIRWAY_WP_OTHER;
  }
  else
  {
    // FSX, P3D and MSFS 2020
    unsigned int nextFlags = stream->readUInt();
    type = static_cast<nav::AirwayWaypointType>(nextFlags & 0x7);
    ident = converter::intToIcao((nextFlags >> 5) & 0x7ffffff, true);

    unsigned int nextIdFlags = stream->readUInt();
    region = converter::intToIcao(nextIdFlags & 0x7ff, true);
    airportIdent = converter::intToIcao((nextIdFlags >> 11) & 0xfffff, true);

    minimumAltitude = stream->readFloat();
  }
}

AirwayWaypoint::~AirwayWaypoint()
{
}

QString AirwayWaypoint::airwayWaypointTypeToStr(nav::AirwayWaypointType type)
{
  switch(type)
  {
    case nav::AIRWAY_WP_NONE:
      return "NONE";

    case nav::AIRWAY_WP_NDB:
      return "N";

    case nav::AIRWAY_WP_VOR:
      return "V";

    case nav::AIRWAY_WP_OTHER:
      return "O";
  }
  qWarning().nospace().noquote() << "Invalid airway waypoint type " << type;
  return "INVALID";
}

QDebug operator<<(QDebug out, const AirwayWaypoint& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const BglBase&>(record)
                          << " AirwayWaypoint[type " << AirwayWaypoint::airwayWaypointTypeToStr(record.type)
                          << ", ident " << record.ident
                          << ", region " << record.region
                          << ", min alt " << record.minimumAltitude
                          << ", airport ID " << record.airportIdent << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
