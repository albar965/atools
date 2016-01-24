/*
 * VorWriter.cpp
 *
 *  Created on: 01.05.2015
 *      Author: alex
 */

#include "vorwriter.h"
#include "../../bgl/nav/dme.h"
#include "../meta/bglfilewriter.h"
#include "../datawriter.h"
#include "../../bgl/util.h"

namespace atools {
namespace fs {
namespace writer {

using atools::fs::bgl::Dme;
using atools::fs::bgl::Vor;
using atools::sql::SqlQuery;

void VorWriter::writeObject(const Vor *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing VOR " << type->getIdent() << type->getName();

  bind(":vor_id", getNextId());
  bind(":file_id", getDataWriter().getBglFileWriter().getCurrentId());
  bind(":ident", type->getIdent());
  bind(":name", type->getName());
  bind(":region", type->getRegion());
  bind(":type", bgl::IlsVor::ilsVorTypeToStr(type->getType()));
  bind(":frequency", type->getFrequency());
  bind(":range", bgl::util::meterToNm(type->getRange()));
  bind(":mag_var", type->getMagVar());
  bind(":dme_only", type->isDmeOnly());
  bind(":altitude", bgl::util::meterToFeet(type->getPosition().getAltitude()));
  bind(":lonx", type->getPosition().getLonX());
  bind(":laty", type->getPosition().getLatY());

  QString apIdent = type->getAirportIdent();
  if(!apIdent.isEmpty() && getOptions().doesAirportIcaoMatch(apIdent))
  {
    QString msg("VOR ID " + QString::number(getCurrentId()) +
                " ident " + type->getIdent() + " name " + type->getName());
    int id = getAirportIndex().getAirportId(apIdent, msg);
    if(id != -1)
      bind(":airport_id", id);
    else
      bind(":airport_id", QVariant(QVariant::Int));
  }

  const Dme *dme = type->getDme();
  if(dme != nullptr)
  {
    bind(":dme_altitude", bgl::util::meterToFeet(dme->getPosition().getAltitude()));
    bind(":dme_lonx", dme->getPosition().getLonX());
    bind(":dme_laty", dme->getPosition().getLatY());
  }

  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
