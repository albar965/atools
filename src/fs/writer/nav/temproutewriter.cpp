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

#include "fs/writer/nav/temproutewriter.h"
#include "fs/writer/meta/bglfilewriter.h"
#include "fs/writer/datawriter.h"
#include "fs/bgl/nav/routewaypoint.h"
#include "fs/bgl/util.h"
#include "fs/writer/nav/waypointwriter.h"

namespace atools {
namespace fs {
namespace writer {

using atools::fs::bgl::RouteEntry;
using atools::sql::SqlQuery;

void TempRouteWriter::writeObject(const RouteEntry *type)
{
  bind(":temp_route_id", getNextId());
  bind(":waypoint_id", getDataWriter().getWaypointWriter()->getCurrentId());
  bind(":name", type->getName());
  bind(":type", RouteEntry::routeTypeToStr(type->getType()));

  if(type->hasNextWaypoint())
  {
    bind(":next_type",
         bgl::util::enumToStr(bgl::RouteWaypoint::routeWaypointTypeToStr,
                              type->getNextWaypoint().getType()));
    bind(":next_ident", type->getNextWaypoint().getIdent());
    bind(":next_region", type->getNextWaypoint().getRegion());
    bind(":next_airport_ident", type->getNextWaypoint().getAirportIdent());
    bind(":next_minimum_altitude", bgl::util::meterToFeet(type->getNextWaypoint().getMinimumAltitude(), 1));
  }

  if(type->hasPreviousWaypoint())
  {
    bind(":previous_type",
         bgl::util::enumToStr(bgl::RouteWaypoint::routeWaypointTypeToStr,
                              type->getPreviousWaypoint().getType()));
    bind(":previous_ident", type->getPreviousWaypoint().getIdent());
    bind(":previous_region", type->getPreviousWaypoint().getRegion());
    bind(":previous_airport_ident", type->getPreviousWaypoint().getAirportIdent());
    bind(":previous_minimum_altitude",
         bgl::util::meterToFeet(type->getPreviousWaypoint().getMinimumAltitude(), 1));
  }

  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
