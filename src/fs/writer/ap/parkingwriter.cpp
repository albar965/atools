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

#include "fs/writer/ap/parkingwriter.h"
#include "fs/writer/datawriter.h"
#include "fs/bgl/util.h"
#include "fs/bglreaderoptions.h"
#include "fs/writer/ap/airportwriter.h"

namespace atools {
namespace fs {
namespace writer {

using atools::fs::bgl::Parking;
using atools::sql::SqlQuery;

void ParkingWriter::writeObject(const Parking *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing Parking for airport "
             << getDataWriter().getAirportWriter()->getCurrentAirportIdent();

  bind(":parking_id", getNextId());
  bind(":airport_id", getDataWriter().getAirportWriter()->getCurrentId());
  bind(":type", bgl::util::enumToStr(Parking::parkingTypeToStr, type->getType()));
  bind(":name", bgl::util::enumToStr(Parking::parkingNameToStr, type->getName()));
  bind(":number", type->getNumber());
  bind(":radius", bgl::util::meterToFeet(type->getRadius()));
  bind(":heading", type->getHeading());
  bind(":has_jetway", type->hasJetway() ? 1 : 0);
  bind(":lonx", type->getPosition().getLonX());
  bind(":laty", type->getPosition().getLatY());

  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
