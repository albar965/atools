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

#include "fs/db/ap/deleteairportwriter.h"
#include "fs/db/datawriter.h"
#include "fs/bgl/util.h"
#include "fs/navdatabaseoptions.h"
#include "fs/db/ap/airportwriter.h"

namespace atools {
namespace fs {
namespace db {

using atools::fs::bgl::DeleteAirport;

void DeleteAirportWriter::writeObject(const DeleteAirport *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing Delete for airport "
             << getDataWriter().getAirportWriter()->getCurrentAirportIdent();

  using atools::fs::bgl::util::isFlagSet;
  atools::fs::bgl::del::DeleteAllFlags flags = type->getFlags();

  bind(":delete_airport_id", getNextId());
  bind(":airport_id", getDataWriter().getAirportWriter()->getCurrentId());
  bind(":num_del_runway", type->getDeleteRunways().size());
  bind(":num_del_start", type->getDeleteStarts().size());
  bind(":num_del_com", type->getDeleteComs().size());
  bind(":approaches", isFlagSet(flags, bgl::del::APPROACHES));
  bind(":apronlights", isFlagSet(flags, bgl::del::APRONLIGHTS));
  bind(":aprons", isFlagSet(flags, bgl::del::APRONS));
  bind(":frequencies", isFlagSet(flags, bgl::del::COMS));
  bind(":helipads", isFlagSet(flags, bgl::del::HELIPADS));
  bind(":runways", isFlagSet(flags, bgl::del::RUNWAYS));
  bind(":starts", isFlagSet(flags, bgl::del::STARTS));
  bind(":taxiways", isFlagSet(flags, bgl::del::TAXIWAYS));

  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
