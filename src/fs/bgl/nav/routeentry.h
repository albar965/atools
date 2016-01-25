/*
 * RouteEntry.h
 *
 *  Created on: 10.05.2015
 *      Author: alex
 */

#ifndef BGL_NAV_ROUTEENTRY_H_
#define BGL_NAV_ROUTEENTRY_H_

#include "fs/bgl/bglbase.h"
#include "fs/bgl/nav/routewaypoint.h"

namespace atools {
namespace fs {
namespace bgl {

namespace nav {
enum RouteType
{
  NONE = 0,
  VICTOR = 1,
  JET = 2,
  BOTH = 3
};

} // namespace nav

class RouteEntry :
  public atools::fs::bgl::BglBase
{
public:
  RouteEntry(atools::io::BinaryStream *bs);
  virtual ~RouteEntry();

  bool hasNextWaypoint() const;

  bool hasPreviousWaypoint() const;

  const atools::fs::bgl::RouteWaypoint& getNextWaypoint() const
  {
    return next;
  }

  const atools::fs::bgl::RouteWaypoint& getPreviousWaypoint() const
  {
    return previous;
  }

  const QString& getName() const
  {
    return name;
  }

  atools::fs::bgl::nav::RouteType getType() const
  {
    return type;
  }

  static QString routeTypeToStr(atools::fs::bgl::nav::RouteType type);

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::RouteEntry& record);

  atools::fs::bgl::nav::RouteType type;
  QString name;

  atools::fs::bgl::RouteWaypoint next;
  atools::fs::bgl::RouteWaypoint previous;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_NAV_ROUTEENTRY_H_ */
