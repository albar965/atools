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

#include "fs/pln/flightplanentry.h"

namespace atools {
namespace fs {
namespace pln {

using atools::sql::SqlDatabase;

FlightplanEntry::FlightplanEntry(SqlDatabase *sqlDb)
{

}

FlightplanEntry::~FlightplanEntry()
{

}

void FlightplanEntry::loadFromWaypoint(int waypointId)
{

}

void FlightplanEntry::loadFromVor(int vorId)
{

}

void FlightplanEntry::loadFromNdb(int ndbId)
{

}

void FlightplanEntry::loadFromCoordinates(const geo::Pos& pos)
{

}

QString FlightplanEntry::getWaypointTypeAsString() const
{
  return waypointTypeToString(waypointType);
}

atools::fs::pln::entry::WaypointType FlightplanEntry::getWaypointType() const
{
  return waypointType;
}

void FlightplanEntry::setWaypointType(const QString& value)
{
  waypointType = stringToWaypointType(value);
}

void FlightplanEntry::setWaypointType(const atools::fs::pln::entry::WaypointType& value)
{
  waypointType = value;
}

QString FlightplanEntry::getWaypointId() const
{
  return waypointId;
}

QString FlightplanEntry::getAirway() const
{
  return airway;
}

void FlightplanEntry::setAirway(const QString& value)
{
  airway = value;
}

QString FlightplanEntry::getIcaoRegion() const
{
  return icaoRegion;
}

QString FlightplanEntry::getIcaoIdent() const
{
  return icaoIdent;
}

atools::geo::Pos FlightplanEntry::getPosition() const
{
  return position;
}

void FlightplanEntry::setPosition(const atools::geo::Pos& value)
{
  position = value;
}

QString FlightplanEntry::waypointTypeToString(entry::WaypointType type)
{
  switch(type)
  {
    case atools::fs::pln::entry::AIRPORT:
      return "Airport";

    case atools::fs::pln::entry::UNKNOWN:
      return "Unknown";

    case atools::fs::pln::entry::INTERSECTION:
      return "Intersection";

    case atools::fs::pln::entry::VOR:
      return "VOR";

    case atools::fs::pln::entry::NDB:
      return "NDB";

    case atools::fs::pln::entry::USER:
      return "User";

  }
  return QString();
}

entry::WaypointType FlightplanEntry::stringToWaypointType(const QString& str)
{
  if(str == "Airport")
    return entry::AIRPORT;
  else if(str == "Intersection")
    return entry::INTERSECTION;
  else if(str == "VOR")
    return entry::VOR;
  else if(str == "NDB")
    return entry::NDB;
  else if(str == "User")
    return entry::USER;

  return entry::UNKNOWN;
}

void FlightplanEntry::setIcaoRegion(const QString& value)
{
  icaoRegion = value;
}

void FlightplanEntry::setIcaoIdent(const QString& value)
{
  icaoIdent = value;
}

void FlightplanEntry::setWaypointId(const QString& value)
{
  waypointId = value;
}

/*
 *  <ATCWaypoint id="UTH">
 *     <ATCWaypointType>NDB</ATCWaypointType>
 *     <WorldPosition>N63° 43' 22.04",E9° 34' 40.01",+000000.00</WorldPosition>
 *     <ATCAirway>L996</ATCAirway>
 *     <ICAO>
 *         <ICAORegion>EN</ICAORegion>
 *         <ICAOIdent>UTH</ICAOIdent>
 *     </ICAO>
 *  </ATCWaypoint>
 *  <ATCWaypoint id="AKABI">
 *      <ATCWaypointType>Intersection</ATCWaypointType>
 *      <WorldPosition>N47° 43' 1.01",E9° 14' 0.00",+000000.00</WorldPosition>
 *      <ATCAirway>UZ613</ATCAirway>
 *      <ICAO>
 *          <ICAORegion>LS</ICAORegion>
 *          <ICAOIdent>AKABI</ICAOIdent>
 *      </ICAO>
 *  </ATCWaypoint>
 *  <ATCWaypoint id="KPT">
 *      <ATCWaypointType>VOR</ATCWaypointType>
 *      <WorldPosition>N47° 44' 44.84",E10° 20' 59.38",+000000.00</WorldPosition>
 *      <ICAO>
 *          <ICAORegion>ED</ICAORegion>
 *          <ICAOIdent>KPT</ICAOIdent>
 *      </ICAO>
 *  </ATCWaypoint>
 */

} // namespace pln
} // namespace fs
} // namespace atools
