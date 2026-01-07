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

#include "fs/bgl/ap/approachtypes.h"
#include "fs/bgl/util.h"
#include "fs/db/ap/airportwriter.h"
#include "fs/db/ap/sidstarapproachlegwriter.h"
#include "fs/db/ap/sidstartransitionwriter.h"
#include "fs/db/ap/sidstarwriter.h"
#include "fs/navdatabaseoptions.h"

namespace atools {
namespace fs {
namespace db {

using atools::fs::bgl::SidStar;
using atools::fs::bgl::ApproachLeg;
using atools::fs::bgl::Transition;

void SidStarWriter::writeObject(const SidStar *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing "
             << QString((type->getSuffix() == 'A') ? "STAR" : "SID")
             << " procedure for airport "
             << getDataWriter().getAirportWriter()->getCurrentAirportIdent();

  // A SidStar record contains data describing multiple approaches. Each approach
  // is related to a RunwayTransition entry unless none exist, in which case, there
  // is only one approach for the record.
  // The logic for this writer is mostly taken from ApproachWriter.

  // First, bind the common column values.
  bind(":airport_id", getDataWriter().getAirportWriter()->getCurrentId());
  // For SID and STAR, the approach type is always "GPS".
  bind(":type", bgl::util::enumToStr(bgl::ap::approachTypeToStr, bgl::ap::GPS));
  bind(":suffix", QChar(type->getSuffix()));
  // SID and STAR always has GPS overlay set.
  bindBool(":has_gps_overlay", true);
  // The "fix" is the SID/STAR procedure name itself.
  bind(":fix_ident", type->getIdent());

  // Save the name of this SID/STAR for logging.
  currentName = QString((type->getSuffix() == 'A') ? "STAR" : "SID") + " " + type->getIdent();

  // Keep a reference to the record for subwriters (due to complexities...)
  currentSidStar = type;

  // The other attributes of "fix" are derived from the IF of the common route or
  // enroute transition (for SIDs), runway transition (for STARs).
  if(!type->getCommonRouteLegs().isEmpty())
  {
    const ApproachLeg leg = type->getCommonRouteLegs().constFirst();
    bind(":fix_type", bgl::util::enumToStr(bgl::ap::approachFixTypeToStr, leg.getFixType()));
    bind(":fix_region", leg.getFixIdent());
    bind(":fix_airport_ident", leg.getFixAirportIdent());
  }
  // The remaining columns are NULL.
  bindNullInt(":altitude");
  bindNullFloat(":heading");
  bindNullInt(":missed_altitude");

  bindNullInt(":runway_end_id");
  bind(":airport_ident", getDataWriter().getAirportWriter()->getCurrentAirportIdent());

  // If the procedure has any runway transition records, we must generate one approach
  // for each of them.
  if(!type->getRunwayTransitionLegs().isEmpty())
  {
    QHashIterator<QString, QList<ApproachLeg> > runwayIter(type->getRunwayTransitionLegs());
    while(runwayIter.hasNext())
    {
      runwayIter.next();

      bind(":runway_name", runwayIter.key());
      bind(":arinc_name", bgl::ap::arincNameAppr(bgl::ap::GPS, runwayIter.key(),
                                                 type->getSuffix(), true /* gpsOverlay */));

      if('A' == type->getSuffix())
        writeArrival(type, &runwayIter.value());
      else
        writeDeparture(type, &runwayIter.value());
    }
  }
  else
  {
    bindNullString(":runway_name");
    bind(":arinc_name", "ALL");

    if('A' == type->getSuffix())
      writeArrival(type, nullptr);
    else
      writeDeparture(type, nullptr);
  }

  // Clear reference to this record.
  currentSidStar = nullptr;
}

void SidStarWriter::writeDeparture(const SidStar *type, const QList<ApproachLeg> *runwayTransLegs)
{
  // Complete the approach first.
  bind(":approach_id", getNextId());
  executeStatement();

  QList<ApproachLeg> commonRouteLegs(type->getCommonRouteLegs());
  // Combine the runway transition legs with any common route legs first. These will
  // be written as 'approach_leg' rows.
  if(runwayTransLegs)
  {
    getDataWriter().getSidStarApproachLegWriter()->write(*runwayTransLegs);
    // ASSUMPTION: The last leg of the runway transition legs and the first leg of the
    // common route legs have the SAME fix.
    if(!commonRouteLegs.isEmpty())
      commonRouteLegs.removeFirst();
  }
  getDataWriter().getSidStarApproachLegWriter()->write(commonRouteLegs);

  // Handle the enroute transitions as transitions.
  getDataWriter().getSidStarTransWriter()->write(type->getEnrouteTransitions().values());
}

void SidStarWriter::writeArrival(const atools::fs::bgl::SidStar *type, const QList<ApproachLeg> *runwayTransLegs)
{
  // Complete the approach first.
  bind(":approach_id", getNextId());
  executeStatement();

  // Combine any common route legs with the runway transition legs first. These will
  // be written as 'approach_leg' rows.
  getDataWriter().getSidStarApproachLegWriter()->write(type->getCommonRouteLegs());
  if(runwayTransLegs)
  {
    QList<ApproachLeg> runwayLegs(*runwayTransLegs);
    if(!type->getCommonRouteLegs().isEmpty())
      // ASSUMPTION: The last leg of the common route legs and the first leg of the
      // runway transition legs have the SAME fix.
      runwayLegs.removeFirst();
    getDataWriter().getSidStarApproachLegWriter()->write(runwayLegs);
  }

  // Handle the enroute transitions as transitions.
  getDataWriter().getSidStarTransWriter()->write(type->getEnrouteTransitions().values());
}

} // namespace db
} // namespace fs
} // namespace atools
