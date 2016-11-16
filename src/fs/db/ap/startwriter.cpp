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

#include "fs/db/ap/startwriter.h"
#include "fs/db/datawriter.h"
#include "fs/bgl/util.h"
#include "fs/navdatabaseoptions.h"
#include "fs/db/ap/airportwriter.h"
#include "fs/db/runwayindex.h"
#include "geo/calculations.h"
#include "atools.h"

namespace atools {
namespace fs {
namespace db {

using atools::fs::bgl::Start;
using atools::geo::meterToFeet;

void StartWriter::writeObject(const Start *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing Start for airport "
             << getDataWriter().getAirportWriter()->getCurrentAirportIdent();

  bind(":start_id", getNextId());
  bind(":airport_id", getDataWriter().getAirportWriter()->getCurrentId());
  bind(":runway_name", type->getRunwayName());
  bind(":type", bgl::util::enumToStr(Start::startTypeToStr, type->getType()));
  bind(":heading", type->getHeading());
  bind(":altitude", roundToInt(meterToFeet(type->getPosition().getAltitude())));
  bind(":lonx", type->getPosition().getLonX());
  bind(":laty", type->getPosition().getLatY());

  bool isComplete = false;
  const QString& apIdent = getDataWriter().getAirportWriter()->getCurrentAirportIdent();
  bindNullInt(":runway_end_id");

  if(type->getType() == bgl::start::HELIPAD)
    bind(":number", type->getNumber());
  else
    bindNullInt(":number");

  // TODO comment in wiki: helipads have no runway
  if(!apIdent.isEmpty() && type->getType() != bgl::start::HELIPAD)
  {
    // Get associated runway for start position
    if(getOptions().isIncludedAirportIdent(apIdent))
    {
      QString msg(" start ID " + QString::number(getCurrentId()));
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
    executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
