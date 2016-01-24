/*
 * MarkerWriter.cpp
 *
 *  Created on: 01.05.2015
 *      Author: alex
 */

#include "markerwriter.h"
#include "../meta/bglfilewriter.h"
#include "../datawriter.h"
#include "../../bgl/util.h"

namespace atools {
namespace fs {
namespace writer {

using atools::fs::bgl::Marker;
using atools::sql::SqlQuery;

void MarkerWriter::writeObject(const Marker *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing Marker " << type->getIdent();

  bind(":marker_id", getNextId());
  bind(":file_id", getDataWriter().getBglFileWriter().getCurrentId());
  bind(":ident", type->getIdent());
  bind(":region", type->getRegion());
  bind(":type", bgl::Marker::markerTypeToStr(type->getType()));
  bind(":altitude", bgl::util::meterToFeet(type->getPosition().getAltitude()));
  bind(":lonx", type->getPosition().getLonX());
  bind(":laty", type->getPosition().getLatY());

  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
