/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel albar965@mailbox.org
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
#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

AirwayWaypoint::AirwayWaypoint(const atools::fs::NavDatabaseOptions *options, atools::io::BinaryStream *bs)
  : BglBase(options, bs)
{
  unsigned int nextFlags = bs->readUInt();
  unsigned int nextIdFlags = bs->readUInt();
  minimumAltitude = bs->readFloat();

  type = static_cast<nav::AirwayWaypointType>(nextFlags & 0x5);
  ident = converter::intToIcao((nextFlags >> 5) & 0x7ffffff, true);
  region = converter::intToIcao(nextIdFlags & 0x7ff, true);
  airportIdent = converter::intToIcao((nextIdFlags >> 11) & 0xfffff, true);
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
