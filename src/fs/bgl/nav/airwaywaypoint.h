/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_BGL_NAV_AIRWAYWAYPOINT_H
#define ATOOLS_BGL_NAV_AIRWAYWAYPOINT_H

#include "fs/bgl/bglbase.h"

#include <QString>

namespace atools {
namespace fs {
namespace bgl {

class Waypoint;

namespace nav {

enum AirwayWaypointType
{
  AIRWAY_WP_NONE = 0,
  AIRWAY_WP_VOR = 2,
  AIRWAY_WP_NDB = 3,
  AIRWAY_WP_OTHER = 5
};

} // namespace nav

/*
 * A waypoint on an airway that is used to reference to the previous and next point along an airway.
 * Ident and region are used to identify a waypoint since the references cross BGL file
 * boundaries.
 */
class AirwayWaypoint :
  public atools::fs::bgl::BglBase
{
public:
  AirwayWaypoint(const Waypoint& waypoint);
  AirwayWaypoint(const NavDatabaseOptions *options, atools::io::BinaryStream *stream, const atools::fs::bgl::Waypoint& waypoint);
  virtual ~AirwayWaypoint() override;

  AirwayWaypoint()
    : type(atools::fs::bgl::nav::AIRWAY_WP_NONE), minimumAltitude(0.f)
  {
  }

  /*
   * @return airport ICAO ident for the waypoint if applicable
   */
  const QString& getAirportIdent() const
  {
    return airportIdent;
  }

  /*
   * @return Waypoint ICAO ident
   */
  const QString& getIdent() const
  {
    return ident;
  }

  /*
   * @return Miniumum altitude for the segment in meter
   */
  float getMinimumAltitude() const
  {
    return minimumAltitude;
  }

  /*
   * @return ICAO two letter region code for the waypoint
   */
  const QString& getRegion() const
  {
    return region;
  }

  atools::fs::bgl::nav::AirwayWaypointType getType() const
  {
    return type;
  }

  static QString airwayWaypointTypeToStr(atools::fs::bgl::nav::AirwayWaypointType type);

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::AirwayWaypoint& record);

  atools::fs::bgl::nav::AirwayWaypointType type;
  QString ident, region, airportIdent;
  float minimumAltitude;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_NAV_AIRWAYWAYPOINT_H
