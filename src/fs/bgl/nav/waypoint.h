/*
 * Waypoint.h
 *
 *  Created on: 25.04.2015
 *      Author: alex
 */

#ifndef BGL_NAV_WAYPOINT_H_
#define BGL_NAV_WAYPOINT_H_

#include "fs/bgl/record.h"
#include "fs/bgl/bglposition.h"
#include "fs/bgl/nav/routeentry.h"

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
  public atools::fs::bgl::Record
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

  const atools::fs::bgl::BglPosition& getPosition() const
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

  const QList<atools::fs::bgl::RouteEntry>& getRoutes() const
  {
    return routes;
  }

  static QString waypointTypeToStr(atools::fs::bgl::nav::WaypointType type);

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Waypoint& record);

  atools::fs::bgl::nav::WaypointType type;
  atools::fs::bgl::BglPosition position;
  float magVar;
  QString ident, region, airportIdent;

  QList<atools::fs::bgl::RouteEntry> routes;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_NAV_WAYPOINT_H_ */
