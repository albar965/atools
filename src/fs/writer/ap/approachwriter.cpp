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

#include "fs/writer/ap/approachwriter.h"
#include "fs/writer/meta/bglfilewriter.h"
#include "fs/writer/datawriter.h"
#include "fs/bgl/util.h"
#include "fs/writer/ap/airportwriter.h"
#include "fs/writer/runwayindex.h"
#include "fs/writer/ap/transitionwriter.h"
#include "fs/bgl/ap/approachtypes.h"

#include <QString>

namespace atools {
namespace fs {
namespace writer {

using atools::fs::bgl::Approach;
using atools::sql::SqlQuery;

void ApproachWriter::writeObject(const Approach *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing Approach for airport "
             << getDataWriter().getAirportWriter()->getCurrentAirportIdent();

  bind(":approach_id", getNextId());
  bind(":type", bgl::util::enumToStr(atools::fs::bgl::ap::approachTypeToStr, type->getType()));
  bind(":has_gps_overlay", type->isGpsOverlay());
  bind(":num_legs", type->getNumLegs());
  bind(":num_missed_legs", type->getNumMissedLegs());
  bind(":fix_type", bgl::util::enumToStr(atools::fs::bgl::ap::approachFixTypeToStr, type->getFixType()));
  bind(":fix_ident", type->getFixIdent());
  bind(":fix_region", type->getFixRegion());
  bind(":fix_airport_ident", type->getFixAirportIdent());
  bind(":altitude", bgl::util::meterToFeet(type->getAltitude(), 1));
  bind(":heading", type->getHeading());
  bind(":missed_altitude", bgl::util::meterToFeet(type->getMissedAltitude(), 1));

  bool isComplete = false;
  const QString& apIdent = getDataWriter().getAirportWriter()->getCurrentAirportIdent();
  bindNullInt(":runway_end_id");
  if(type->hasRunwayReference() && !apIdent.isEmpty())
  {
    if(getOptions().includeAirport(apIdent))
    {
      QString msg(" approach ID " + QString::number(getCurrentId()));
      int id = getRunwayIndex()->getRunwayEndId(apIdent, type->getRunwayName(), msg);
      if(id != -1)
      {
        isComplete = true;
        bind(":runway_end_id", id);
      }
    }
  }
  else
    isComplete = true;

  if(getOptions().isIncomplete() || isComplete)
  {
    executeStatement();

    TransitionWriter *appWriter = getDataWriter().getApproachTransWriter();
    appWriter->write(type->getTransitions());
  }
}

} // namespace writer
} // namespace fs
} // namespace atools
