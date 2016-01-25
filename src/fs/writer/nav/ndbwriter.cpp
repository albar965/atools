/*
 * NdbWriter.cpp
 *
 *  Created on: 01.05.2015
 *      Author: alex
 */

#include "fs/writer/nav/ndbwriter.h"
#include "fs/writer/meta/bglfilewriter.h"
#include "fs/writer/datawriter.h"
#include "fs/bgl/util.h"
#include "fs/writer/airportindex.h"

namespace atools {
namespace fs {
namespace writer {

using atools::fs::bgl::Ndb;
using atools::sql::SqlQuery;

void NdbWriter::writeObject(const Ndb *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing NDB " << type->getIdent() << type->getName();

  bind(":ndb_id", getNextId());
  bind(":file_id", getDataWriter().getBglFileWriter()->getCurrentId());
  bind(":ident", type->getIdent());
  bind(":name", type->getName());
  bind(":region", type->getRegion());
  bind(":type", bgl::Ndb::ndbTypeToStr(type->getType()));
  bind(":frequency", type->getFrequency());
  bind(":range", bgl::util::meterToNm(type->getRange()));
  bind(":mag_var", type->getMagVar());
  bind(":altitude", bgl::util::meterToFeet(type->getPosition().getAltitude()));
  bind(":lonx", type->getPosition().getLonX());
  bind(":laty", type->getPosition().getLatY());

  QString apIdent = type->getAirportIdent();
  if(!apIdent.isEmpty() && getOptions().doesAirportIcaoMatch(apIdent))
  {
    QString msg("NDB ID " + QString::number(getCurrentId()) +
                " ident " + type->getIdent() + " name " + type->getName());
    int id = getAirportIndex()->getAirportId(apIdent, msg);
    if(id != -1)
      bind(":airport_id", id);
    else
      bind(":airport_id", QVariant(QVariant::Int));
  }

  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
