/*
 * RunwayWriter.cpp
 *
 *  Created on: 01.05.2015
 *      Author: alex
 */

#include "fs/writer/ap/rw/runwaywriter.h"
#include "fs/writer//datawriter.h"
#include "fs/bgl/util.h"
#include "fs/writer/ap/airportwriter.h"
#include "fs/writer/ap/rw/runwayendwriter.h"
#include "fs/writer/runwayindex.h"
#include "fs/bglreaderoptions.h"

#include <QString>

namespace atools {
namespace fs {
namespace writer {

using atools::fs::bgl::Runway;
using atools::sql::SqlQuery;

void RunwayWriter::writeObject(const Runway *type)
{
  int runwayId = getNextId();

  QString apIdent = getDataWriter().getAirportWriter()->getCurrentAirportIdent();

  RunwayEndWriter *rweWriter = getDataWriter().getRunwayEndWriter();
  rweWriter->writeOne(&(type->getPrimary()));
  int primaryEndId = rweWriter->getCurrentId();
  getRunwayIndex()->add(apIdent, type->getPrimary().getName(), primaryEndId);

  rweWriter->writeOne(&(type->getSecondary()));
  int secondaryEndId = rweWriter->getCurrentId();
  getRunwayIndex()->add(apIdent, type->getSecondary().getName(), secondaryEndId);

  if(getOptions().isVerbose())
    qDebug() << "Writing Runway for airport " << apIdent;

  bind(":runway_id", runwayId);
  bind(":airport_id", getDataWriter().getAirportWriter()->getCurrentId());
  bind(":primary_end_id", primaryEndId);
  bind(":secondary_end_id", secondaryEndId);
  bind(":surface", Runway::surfaceToStr(type->getSurface()));
  bind(":length", bgl::util::meterToFeet(type->getLength()));
  bind(":width", bgl::util::meterToFeet(type->getWidth()));
  bind(":heading", type->getHeading());
  bind(":pattern_altitude", bgl::util::meterToFeet(type->getPatternAltitude(), 1));
  bind(":marking_flags", type->getMarkingFlags());
  bind(":light_flags", type->getLightFlags());
  bind(":pattern_flags", type->getPatternFlags());
  bind(":edge_light", bgl::util::enumToStr(Runway::lightToStr, type->getEdgeLight()));
  bind(":center_light",
       bgl::util::enumToStr(Runway::lightToStr, type->getCenterLight()));
  bind(":has_center_red", type->isCenterRed());
  bind(":altitude", bgl::util::meterToFeet(type->getPosition().getAltitude()));
  bind(":lonx", type->getPosition().getLonX());
  bind(":laty", type->getPosition().getLatY());

  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
