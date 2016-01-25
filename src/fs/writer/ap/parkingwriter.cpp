/*
 * ParkingWriter.cpp
 *
 *  Created on: 01.05.2015
 *      Author: alex
 */

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
  bind(":lonx", type->getPosition().getLonX());
  bind(":laty", type->getPosition().getLatY());

  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
