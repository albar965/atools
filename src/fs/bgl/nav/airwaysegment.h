/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_BGL_NAV_AIRWAYENTRY_H
#define ATOOLS_BGL_NAV_AIRWAYENTRY_H

#include "fs/bgl/bglbase.h"
#include "fs/bgl/nav/airwaywaypoint.h"

namespace atools {
namespace fs {
namespace bgl {
class Waypoint;
namespace nav {

enum AirwayType
{
  NONE = 0,
  VICTOR = 1,
  JET = 2,
  BOTH = 3
};

} // namespace nav

/*
 * Airway segment that is attached to a waypoint and has next and previous waypoints. Subrecord of waypoint.
 * previous waypoint -> current waypoint (parent of segment) -> next waypoint
 */
class AirwaySegment :
  public atools::fs::bgl::BglBase
{
public:
  AirwaySegment(const NavDatabaseOptions *options, atools::io::BinaryStream *bs,
                const atools::fs::bgl::Waypoint& waypoint);
  virtual ~AirwaySegment();

  AirwaySegment(const atools::fs::bgl::AirwaySegment& other);
  AirwaySegment& operator=(const atools::fs::bgl::AirwaySegment& other);

  bool hasNextWaypoint() const;

  bool hasPreviousWaypoint() const;

  /*
   * @return next waypoint on the airway
   */
  const atools::fs::bgl::AirwayWaypoint& getNextWaypoint() const
  {
    return next;
  }

  /*
   * @return previous waypoint on the airway
   */
  const atools::fs::bgl::AirwayWaypoint& getPreviousWaypoint() const
  {
    return previous;
  }

  const QString& getAirwayName() const
  {
    return name;
  }

  atools::fs::bgl::nav::AirwayType getAirwayType() const
  {
    return type;
  }

  static QString airwayTypeToStr(atools::fs::bgl::nav::AirwayType type);

  const atools::fs::bgl::Waypoint *getMidWaypoint() const
  {
    return mid;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::AirwaySegment& record);

  atools::fs::bgl::nav::AirwayType type;
  QString name;

  atools::fs::bgl::Waypoint *mid = nullptr;
  atools::fs::bgl::AirwayWaypoint next;
  atools::fs::bgl::AirwayWaypoint previous;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_NAV_AIRWAYENTRY_H
