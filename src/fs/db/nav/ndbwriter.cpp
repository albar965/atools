/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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

#include "fs/db/nav/ndbwriter.h"
#include "fs/db/meta/bglfilewriter.h"
#include "fs/db/datawriter.h"
#include "fs/bgl/util.h"
#include "geo/calculations.h"
#include "atools.h"

namespace atools {
namespace fs {
namespace db {

using atools::fs::bgl::Ndb;

void NdbWriter::writeObject(const Ndb *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing NDB " << type->getIdent() << type->getName();

  if(type->getIdent().isEmpty())
  {
    qWarning() << Q_FUNC_INFO << "Found NDB with empty ident in file"
               << getDataWriter().getBglFileWriter()->getCurrentFilepath();
    return;
  }

  using namespace atools::geo;
  using namespace atools;

  bind(":ndb_id", getNextId());
  bind(":file_id", getDataWriter().getBglFileWriter()->getCurrentId());
  bind(":ident", type->getIdent());
  bind(":name", type->getName());
  bind(":region", type->getRegion());
  bind(":type", bgl::Ndb::ndbTypeToStr(type->getType()));

  bind(":airport_ident", type->getAirportIdent());
  bindNullInt(":airport_id");

  bind(":frequency", type->getFrequency());
  bind(":range", roundToInt(meterToNm(type->getRange())));
  bind(":mag_var", type->getMagVar());
  bind(":altitude", roundToInt(meterToFeet(type->getPosition().getAltitude())));
  bind(":lonx", type->getPosition().getLonX());
  bind(":laty", type->getPosition().getLatY());

  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
