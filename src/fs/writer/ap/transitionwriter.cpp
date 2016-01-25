/*
 * ApproachTransWriter.cpp
 *
 *  Created on: 01.05.2015
 *      Author: alex
 */

#include "fs/writer/datawriter.h"
#include "fs/bgl/util.h"
#include "fs/writer/ap/transitionwriter.h"
#include "fs/bglreaderoptions.h"
#include "fs/writer/ap/approachwriter.h"
#include "fs/writer/ap/airportwriter.h"

namespace atools {
namespace fs {
namespace writer {

using atools::fs::bgl::Transition;
using atools::sql::SqlQuery;

void TransitionWriter::writeObject(const Transition *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing Transition for airport "
             << getDataWriter().getAirportWriter()->getCurrentAirportIdent();

  bind(":transition_id", getNextId());
  bind(":approach_id", getDataWriter().getApproachWriter()->getCurrentId());
  bind(":type", Transition::transitionTypeToStr(type->getType()));
  bind(":num_legs", type->getNumLegs());
  bind(":fix_type", Transition::transitionFixTypeToStr(type->getTransFixType()));
  bind(":fix_ident", type->getTransFixIdent());
  bind(":fix_region", type->getFixRegion());
  bind(":fix_airport_ident", type->getFixAirportIdent());
  bind(":altitude", bgl::util::meterToFeet(type->getAltitude(), 1));
  bind(":dme_ident", type->getDmeIdent());
  bind(":dme_region", type->getDmeRegion());
  bind(":dme_airport_ident", type->getDmeAirportIdent());
  bind(":dme_radial", type->getDmeRadial());
  bind(":dme_distance", bgl::util::meterToNm(type->getDmeDist()));

  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
