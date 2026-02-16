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

#include "fs/db/ap/rw/runwaywriter.h"
#include "fs/db//datawriter.h"
#include "fs/bgl/util.h"
#include "fs/bgl/surface.h"
#include "fs/db/ap/airportwriter.h"
#include "fs/db/ap/rw/runwayendwriter.h"
#include "fs/db/runwayindex.h"
#include "fs/navdatabaseoptions.h"
#include "geo/calculations.h"
#include "atools.h"

#include <QString>

namespace atools {
namespace fs {
namespace db {

using atools::fs::bgl::Runway;
using atools::geo::meterToFeet;

void RunwayWriter::writeObject(const Runway *type)
{
  int runwayId = getNextId();

  QString apIdent = getDataWriter().getAirportWriter()->getCurrentAirportIdent();

  // Write runway ends before runway because we need the end ids to keep the foreign keys valid
  RunwayEndWriter *runwayEndWriter = getDataWriter().getRunwayEndWriter();

  runwayEndWriter->writeOne(type->getPrimary());
  int primaryEndId = runwayEndWriter->getCurrentId();
  getRunwayIndex()->add(apIdent, type->getPrimary().getName(), primaryEndId);

  runwayEndWriter->writeOne(type->getSecondary());
  int secondaryEndId = runwayEndWriter->getCurrentId();
  getRunwayIndex()->add(apIdent, type->getSecondary().getName(), secondaryEndId);

  if(getOptions().isVerbose())
    qDebug() << "Writing Runway for airport " << apIdent;

  // Write runway
  bind(QStringLiteral(":runway_id"), runwayId);
  bind(QStringLiteral(":airport_id"), getDataWriter().getAirportWriter()->getCurrentId());
  bind(QStringLiteral(":primary_end_id"), primaryEndId);
  bind(QStringLiteral(":secondary_end_id"), secondaryEndId);

  // Use MSFS material library is UUID is set
  if(!type->getMaterialUuid().isNull())
    bind(QStringLiteral(":surface"), atools::fs::bgl::surface::surfaceToDbStr(getDataWriter().getSurface(type->getMaterialUuid())));
  else
    bind(QStringLiteral(":surface"), atools::fs::bgl::surface::surfaceToDbStr(type->getSurface()));

  bind(QStringLiteral(":length"), roundToInt(meterToFeet(type->getLength())));
  bind(QStringLiteral(":width"), roundToInt(meterToFeet(type->getWidth())));
  bind(QStringLiteral(":heading"), type->getHeading());
  bind(QStringLiteral(":pattern_altitude"), roundToPrecision(meterToFeet(type->getPatternAltitude()), 1));
  bind(QStringLiteral(":marking_flags"), type->getMarkingFlags());
  bind(QStringLiteral(":edge_light"), bgl::util::enumToStr(Runway::lightToStr, type->getEdgeLight()));
  bind(QStringLiteral(":center_light"),
       bgl::util::enumToStr(Runway::lightToStr, type->getCenterLight()));
  bind(QStringLiteral(":has_center_red"), type->isCenterRed());
  bind(QStringLiteral(":altitude"), roundToInt(meterToFeet(type->getPosition().getAltitude())));

  bind(QStringLiteral(":primary_lonx"), type->getPrimaryPosition().getLonX());
  bind(QStringLiteral(":primary_laty"), type->getPrimaryPosition().getLatY());
  bind(QStringLiteral(":secondary_lonx"), type->getSecondaryPosition().getLonX());
  bind(QStringLiteral(":secondary_laty"), type->getSecondaryPosition().getLatY());

  bind(QStringLiteral(":lonx"), type->getPosition().getLonX());
  bind(QStringLiteral(":laty"), type->getPosition().getLatY());

  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
