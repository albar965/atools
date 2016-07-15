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

#include "fs/db/nav/vorwriter.h"
#include "fs/bgl/nav/dme.h"
#include "fs/db/meta/bglfilewriter.h"
#include "fs/db/datawriter.h"
#include "fs/bgl/util.h"
#include "fs/db/airportindex.h"
#include "geo/calculations.h"
#include "atools.h"

namespace atools {
namespace fs {
namespace db {

using atools::fs::bgl::Dme;
using atools::fs::bgl::Vor;

void VorWriter::writeObject(const Vor *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing VOR " << type->getIdent() << type->getName();

  using namespace atools::geo;
  using namespace atools;

  bind(":vor_id", getNextId());
  bind(":file_id", getDataWriter().getBglFileWriter()->getCurrentId());
  bind(":ident", type->getIdent());
  bind(":name", type->getName());
  bind(":region", type->getRegion());
  bind(":type", bgl::IlsVor::ilsVorTypeToStr(type->getType()));
  bind(":frequency", type->getFrequency());
  bind(":range", roundToPrecision(meterToNm(type->getRange())));
  bind(":mag_var", type->getMagVar());
  bind(":dme_only", type->isDmeOnly());
  bind(":altitude", roundToPrecision(meterToFeet(type->getPosition().getAltitude())));
  bind(":lonx", type->getPosition().getLonX());
  bind(":laty", type->getPosition().getLatY());

  bindNullInt(":airport_id");
  QString apIdent = type->getAirportIdent();
  if(!apIdent.isEmpty() && getOptions().isIncludedAirportIdent(apIdent))
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
    bind(":dme_altitude", roundToPrecision(meterToFeet(dme->getPosition().getAltitude())));
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
