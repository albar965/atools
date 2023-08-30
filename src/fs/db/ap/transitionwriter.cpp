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

#include "fs/db/datawriter.h"

#include "atools.h"
#include "fs/db/ap/airportwriter.h"
#include "fs/db/ap/approachwriter.h"
#include "fs/db/ap/transitionlegwriter.h"
#include "fs/db/ap/transitionwriter.h"
#include "fs/navdatabaseoptions.h"
#include "geo/calculations.h"

namespace atools {
namespace fs {
namespace db {

using atools::fs::bgl::Transition;
using atools::geo::meterToFeet;
using atools::geo::meterToNm;

void TransitionWriter::writeObject(const Transition *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing Transition for airport "
             << getDataWriter().getAirportWriter()->getCurrentAirportIdent();

  QString transtype = Transition::transitionTypeToStr(type->getType());
  if(transtype.isEmpty())
  {
    qWarning() << Q_FUNC_INFO << "Skipping transition with invalid type"
               << getDataWriter().getAirportWriter()->getCurrentAirportIdent();
    return;
  }

  bind(":transition_id", getNextId());
  bind(":approach_id", getDataWriter().getApproachWriter()->getCurrentId());

  bind(":type", transtype);
  bind(":fix_type", Transition::transitionFixTypeToStr(type->getTransFixType()));
  bind(":fix_ident", type->getTransFixIdent());
  bind(":fix_region", type->getFixRegion());
  bind(":fix_airport_ident", type->getFixAirportIdent());
  bind(":altitude", roundToPrecision(meterToFeet(type->getAltitude()), 1));

  // Write DME if available - otherwise null all fields
  if(type->getType() == bgl::ap::DME)
  {
    bind(":dme_ident", type->getDmeIdent());
    bind(":dme_region", type->getDmeRegion());
    bind(":dme_airport_ident", type->getDmeAirportIdent());
    bind(":dme_radial", type->getDmeRadial());
    bind(":dme_distance", roundToInt(meterToNm(type->getDmeDistance())));
  }
  else
  {
    bindNullString(":dme_ident");
    bindNullString(":dme_region");
    bindNullString(":dme_airport_ident");
    bindNullInt(":dme_radial");
    bindNullInt(":dme_distance");
  }
  executeStatement();

  // Write transition legs
  getDataWriter().getApproachTransLegWriter()->write(type->getLegs());
}

} // namespace writer
} // namespace fs
} // namespace atools
