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

#include "fs/writer/boundarylinewriter.h"
#include "fs/writer/boundarywriter.h"
#include "fs/writer/datawriter.h"
#include "fs/bgl/util.h"
#include "fs/writer/ap/airportwriter.h"
#include "fs/bglreaderoptions.h"

namespace atools {
namespace fs {
namespace writer {

using atools::fs::bgl::BoundaryLine;
using atools::sql::SqlQuery;

void BoundaryLineWriter::writeObject(const BoundaryLine *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing BOUNDARYLINE ";

  bind(":boundary_line_id", getNextId());
  bind(":boundary_id", getDataWriter().getBoundaryWriter()->getCurrentId());
  bind(":type", bgl::util::enumToStr(bgl::BoundaryLine::boundarylineTypeToStr, type->getType()));

  if(type->getType() == bgl::boundaryline::CIRCLE)
  {
    bind(":radius", bgl::util::meterToNm(type->getRadius()));
    bindNullFloat(":lonx");
    bindNullFloat(":laty");
  }
    else
  {
    bindNullFloat(":radius");
    bind(":lonx", type->getPosition().getLonX());
    bind(":laty", type->getPosition().getLatY());
  }
  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
