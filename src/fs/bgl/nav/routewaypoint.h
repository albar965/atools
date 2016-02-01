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

#ifndef BGL_NAV_ROUTEWAYPOINT_H_
#define BGL_NAV_ROUTEWAYPOINT_H_

#include "fs/bgl/bglbase.h"

namespace atools {
namespace fs {
namespace bgl {

namespace nav {

enum RouteWaypointType
{
  ROUTE_WP_NONE = 0,
  ROUTE_WP_NDB = 1, // wiki error reported
  ROUTE_WP_VOR = 2,
  ROUTE_WP_OTHER = 5
};

} // namespace nav

class RouteWaypoint :
  public atools::fs::bgl::BglBase
{
public:
  RouteWaypoint(const BglReaderOptions *options, atools::io::BinaryStream *bs);
  virtual ~RouteWaypoint();

  RouteWaypoint()
    : type(atools::fs::bgl::nav::ROUTE_WP_NONE), minimumAltitude(0.f)
  {
  }

  const QString& getAirportIdent() const
  {
    return airportIdent;
  }

  const QString& getIdent() const
  {
    return ident;
  }

  float getMinimumAltitude() const
  {
    return minimumAltitude;
  }

  const QString& getRegion() const
  {
    return region;
  }

  atools::fs::bgl::nav::RouteWaypointType getType() const
  {
    return type;
  }

  static QString routeWaypointTypeToStr(atools::fs::bgl::nav::RouteWaypointType type);

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::RouteWaypoint& record);

  atools::fs::bgl::nav::RouteWaypointType type;
  QString ident, region, airportIdent;
  float minimumAltitude;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_NAV_ROUTEWAYPOINT_H_ */
