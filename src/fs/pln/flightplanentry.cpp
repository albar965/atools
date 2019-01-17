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

#include "fs/pln/flightplanentry.h"

namespace atools {
namespace fs {
namespace pln {

FlightplanEntry::FlightplanEntry()
{
}

FlightplanEntry::~FlightplanEntry()
{

}

FlightplanEntry::FlightplanEntry(const FlightplanEntry& other)
{
  this->operator=(other);

}

FlightplanEntry& FlightplanEntry::operator=(const FlightplanEntry& other)
{
  waypointType = other.waypointType;
  waypointId = other.waypointId;
  airway = other.airway;
  icaoRegion = other.icaoRegion;
  icaoIdent = other.icaoIdent;
  name = other.name;
  magvar = other.magvar;
  position = other.position;
  noSave = other.noSave;
  frequency = other.frequency;
  return *this;
}

const QString& FlightplanEntry::getWaypointTypeAsString() const
{
  return waypointTypeToString(waypointType);
}

QString FlightplanEntry::getWaypointTypeAsStringShort() const
{
  const QString type = waypointTypeToString(waypointType);
  return type.isEmpty() ? type : type.at(0);
}

atools::fs::pln::entry::WaypointType FlightplanEntry::getWaypointType() const
{
  return waypointType;
}

void FlightplanEntry::setWaypointType(const QString& value)
{
  waypointType = stringToWaypointType(value);
}

bool FlightplanEntry::operator==(const FlightplanEntry& other)
{
  return waypointType == other.waypointType &&
         waypointId == other.waypointId &&
         icaoRegion == other.icaoRegion &&
         icaoIdent == other.icaoIdent &&
         name == other.name;
}

const QString& FlightplanEntry::waypointTypeToString(entry::WaypointType type)
{
  static const QString airportName("Airport"), unknownName("Unknown"), isecName("Intersection"),
  vorName("VOR"), ndbName("NDB"), userName("User"), emptyName;

  switch(type)
  {
    case atools::fs::pln::entry::AIRPORT:
      return airportName;

    case atools::fs::pln::entry::UNKNOWN:
      return unknownName;

    case atools::fs::pln::entry::INTERSECTION:
      return isecName;

    case atools::fs::pln::entry::VOR:
      return vorName;

    case atools::fs::pln::entry::NDB:
      return ndbName;

    case atools::fs::pln::entry::USER:
      return userName;

  }
  return emptyName;
}

entry::WaypointType FlightplanEntry::stringToWaypointType(const QString& str)
{
  if(str.startsWith("A"))
    return entry::AIRPORT;
  else if(str.startsWith("I"))
    return entry::INTERSECTION;
  else if(str.startsWith("V"))
    return entry::VOR;
  else if(str.startsWith("N"))
    return entry::NDB;
  else if(str.startsWith("U"))
    return entry::USER;

  return entry::UNKNOWN;
}

QDebug operator<<(QDebug out, const FlightplanEntry& record)
{
  QDebugStateSaver saver(out);

  out.noquote().nospace() << "FlightplanEntry[id " << record.getWaypointId()
                          << ", type " << record.getWaypointTypeAsString()
                          << ", ident " << record.getIcaoIdent()
                          << ", region " << record.getIcaoRegion()
                          << ", airway " << record.getAirway()
                          << ", pos " << record.getPosition()
                          << ", save " << !record.isNoSave() << "]" << endl;
  return out;
}

} // namespace pln
} // namespace fs
} // namespace atools
