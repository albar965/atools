/*
 * RouteEntry.h
 *
 *  Created on: 10.05.2015
 *      Author: alex
 */

#ifndef BGL_NAV_ROUTEWAYPOINT_H_
#define BGL_NAV_ROUTEWAYPOINT_H_

#include "../bglbase.h"

namespace atools {
namespace fs {
namespace bgl {

namespace nav {

enum RouteWaypointType
{
  ROUTE_WP_NONE = 0,
  ROUTE_WP_NDB = 1, // TODO error in wiki
  ROUTE_WP_VOR = 2,
  ROUTE_WP_OTHER = 5
};

} // namespace nav

class RouteWaypoint :
  public BglBase
{
public:
  RouteWaypoint(atools::io::BinaryStream *bs);

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
  friend QDebug operator<<(QDebug out, const RouteWaypoint& record);

  atools::fs::bgl::nav::RouteWaypointType type;
  QString ident, region, airportIdent;
  float minimumAltitude;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_NAV_ROUTEWAYPOINT_H_ */
