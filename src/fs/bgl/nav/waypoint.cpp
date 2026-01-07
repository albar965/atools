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

#include "fs/bgl/nav/waypoint.h"
#include "fs/bgl/converter.h"
#include "fs/bgl/nav/airwaysegment.h"
#include "fs/bgl/recordtypes.h"
#include "io/binarystream.h"
#include "fs/navdatabaseoptions.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

QString Waypoint::waypointTypeToStr(nav::WaypointType type)
{
  switch(type)
  {
    case nav::WP_NONE:
      return "WP_NONE";

    case nav::RNAV:
      return "RNAV";

    case nav::VFR:
      return "VFR";

    case nav::NAMED:
      return "WN";

    case nav::UNNAMED:
      return "WU";

    case nav::VOR:
      return "V";

    case nav::NDB:
      return "N";

    case nav::OFF_AIRWAY:
      return "OA";

    case nav::IAF:
      return "IAF";

    case nav::FAF:
      return "FAF";
  }
  qWarning().nospace().noquote() << "Invalid waypoint type " << type;
  return "INVALID";
}

nav::AirwayWaypointType Waypoint::airwayWaypointTypeFromWaypointType(nav::WaypointType type)
{
  switch(type)
  {
    case nav::WP_NONE:
    case nav::NAMED:
    case nav::UNNAMED:
    case nav::OFF_AIRWAY:
    case nav::IAF:
    case nav::FAF:
    case nav::RNAV:
    case nav::VFR:
      // Airway waypoint has a limited number of types
      return nav::AIRWAY_WP_OTHER;
      break;

    case nav::VOR:
      return nav::AIRWAY_WP_VOR;
      break;

    case nav::NDB:
      return nav::AIRWAY_WP_NDB;
      break;
  }
  return nav::AIRWAY_WP_NONE;
}

bool Waypoint::isValid() const
{
  if(ident == "NPOLE")
    // Only waypoint allowed on the poles
    return position.getPos().isValid() && !position.getPos().isNull();
  else
    return position.getPos().isValid() && !position.getPos().isPole() && !position.getPos().isNull();
}

bool atools::fs::bgl::Waypoint::isDisabled() const
{
  return position.getPos().isPole();
}

QString Waypoint::getObjectName() const
{
  return Record::getObjectName() + QStringLiteral("waypoint ident %1 region %2 position %3").
         arg(ident).arg(region).arg(position.getPos().toString());
}

Waypoint::Waypoint(const NavDatabaseOptions *options, BinaryStream *stream)
  : Record(options, stream)
{
  type = static_cast<nav::WaypointType>(stream->readUByte());
  int numAirways = stream->readUByte();
  position = BglPosition(stream);
  magVar = converter::adjustMagvar(stream->readFloat());
  ident = id == rec::WAYPOINT_MSFS2024 ? converter::intToIcaoLong(stream->readULong()) : converter::intToIcao(stream->readUInt());

  unsigned int regionFlags = stream->readUInt();
  region = converter::intToIcao(regionFlags & 0x7ff, true);
  airportIdent = converter::intToIcao((regionFlags >> 11) & 0x1fffff, true);

  if(region.isEmpty() && !isDisabled())
    qWarning().nospace().noquote() << "Waypoint at " << position << " ident " << ident << " has no region";

  if(ident.isEmpty() && type != nav::UNNAMED && !isDisabled() && options->getSimulatorType() != atools::fs::FsPaths::MSFS)
    qWarning().nospace().noquote() << "Waypoint at " << position << " region " << region << " has no ident";

  if(id == rec::WAYPOINT_MSFS2024)
    stream->skip(4); // Skip unknown data

  // Read airways
  for(int i = 0; i < numAirways; i++)
  {
    // Read always to avoid messing up current file position
    AirwaySegment segment(options, stream, *this);
    if(options->isIncludedNavDbObject(type::AIRWAY))
    {
      airways.append(segment);

      if(segment.getAirwayType() == atools::fs::bgl::nav::BOTH)
      {
        numVictorAirway++;
        numJetAirway++;
      }
      else if(segment.getAirwayType() == atools::fs::bgl::nav::VICTOR)
        numVictorAirway++;
      else if(segment.getAirwayType() == atools::fs::bgl::nav::JET)
        numJetAirway++;
    }
  }
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
                          << ", airport ID " << record.airportIdent << Qt::endl;
  out << record.airways;
  out << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
