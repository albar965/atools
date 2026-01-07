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

#include "fs/db/ap/approachwriter.h"

#include "atools.h"
#include "fs/bgl/ap/approachtypes.h"
#include "fs/bgl/util.h"
#include "fs/db/ap/airportwriter.h"
#include "fs/db/ap/approachlegwriter.h"
#include "fs/db/ap/transitionwriter.h"
#include "fs/db/datawriter.h"
#include "fs/db/runwayindex.h"
#include "geo/calculations.h"

#include <QString>

namespace atools {
namespace fs {
namespace db {

using atools::fs::bgl::Approach;
using namespace atools::geo;
using namespace atools;

void ApproachWriter::writeObject(const Approach *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing Approach for airport "
             << getDataWriter().getAirportWriter()->getCurrentAirportIdent();

  QString apptype = bgl::util::enumToStr(atools::fs::bgl::ap::approachTypeToStr, type->getType());
  DataWriter& dataWriter = getDataWriter();

  if(apptype.isEmpty())
  {
    qWarning() << Q_FUNC_INFO << "Skipping approach with invalid type" << type->getFixIdent() << type->getFixRegion()
               << dataWriter.getAirportWriter()->getCurrentAirportIdent();
    return;
  }

  bind(":approach_id", getNextId());
  bind(":airport_id", dataWriter.getAirportWriter()->getCurrentId());
  bind(":type", apptype);

  if(type->getSuffix() == '0' || type->getSuffix() == 0)
    bindNullString(":suffix");
  else
    bind(":suffix", QChar(type->getSuffix()));

  bind(":has_gps_overlay", type->hasGpsOverlay());
  bind(":fix_type", bgl::util::enumToStr(atools::fs::bgl::ap::approachFixTypeToStr, type->getFixType()));
  bind(":fix_ident", type->getFixIdent());
  bind(":fix_region", type->getFixRegion());
  bind(":fix_airport_ident", type->getFixAirportIdent());
  bind(":altitude", roundToPrecision(meterToFeet(type->getAltitude()), 1));
  bind(":heading", type->getHeading());
  bind(":missed_altitude", roundToPrecision(meterToFeet(type->getMissedAltitude()), 1));

  bindNullInt(":runway_end_id");
  bind(":airport_ident", dataWriter.getAirportWriter()->getCurrentAirportIdent());

  QString runwayName;
  if(/*type->getRunwayName() == "36" ||*/ type->getRunwayName() == "00" || type->getRunwayName().isEmpty())
  {
    // No valid runway given - try to find one in the approach legs
    QString runway;
    for(const atools::fs::bgl::ApproachLeg& leg : type->getLegs())
    {
      if(leg.getFixType() == atools::fs::bgl::ap::fix::RUNWAY)
      {
        if(leg.getFixIdent().startsWith("RW"))
          runway = leg.getFixIdent().mid(2);
        else
          runway = leg.getFixIdent();
        break;
      }
    }

    if(runway.isEmpty())
    {
      // Use invalid 36 as fallback if nothing found
      runway = type->getRunwayName();

      // Look into the index
      int id = dataWriter.getRunwayIndex()->getRunwayEndId(
        dataWriter.getAirportWriter()->getCurrentAirportIdent(), runway, "approach runway");
      if(id != -1)
        runwayName = runway;
      // else
      //// Nothing found in index
      // bindNullString(":runway_name");
    }
    else
      // Use runway name as found in legs
      runwayName = runway;
  }
  else
    // Use given runway name
    runwayName = type->getRunwayName();

  if(runwayName.isEmpty())
    bindNullString(":runway_name");
  else
    bind(":runway_name", runwayName);

  bind(":arinc_name", bgl::ap::arincNameAppr(type->getType(), runwayName, type->getSuffix(), type->hasGpsOverlay()));

  // Write approach
  executeStatement();

  // Write all legs
  dataWriter.getApproachLegWriter()->write(type->getLegs());
  dataWriter.getApproachLegWriter()->write(type->getMissedLegs());

  // Write transitions for this approach
  TransitionWriter *appWriter = dataWriter.getApproachTransWriter();
  appWriter->write(type->getTransitions());
}

} // namespace writer
} // namespace fs
} // namespace atools
