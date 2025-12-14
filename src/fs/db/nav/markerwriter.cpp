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

#include "fs/db/nav/markerwriter.h"
#include "fs/db/meta/bglfilewriter.h"
#include "fs/db/datawriter.h"
#include "fs/bgl/util.h"
#include "geo/calculations.h"
#include "atools.h"

namespace atools {
namespace fs {
namespace db {

using atools::fs::bgl::Marker;

void MarkerWriter::writeObject(const Marker *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing Marker " << type->getIdent();

  using namespace atools::geo;
  using namespace atools;

  bind(":marker_id", getNextId());
  bind(":file_id", getDataWriter().getBglFileWriter()->getCurrentId());
  bind(":ident", type->getIdent());
  bind(":region", type->getRegion());
  bind(":type", bgl::Marker::markerTypeToStr(type->getType()));
  bind(":heading", type->getHeading());
  bind(":altitude", roundToInt(meterToFeet(type->getPosition().getAltitude())));
  bind(":lonx", type->getPosition().getLonX());
  bind(":laty", type->getPosition().getLatY());

  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
