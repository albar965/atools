/*
 * TempRouteWriterWriter.cpp
 *
 *  Created on: 01.05.2015
 *      Author: alex
 */

#include "temproutewriter.h"
#include "../meta/bglfilewriter.h"
#include "../datawriter.h"
#include "../../bgl/nav/routewaypoint.h"
#include "../../bgl/util.h"
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
