/*
 * Waypoint.h
 *
 *  Created on: 25.04.2015
 *      Author: alex
 */

#ifndef BGL_NAV_WAYPOINT_H_
#define BGL_NAV_WAYPOINT_H_

#include "../record.h"
#include "../bglposition.h"
#include "routeentry.h"

namespace atools {
namespace fs {
namespace bgl {

namespace nav {

enum WaypointType
{
  NAMED = 1,
  UNNAMED = 2,
  VOR = 3,
  NDB = 4,
  OFF_ROUTE = 5,
  IAF = 6,
  FAF = 7
};

} // namespace nav

class Waypoint :
  public Record
{
public:
  Waypoint(atools::io::BinaryStream *bs);
  virtual ~Waypoint();

  const QString& getAirportIdent() const
  {
    return airportIdent;
  }

  const QString& getIdent() const
  {
    return ident;
  }

  float getMagVar() const
  {
    return magVar;
  }

  const BglPosition& getPosition() const
  {
    return position;
  }

  const QString& getRegion() const
  {
    return region;
  }

  atools::fs::bgl::nav::WaypointType getType() const
  {
    return type;
  }

  const QList<RouteEntry>& getRoutes() const
  {
    return routes;
  }

  static QString waypointTypeToStr(atools::fs::bgl::nav::WaypointType type);

private:
  friend QDebug operator<<(QDebug out, const Waypoint& record);

  atools::fs::bgl::nav::WaypointType type;
  BglPosition position;
  float magVar;
  QString ident, region, airportIdent;

  QList<RouteEntry> routes;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_NAV_WAYPOINT_H_ */
