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

#include "fs/writer/boundarywriter.h"
#include "fs/writer/datawriter.h"
#include "fs/bgl/util.h"
#include "fs/writer/ap/airportwriter.h"
#include "fs/bglreaderoptions.h"
#include "fs/writer/boundarylinewriter.h"
#include "fs/writer/meta/bglfilewriter.h"

namespace atools {
namespace fs {
namespace writer {

using atools::fs::bgl::Boundary;
using atools::sql::SqlQuery;


void BoundaryWriter::writeObject(const Boundary *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing BOUNDARY " << type->getName();

  bind(":boundary_id", getNextId());
  bind(":file_id", getDataWriter().getBglFileWriter()->getCurrentId());
  bind(":type", bgl::util::enumToStr(bgl::Boundary::boundaryTypeToStr, type->getType()));
  bind(":name", type->getName());

  if(type->hasCom())
  {
    bind(":com_type", bgl::util::enumToStr(bgl::Com::comTypeToStr, type->getComType()));
    bind(":com_frequency", type->getComFrequency());
    bind(":com_name", type->getComName());
  }
  else
  {
    bindNullString(":com_type");
    bindNullInt(":com_frequency");
    bindNullString(":com_name");
  }

  bind(":min_altitude_type", bgl::util::enumToStr(bgl::Boundary::altTypeToStr, type->getMinAltType()));
  bind(":max_altitude_type", bgl::util::enumToStr(bgl::Boundary::altTypeToStr, type->getMaxAltType()));

  bind(":max_altitude", bgl::util::meterToFeet(type->getMaxPosition().getAltitude()));
  bind(":max_lonx", type->getMaxPosition().getLonX());
  bind(":max_laty", type->getMaxPosition().getLatY());

  bind(":min_altitude", bgl::util::meterToFeet(type->getMinPosition().getAltitude()));
  bind(":min_lonx", type->getMinPosition().getLonX());
  bind(":min_laty", type->getMinPosition().getLatY());
  executeStatement();

  BoundaryLineWriter *w = getDataWriter().getBoundaryLineWriter();
  w->write(type->getLines());
}

} // namespace writer
} // namespace fs
} // namespace atools
