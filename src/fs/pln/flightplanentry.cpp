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

#include "fs/pln/flightplanentry.h"

namespace atools {
namespace fs {
namespace pln {

QString FlightplanEntry::getWaypointTypeAsStringShort() const
{
  const QString type = waypointTypeToFsxString(waypointType);
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

void FlightplanEntry::setWaypointTypeFromLnm(const QString& value)
{
  waypointType = stringToWaypointTypeLnm(value);
}

bool FlightplanEntry::operator==(const FlightplanEntry& other)
{
  return waypointType == other.waypointType &&
         region == other.region &&
         ident == other.ident &&
         name == other.name;
}

const QString& FlightplanEntry::waypointTypeToFsxString(entry::WaypointType type)
{
  static const QString airportName("Airport"), unknownName("Unknown"), isecName("Intersection"),
  vorName("VOR"), ndbName("NDB"), userName("User"), emptyName;

  switch(type)
  {
    case atools::fs::pln::entry::AIRPORT:
      return airportName;

    case atools::fs::pln::entry::UNKNOWN:
      return unknownName;

    case atools::fs::pln::entry::WAYPOINT:
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

QString FlightplanEntry::waypointTypeToDisplayString(entry::WaypointType type)
{
  switch(type)
  {
    case atools::fs::pln::entry::AIRPORT:
      return tr("Airport");

    case atools::fs::pln::entry::UNKNOWN:
      return tr("Unknown");

    case atools::fs::pln::entry::WAYPOINT:
      return tr("Waypoint");

    case atools::fs::pln::entry::VOR:
      return tr("VOR");

    case atools::fs::pln::entry::NDB:
      return tr("NDB");

    case atools::fs::pln::entry::USER:
      return tr("User");

  }
  return tr("Unknown");
}

entry::WaypointType FlightplanEntry::stringToWaypointType(const QString& str)
{
  if(str.startsWith("A"))
    return entry::AIRPORT;
  else if(str.startsWith("I"))
    return entry::WAYPOINT;
  else if(str.startsWith("V"))
    return entry::VOR;
  else if(str.startsWith("N"))
    return entry::NDB;
  else if(str.startsWith("U"))
    return entry::USER;

  return entry::UNKNOWN;
}

const QString& FlightplanEntry::waypointTypeToLnmString(entry::WaypointType type)
{
  static const QString airportName("AIRPORT"), unknownName("UNKNOWN"), isecName("WAYPOINT"),
  vorName("VOR"), ndbName("NDB"), userName("USER"), emptyName;

  switch(type)
  {
    case atools::fs::pln::entry::AIRPORT:
      return airportName;

    case atools::fs::pln::entry::UNKNOWN:
      return unknownName;

    case atools::fs::pln::entry::WAYPOINT:
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

entry::WaypointType FlightplanEntry::stringToWaypointTypeLnm(const QString& str)
{
  if(str.startsWith("A"))
    return entry::AIRPORT;
  else if(str.startsWith("W"))
    return entry::WAYPOINT;
  else if(str.startsWith("V"))
    return entry::VOR;
  else if(str.startsWith("N"))
    return entry::NDB;
  else if(str.startsWith("U"))
    return entry::USER;

  return entry::UNKNOWN;
}

QString FlightplanEntry::flagsAsString(atools::fs::pln::entry::Flags flags)
{
  QStringList retval;

  if(flags == entry::NONE)
    return "NONE";

  if(flags & entry::PROCEDURE)
    retval.append("PROCEDURE");
  if(flags & entry::ALTERNATE)
    retval.append("ALTERNATE");
  if(flags & entry::TRACK)
    retval.append("TRACK");
  return retval.join(",");
}

QDebug operator<<(QDebug out, const FlightplanEntry& record)
{
  QDebugStateSaver saver(out);

  out.noquote().nospace() << "FlightplanEntry[type " << record.getWaypointTypeAsFsxString()
                          << ", ident " << record.getIdent()
                          << ", region " << record.getRegion()
                          << ", airway " << record.getAirway()
                          << ", comment " << record.getComment()
                          << ", pos " << record.getPosition()
                          << ", flags " << FlightplanEntry::flagsAsString(record.getFlags())
                          << ", alternate " << !record.isAlternate()
                          << ", procedure " << !record.isProcedure()
                          << "]";
  return out;
}

} // namespace pln
} // namespace fs
} // namespace atools
