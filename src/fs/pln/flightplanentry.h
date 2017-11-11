/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

  /* Can use FSX or FS9 types */
  void setWaypointType(const QString& value);

  void setWaypointType(const atools::fs::pln::entry::WaypointType& value)
  {
    waypointType = value;
  }

  /*
   * @return ICAO ident of this waypoint
   */
  const QString& getWaypointId() const
  {
    return waypointId;
  }

  void setWaypointId(const QString& value)
  {
    waypointId = value;
  }

  /*
   * @return airway name if plan is an low alt or high alt flight plan
   */
  const QString& getAirway() const
  {
    return airway;
  }

  void setAirway(const QString& value)
  {
    airway = value;
  }

  /*
   * @return two letter ICAO region code
   */
  const QString& getIcaoRegion() const
  {
    return icaoRegion;
  }

  void setIcaoRegion(const QString& value)
  {
    icaoRegion = value;
  }

  /*
   * @return ICAO ident of this waypoint
   */
  const QString& getIcaoIdent() const
  {
    return icaoIdent;
  }

  void setIcaoIdent(const QString& value)
  {
    icaoIdent = value;
  }

  /*
   * @return coordinates of this waypoint
   */
  const geo::Pos& getPosition() const
  {
    return position;
  }

  void setPosition(const atools::geo::Pos& value)
  {
    position = value;
  }

  /* Do not save entry into the file */
  bool isNoSave() const
  {
    return noSave;
  }

  void setNoSave(bool value)
  {
    noSave = value;
  }

  bool operator==(const atools::fs::pln::FlightplanEntry& other);

  bool operator!=(const atools::fs::pln::FlightplanEntry& other)
  {
    return !operator==(other);
  }

  /* Name is not saved with PLN file */
  QString getName() const
  {
    return name;
  }

  void setName(const QString& value)
  {
    name = value;
  }

  /* Magnetic variance is not saved with PLN file */
  float getMagvar() const
  {
    return magvar;
  }

  void setMagvar(float value)
  {
    magvar = value;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::pln::FlightplanEntry& record);

  static const QString& waypointTypeToString(atools::fs::pln::entry::WaypointType type);
  static atools::fs::pln::entry::WaypointType stringToWaypointType(const QString& str);

  atools::fs::pln::entry::WaypointType waypointType = entry::UNKNOWN;
  QString waypointId, airway, icaoRegion, icaoIdent, name;
  atools::geo::Pos position;
  float magvar = 0.f;
  bool noSave = false;
};

} // namespace pln
} // namespace fs
} // namespace atools

#endif // ATOOLS_FLIGHTPLANENTRY_H
