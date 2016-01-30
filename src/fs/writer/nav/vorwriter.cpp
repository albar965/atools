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

#include "fs/writer/nav/vorwriter.h"
#include "fs/bgl/nav/dme.h"
#include "fs/writer/meta/bglfilewriter.h"
#include "fs/writer/datawriter.h"
#include "fs/bgl/util.h"
#include "fs/writer/airportindex.h"

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
  bind(":file_id", getDataWriter().getBglFileWriter()->getCurrentId());
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

  bindNullInt(":airport_id");
  QString apIdent = type->getAirportIdent();
  if(!apIdent.isEmpty() && getOptions().includeAirport(apIdent))
  {
    QString msg("VOR ID " + QString::number(getCurrentId()) +
                " ident " + type->getIdent() + " name " + type->getName());
    int id = getAirportIndex()->getAirportId(apIdent, msg);
    if(id != -1)
      bind(":airport_id", id);
  }

  const Dme *dme = type->getDme();
  if(dme != nullptr)
  {
    bind(":dme_altitude", bgl::util::meterToFeet(dme->getPosition().getAltitude()));
    bind(":dme_lonx", dme->getPosition().getLonX());
    bind(":dme_laty", dme->getPosition().getLatY());
  }
  else
  {
    bindNullFloat(":dme_altitude");
    bindNullFloat(":dme_lonx");
    bindNullFloat(":dme_laty");
  }
  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
