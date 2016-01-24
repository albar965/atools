/*
 * AirportComWriter.cpp
 *
 *  Created on: 01.05.2015
 *      Author: alex
 */

#include "comwriter.h"
#include "../datawriter.h"
#include "../../bgl/util.h"

namespace atools {
namespace fs {
namespace writer {

using atools::fs::bgl::Com;
using atools::sql::SqlQuery;

void ComWriter::writeObject(const Com *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing COM for airport "
             << getDataWriter().getAirportWriter().getCurrentAirportIdent();

  bind(":com_id", getNextId());
  bind(":airport_id", getDataWriter().getAirportWriter().getCurrentId());
  bind(":type", bgl::util::enumToStr(bgl::Com::comTypeToStr, type->getType()));
  bind(":frequency", type->getFrequency());
  bind(":name", type->getName());

  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
