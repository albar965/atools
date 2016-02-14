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

#include "fs/db/ap/fencewriter.h"
#include "fs/db/datawriter.h"
#include "fs/bgl/util.h"
#include "fs/bglreaderoptions.h"
#include "fs/db/ap/airportwriter.h"
#include "fs/bgl/ap/rw/runway.h"

namespace atools {
namespace fs {
namespace db {

using atools::fs::bgl::Fence;
using atools::fs::bgl::Runway;
using atools::sql::SqlQuery;

void FenceWriter::writeObject(const bgl::Fence *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing Fence for airport "
             << getDataWriter().getAirportWriter()->getCurrentAirportIdent();

  bind(":fence_id", getNextId());
  bind(":airport_id", getDataWriter().getAirportWriter()->getCurrentId());
  bind(":type", bgl::Fence::fenceTypeToStr(type->getType()));

  // TODO create a WKT polygon from the triangles
  QStringList list;
  for(const bgl::BglPosition& pos : type->getVertices())
    list.push_back(QString::number(pos.getLonX(), 'g', 8) + " " +
                   QString::number(pos.getLatY(), 'g', 8));
  bind(":vertices", list.join(", "));

  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
