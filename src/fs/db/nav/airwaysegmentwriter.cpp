/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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

#include "fs/db/nav/airwaysegmentwriter.h"
#include "fs/db/meta/bglfilewriter.h"
#include "fs/db/datawriter.h"
#include "fs/bgl/nav/airwaywaypoint.h"
#include "fs/bgl/util.h"
#include "fs/db/nav/waypointwriter.h"
#include "geo/calculations.h"
#include "atools.h"

namespace atools {
namespace fs {
namespace db {

using atools::fs::bgl::AirwaySegment;

void AirwaySegmentWriter::writeObject(const AirwaySegment *type)
{
  bind(":airway_point_id", getNextId());
  bindNullInt(":waypoint_id");
  bind(":name", type->getAirwayName());
  bind(":type", AirwaySegment::airwayTypeToStr(type->getAirwayType()));

  bind(":mid_ident", type->getMidWaypoint()->getIdent());
  bind(":mid_region", type->getMidWaypoint()->getRegion());
  bind(":mid_type", bgl::util::enumToStr(bgl::Waypoint::waypointTypeToStr,
                                         type->getMidWaypoint()->getType()));

  if(type->hasNextWaypoint())
  {
    using namespace atools::geo;
    using namespace atools;

    bind(":next_type",
         bgl::util::enumToStr(bgl::AirwayWaypoint::airwayWaypointTypeToStr,
                              type->getNextWaypoint().getType()));
    bind(":next_ident", type->getNextWaypoint().getIdent());
    bind(":next_region", type->getNextWaypoint().getRegion());
    bind(":next_airport_ident", type->getNextWaypoint().getAirportIdent());
    bind(":next_minimum_altitude",
         roundToPrecision(meterToFeet(type->getNextWaypoint().getMinimumAltitude()), 1));
  }
  else
  {
  bindNullString(":next_type");
  bindNullString(":next_ident");
  bindNullString(":next_region");
  bindNullString(":next_airport_ident");
  bindNullFloat(":next_minimum_altitude");
  }

  if(type->hasPreviousWaypoint())
  {
    using namespace atools::geo;

    bind(":previous_type",
         bgl::util::enumToStr(bgl::AirwayWaypoint::airwayWaypointTypeToStr,
                              type->getPreviousWaypoint().getType()));
    bind(":previous_ident", type->getPreviousWaypoint().getIdent());
    bind(":previous_region", type->getPreviousWaypoint().getRegion());
    bind(":previous_airport_ident", type->getPreviousWaypoint().getAirportIdent());
    bind(":previous_minimum_altitude",
         roundToPrecision(meterToFeet(type->getPreviousWaypoint().getMinimumAltitude()), 1));
  }
  else
  {
  bindNullString(":previous_type");
  bindNullString(":previous_ident");
  bindNullString(":previous_region");
  bindNullString(":previous_airport_ident");
  bindNullFloat(":previous_minimum_altitude");
  }

  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
