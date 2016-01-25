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

#include "fs/writer/ap/deleteairportwriter.h"
#include "fs/writer/datawriter.h"
#include "fs/bgl/util.h"
#include "fs/bglreaderoptions.h"
#include "fs/writer/ap/airportwriter.h"

namespace atools {
namespace fs {
namespace writer {

using atools::fs::bgl::DeleteAirport;
using atools::sql::SqlQuery;

void DeleteAirportWriter::writeObject(const DeleteAirport *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing Delete for airport "
             << getDataWriter().getAirportWriter()->getCurrentAirportIdent();

  using atools::fs::bgl::util::isFlagSet;
  atools::fs::bgl::del::DeleteAllFlags flags = type->getFlags();

  bind(":delete_airport_id", getNextId());
  bind(":airport_id", getDataWriter().getAirportWriter()->getCurrentId());
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
