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
  RouteEntry(const BglReaderOptions *options, atools::io::BinaryStream *bs);
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
