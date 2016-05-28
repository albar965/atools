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

#include "fs/db/ap/approachwriter.h"
#include "fs/db/meta/bglfilewriter.h"
#include "fs/db/datawriter.h"
#include "fs/bgl/util.h"
#include "fs/db/ap/airportwriter.h"
#include "fs/db/ap/approachlegwriter.h"
#include "fs/db/runwayindex.h"
#include "fs/db/ap/transitionwriter.h"
#include "fs/bgl/ap/approachtypes.h"
#include "geo/calculations.h"
#include "atools.h"

#include <QString>

namespace atools {
namespace fs {
namespace db {

using atools::fs::bgl::Approach;
using atools::sql::SqlQuery;
using namespace atools::geo;
using namespace atools;

void ApproachWriter::writeObject(const Approach *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing Approach for airport "
             << getDataWriter().getAirportWriter()->getCurrentAirportIdent();

  bind(":approach_id", getNextId());
  bind(":airport_id", getDataWriter().getAirportWriter()->getCurrentId());
  bind(":type", bgl::util::enumToStr(atools::fs::bgl::ap::approachTypeToStr, type->getType()));
  bind(":has_gps_overlay", type->isGpsOverlay());
  bindNullInt(":fix_nav_id");
  bind(":fix_type", bgl::util::enumToStr(atools::fs::bgl::ap::approachFixTypeToStr, type->getFixType()));
  bind(":fix_ident", type->getFixIdent());
  bind(":fix_region", type->getFixRegion());
  bind(":fix_airport_ident", type->getFixAirportIdent());
  bind(":altitude", roundToPrecision(meterToFeet(type->getAltitude()), 1));
  bind(":heading", type->getHeading());
  bind(":missed_altitude", roundToPrecision(meterToFeet(type->getMissedAltitude()), 1));

  const QString& apIdent = getDataWriter().getAirportWriter()->getCurrentAirportIdent();
  bindNullInt(":runway_end_id");
  if(type->hasRunwayReference() && !apIdent.isEmpty())
    if(getOptions().includeAirport(apIdent))
    {
      QString msg(" approach ID " + QString::number(getCurrentId()));
      int id = getRunwayIndex()->getRunwayEndId(apIdent, type->getRunwayName(), msg);
      if(id != -1)
        bind(":runway_end_id", id);
    }

  executeStatement();

  getDataWriter().getApproachLegWriter()->write(type->getLegs());
  getDataWriter().getApproachLegWriter()->write(type->getMissedLegs());

  TransitionWriter *appWriter = getDataWriter().getApproachTransWriter();
  appWriter->write(type->getTransitions());
}

} // namespace writer
} // namespace fs
} // namespace atools
