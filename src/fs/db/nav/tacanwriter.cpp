/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

#include "fs/db/nav/tacanwriter.h"

#include "fs/db/nav/vorwriter.h"
#include "fs/bgl/nav/dme.h"
#include "fs/db/meta/bglfilewriter.h"
#include "fs/db/datawriter.h"
#include "fs/bgl/util.h"
#include "geo/calculations.h"
#include "atools.h"

namespace atools {
namespace fs {
namespace db {

using atools::fs::bgl::Dme;
using atools::fs::bgl::Vor;
using namespace atools::geo;
using namespace atools;

void TacanWriter::writeObject(const bgl::Tacan *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing TACAN " << type->getIdent() << type->getName();

  if(type->getIdent().isEmpty())
  {
    qWarning() << Q_FUNC_INFO << "Found TACAN with empty ident in file"
               << getDataWriter().getBglFileWriter()->getCurrentFilepath();
    return;
  }

  // Use VOR id
  bind(":vor_id", getDataWriter().getVorWriter()->getNextId());
  bind(":file_id", getDataWriter().getBglFileWriter()->getCurrentId());
  bind(":ident", type->getIdent());
  bind(":name", type->getName());
  bind(":region", type->getRegion());
  bind(":type", "TC");

  bind(":airport_ident", type->getAirportIdent());
  bindNullInt(":airport_id");

  bindNullInt(":frequency");
  bind(":channel", type->getChannel());
  bind(":range", roundToInt(meterToNm(type->getRange())));
  bind(":mag_var", getDataWriter().getMagVar(type->getPosition().getPos(), type->getMagVar()));
  bind(":altitude", roundToInt(meterToFeet(type->getPosition().getAltitude())));
  bind(":lonx", type->getPosition().getLonX());
  bind(":laty", type->getPosition().getLatY());
  bind(":dme_only", type->isDmeOnly());

  const Dme *dme = type->getDme();
  if(dme != nullptr)
  {
    bind(":dme_altitude", roundToInt(meterToFeet(dme->getPosition().getAltitude())));
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
