/*
 * AirportDeleteAirportWriter.cpp
 *
 *  Created on: 01.05.2015
 *      Author: alex
 */

#include "deleteairportwriter.h"
#include "../datawriter.h"
#include "../../bgl/util.h"

namespace atools {
namespace fs {
namespace writer {

using atools::fs::bgl::DeleteAirport;
using atools::sql::SqlQuery;

void DeleteAirportWriter::writeObject(const DeleteAirport *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing Delete for airport "
             << getDataWriter().getAirportWriter().getCurrentAirportIdent();

  using atools::fs::bgl::util::isFlagSet;
  atools::fs::bgl::del::DeleteAllFlags flags = type->getFlags();

  bind(":delete_airport_id", getNextId());
  bind(":airport_id", getDataWriter().getAirportWriter().getCurrentId());
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
