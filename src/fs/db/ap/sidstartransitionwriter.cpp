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

#include "fs/db/datawriter.h"

#include "fs/bgl/util.h"
#include "fs/db/ap/airportwriter.h"
#include "fs/db/ap/sidstartransitionlegwriter.h"
#include "fs/db/ap/sidstartransitionwriter.h"
#include "fs/db/ap/sidstarwriter.h"
#include "fs/navdatabaseoptions.h"

namespace atools {
namespace fs {
namespace db {

using atools::fs::bgl::SidStar;
using atools::fs::bgl::ApproachLeg;
using atools::fs::bgl::Transition;

void SidStarTransitionWriter::writeObject(const QList<atools::fs::bgl::ApproachLeg> *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing EnrouteTransition for "
             << getDataWriter().getSidStarWriter()->getCurrentApproachName()
             << " at airport "
             << getDataWriter().getAirportWriter()->getCurrentAirportIdent();

  bind(":transition_id", getNextId());
  bind(":approach_id", getDataWriter().getSidStarWriter()->getCurrentId());

  // For SIDs and STARs, the transition type is always FULL.
  bind(":type", Transition::transitionTypeToStr(bgl::ap::FULL));
  // Most of the fields are omitted too.
  bindNullFloat(":altitude");
  bindNullString(":dme_ident");
  bindNullString(":dme_region");
  bindNullString(":dme_airport_ident");
  bindNullInt(":dme_radial");
  bindNullInt(":dme_distance");

  bool isArrival = 'A' == getDataWriter().getSidStarWriter()->getCurrentSidStar()->getSuffix();
  const ApproachLeg& fixLeg = isArrival ? type->constFirst() : type->constLast();
  // Fill in the fix information.
  bind(":fix_type", bgl::util::enumToStr(bgl::ap::approachFixTypeToStr, fixLeg.getFixType()));
  bind(":fix_ident", fixLeg.getFixIdent());
  bind(":fix_region", fixLeg.getFixRegion());
  bind(":fix_airport_ident", fixLeg.getFixAirportIdent());

  // Write the transition row.
  executeStatement();

  // Then write the transition legs.
  getDataWriter().getSidStarTransLegWriter()->write(*type);
}

} // namespace db
} // namespace fs
} // namespace atools
