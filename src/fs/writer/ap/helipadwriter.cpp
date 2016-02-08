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

#include "fs/writer/ap/helipadwriter.h"
#include "fs/writer/datawriter.h"
#include "fs/bgl/util.h"
#include "fs/bglreaderoptions.h"
#include "fs/writer/ap/airportwriter.h"
#include "fs/bgl/ap/rw/runway.h"

namespace atools {
namespace fs {
namespace writer {

using atools::fs::bgl::Helipad;
using atools::fs::bgl::Runway;
using atools::sql::SqlQuery;

void HelipadWriter::writeObject(const Helipad *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing Helipad for airport "
             << getDataWriter().getAirportWriter()->getCurrentAirportIdent();

  bind(":helipad_id", getNextId());
  bind(":airport_id", getDataWriter().getAirportWriter()->getCurrentId());
  bind(":surface", Runway::surfaceToStr(type->getSurface()));
  bind(":type", bgl::util::enumToStr(Helipad::helipadTypeToStr, type->getType()));
  bind(":length", bgl::util::meterToFeet(type->getLength()));
  bind(":width", bgl::util::meterToFeet(type->getWidth()));
  bind(":heading", type->getHeading());
  bindBool(":is_transparent", type->isTransparent());
  bindBool(":is_closed", type->isClosed());
  bind(":altitude", bgl::util::meterToFeet(type->getPosition().getAltitude()));
  bind(":lonx", type->getPosition().getLonX());
  bind(":laty", type->getPosition().getLatY());

  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
