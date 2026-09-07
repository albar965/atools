/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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

#include "atools.h"
#include "fs/bgl/nav/airwaywaypoint.h"
#include "fs/bgl/util.h"
#include "fs/db/datawriter.h"
#include "geo/calculations.h"

namespace atools {
namespace fs {
namespace db {

using atools::fs::bgl::AirwaySegment;

void AirwaySegmentWriter::writeObject(const AirwaySegment *type)
{
  const atools::fs::bgl::AirwayWaypoint& previousWaypoint = type->getPreviousWaypoint();
  const atools::fs::bgl::AirwayWaypoint& midWaypoint = type->getMidWaypoint();
  const atools::fs::bgl::AirwayWaypoint& nextWaypoint = type->getNextWaypoint();

  if(type->getAirwayName().isEmpty())
  {
    qWarning() << Q_FUNC_INFO << "Airway has no name" << getCurrentFilepath()
               << previousWaypoint.getIdent() << "/" << previousWaypoint.getRegion() << "->"
               << midWaypoint.getIdent() << "/" << midWaypoint.getRegion() << "->"
               << nextWaypoint.getIdent() << "/" << nextWaypoint.getRegion();
    return;
  }

  bind(QStringLiteral(":airway_point_id"), getNextId());
  bind(QStringLiteral(":name"), type->getAirwayName());
  bind(QStringLiteral(":type"), AirwaySegment::airwayTypeToStr(type->getAirwayType()));

  bind(QStringLiteral(":mid_ident"), midWaypoint.getIdent());
  bind(QStringLiteral(":mid_region"), midWaypoint.getRegion());
  bind(QStringLiteral(":mid_type"), bgl::util::enumToStr(bgl::AirwayWaypoint::airwayWaypointTypeToStr, midWaypoint.getType()));

  if(type->hasNextWaypoint())
  {
    using namespace atools::geo;
    using namespace atools;

    bind(QStringLiteral(":next_type"),
         bgl::util::enumToStr(bgl::AirwayWaypoint::airwayWaypointTypeToStr, nextWaypoint.getType()));
    bind(QStringLiteral(":next_ident"), nextWaypoint.getIdent());
    bind(QStringLiteral(":next_region"), nextWaypoint.getRegion());
    bind(QStringLiteral(":next_airport_ident"), nextWaypoint.getAirportIdent());
    bind(QStringLiteral(":next_minimum_altitude"), roundToPrecision(meterToFeet(nextWaypoint.getMinimumAltitude()), 1));
  }
  else
  {
    bindNullString(QStringLiteral(":next_type"));
    bindNullString(QStringLiteral(":next_ident"));
    bindNullString(QStringLiteral(":next_region"));
    bindNullString(QStringLiteral(":next_airport_ident"));
    bindNullFloat(QStringLiteral(":next_minimum_altitude"));
  }

  if(type->hasPreviousWaypoint())
  {
    using namespace atools::geo;

    bind(QStringLiteral(":previous_type"),
         bgl::util::enumToStr(bgl::AirwayWaypoint::airwayWaypointTypeToStr, previousWaypoint.getType()));
    bind(QStringLiteral(":previous_ident"), previousWaypoint.getIdent());
    bind(QStringLiteral(":previous_region"), previousWaypoint.getRegion());
    bind(QStringLiteral(":previous_airport_ident"), previousWaypoint.getAirportIdent());
    bind(QStringLiteral(":previous_minimum_altitude"), roundToPrecision(meterToFeet(previousWaypoint.getMinimumAltitude()), 1));
  }
  else
  {
    bindNullString(QStringLiteral(":previous_type"));
    bindNullString(QStringLiteral(":previous_ident"));
    bindNullString(QStringLiteral(":previous_region"));
    bindNullString(QStringLiteral(":previous_airport_ident"));
    bindNullFloat(QStringLiteral(":previous_minimum_altitude"));
  }

  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
