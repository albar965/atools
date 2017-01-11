/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

#include "fs/db/ap/fencewriter.h"
#include "fs/db/datawriter.h"
#include "fs/bgl/util.h"
#include "fs/navdatabaseoptions.h"
#include "fs/db/ap/airportwriter.h"
#include "fs/bgl/ap/rw/runway.h"

namespace atools {
namespace fs {
namespace db {

using atools::fs::bgl::Fence;
using atools::fs::bgl::Runway;

void FenceWriter::writeObject(const bgl::Fence *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing Fence for airport "
             << getDataWriter().getAirportWriter()->getCurrentAirportIdent();

  bind(":fence_id", getNextId());
  bind(":airport_id", getDataWriter().getAirportWriter()->getCurrentId());
  bind(":type", bgl::Fence::fenceTypeToStr(type->getType()));
  bindCoordinateList(":vertices", type->getVertices());

  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
