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

#ifndef ATOOLS_FLIGHTPLANENTRY_H
#define ATOOLS_FLIGHTPLANENTRY_H

#include <QString>

#include "geo/pos.h"

namespace atools {
namespace fs {
namespace pln {

namespace entry {
enum WaypointType
{
  UNKNOWN,
  AIRPORT,
  INTERSECTION,
  VOR,
  NDB,
  USER
};

}

/*
 * Waypoint or airport as part of the flight plan. Also covers departure and destination airports.
 */
class FlightplanEntry
{
public:
  FlightplanEntry();
  FlightplanEntry(const atools::fs::pln::FlightplanEntry& other);
  ~FlightplanEntry();

  FlightplanEntry& operator=(const atools::fs::pln::FlightplanEntry& other);

  /*
   * @return waypoint type as string like "VOR", "Waypoint" or "User"
   */
  const QString& getWaypointTypeAsString() const;

  atools::fs::pln::entry::WaypointType getWaypointType() const;

  void setWaypointType(const QString& value);
  void setWaypointType(const atools::fs::pln::entry::WaypointType& value);

  /*
   * @return ICAO ident of this waypoint
   */
  const QString& getWaypointId() const;
  void setWaypointId(const QString& value);

  /*
   * @return airway name if plan is an low alt or high alt flight plan
   */
  const QString& getAirway() const;
  void setAirway(const QString& value);

  /*
   * @return two letter ICAO region code
   */
  const QString& getIcaoRegion() const;
  void setIcaoRegion(const QString& value);

  /*
   * @return ICAO ident of this waypoint
   */
  const QString& getIcaoIdent() const;
  void setIcaoIdent(const QString& value);

  /*
   * @return coordinates of this waypoint
   */
  const geo::Pos& getPosition() const;
  void setPosition(const atools::geo::Pos& value);

private:
  static const QString& waypointTypeToString(atools::fs::pln::entry::WaypointType type);
  static atools::fs::pln::entry::WaypointType stringToWaypointType(const QString& str);

  atools::fs::pln::entry::WaypointType waypointType;
  QString waypointId, airway, icaoRegion, icaoIdent, name, parking;
  atools::geo::Pos position;
};

} // namespace pln
} // namespace fs
} // namespace atools

#endif // ATOOLS_FLIGHTPLANENTRY_H
