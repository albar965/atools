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

#ifndef ATOOLS_FLIGHTPLANENTRY_H
#define ATOOLS_FLIGHTPLANENTRY_H

#include <QCoreApplication>

#include "fs/pln/flightplanconstants.h"
#include "geo/pos.h"

namespace atools {
namespace fs {
namespace pln {

/*
 * Waypoint or airport as part of the flight plan. Also covers departure and destination airports.
 */
class FlightplanEntry
{
  Q_DECLARE_TR_FUNCTIONS(FlightplanEntry)

public:
  /*
   * @return waypoint type as string like "VOR", "Waypoint" or "User"
   */
  const QString& getWaypointTypeAsFsxString() const
  {
    return waypointTypeToFsxString(waypointType);
  }

  /* As above but for LNM format */
  const QString& getWaypointTypeAsLnmString() const
  {
    return waypointTypeToLnmString(waypointType);
  }

  /* Translated for display */
  QString getWaypointTypeAsDisplayString() const
  {
    return waypointTypeToDisplayString(waypointType);
  }

  /* FS9 one character. First of getWaypointTypeAsString() */
  QString getWaypointTypeAsStringShort() const;

  atools::fs::pln::entry::WaypointType getWaypointType() const;

  /* Can use FSX or FS9 types */
  void setWaypointType(const QString& value);
  void setWaypointTypeFromLnm(const QString& value);

  void setWaypointType(const atools::fs::pln::entry::WaypointType& value)
  {
    waypointType = value;
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
  const QString& getRegion() const
  {
    return region;
  }

  void setRegion(const QString& value)
  {
    region = value;
  }

  /*
   * @return ICAO ident of this waypoint
   */
  const QString& getIdent() const
  {
    return ident;
  }

  void setIdent(const QString& value)
  {
    ident = value;
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

  /* sets lat and lon but not altitude */
  void setCoords(const atools::geo::Pos& value)
  {
    position.setLonX(value.getLonX());
    position.setLatY(value.getLatY());
  }

  /* sets altitude in ft but not lat and lon */
  void setAltitude(const atools::geo::Pos& value)
  {
    position.setAltitude(value.getAltitude());
  }

  /* Altitude [ft] which is part of the position is used by some export functions */
  void setAltitude(float value)
  {
    position.setAltitude(value);
  }

  /* Do not save entry into the file if it is a procedure or an alternate airport */
  bool isNoSave() const
  {
    return (flags& entry::PROCEDURE) || (flags & entry::ALTERNATE);
  }

  bool isTrack() const
  {
    return !airway.isEmpty() && flags & entry::TRACK;
  }

  bool isAirway() const
  {
    return !airway.isEmpty() && !(flags & entry::TRACK);
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

  /* NDB or VOR frequency, kHz * 100 or MHz * 1000 - not saved */
  int getFrequency() const
  {
    return frequency;
  }

  void setFrequency(int value)
  {
    frequency = value;
  }

  const atools::fs::pln::entry::Flags& getFlags() const
  {
    return flags;
  }

  void setFlags(const atools::fs::pln::entry::Flags& value)
  {
    flags = value;
  }

  void setFlag(atools::fs::pln::entry::Flag value, bool on = true)
  {
    flags.setFlag(value, on);
  }

  const QString& getComment() const
  {
    return comment;
  }

  void setComment(const QString& value)
  {
    comment = value;
  }

  /* All below are for MSFS and are set before export =========================== */
  const QString& getSid() const
  {
    return sid;
  }

  void setSid(const QString& value)
  {
    sid = value;
  }

  const QString& getStar() const
  {
    return star;
  }

  void setStar(const QString& value)
  {
    star = value;
  }

  const QString& getApproach() const
  {
    return approach;
  }

  const QString& getApproachSuffix() const
  {
    return approachSuffix;
  }

  void setApproach(const QString& approachType, const QString& suffix, const QString& transition)
  {
    approach = approachType;
    approachSuffix = suffix;
    approachTransition = transition;
  }

  const QString& getRunwayNumber() const
  {
    return runway;
  }

  void setRunway(const QString& runwayNumber, const QString& runwayDesignator)
  {
    runway = runwayNumber;
    designator = runwayDesignator;
  }

  /* One letter code like L, R, C and more */
  const QString& getRunwayDesignator() const
  {
    return designator;
  }

  const QString& getAirport() const
  {
    return airport;
  }

  void setAirport(const QString& value)
  {
    airport = value;
  }

  /* true if entry is valid and not default constructed */
  bool isValid() const
  {
    return position.isValid();
  }

  const QString& getStarTransition() const
  {
    return starTransition;
  }

  void setStarTransition(const QString& value)
  {
    starTransition = value;
  }

  const QString& getSidTransition() const
  {
    return sidTransition;
  }

  void setSidTransition(const QString& value)
  {
    sidTransition = value;
  }

  const QString& getApproachTransition() const
  {
    return approachTransition;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::pln::FlightplanEntry& record);

  static const QString& waypointTypeToLnmString(atools::fs::pln::entry::WaypointType type);
  static const QString& waypointTypeToFsxString(atools::fs::pln::entry::WaypointType type);
  static QString waypointTypeToDisplayString(atools::fs::pln::entry::WaypointType type);
  static atools::fs::pln::entry::WaypointType stringToWaypointType(const QString& str);
  static atools::fs::pln::entry::WaypointType stringToWaypointTypeLnm(const QString& str);
  static QString flagsAsString(atools::fs::pln::entry::Flags flags);

  atools::fs::pln::entry::WaypointType waypointType = entry::UNKNOWN;
  QString airway, region, ident, name, comment;

  /* MSFS fields - these are set as found in the ATCWaypoint element.
   * Needed here since the waypoints have to be removed after loading. */
  QString sid, sidTransition, star, starTransition,
          approach, approachSuffix, approachTransition, runway, designator, airport;

  atools::geo::Pos position;
  atools::fs::pln::entry::Flags flags = atools::fs::pln::entry::NONE;
  float magvar = 0.f;
  int frequency = 0;
};

} // namespace pln
} // namespace fs
} // namespace atools

#endif // ATOOLS_FLIGHTPLANENTRY_H
