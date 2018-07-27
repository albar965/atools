/*****************************************************************************
* Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
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

#include "fs/db/nav/boundarywriter.h"
#include "fs/db/datawriter.h"
#include "fs/common/binarygeometry.h"
#include "fs/bgl/util.h"
#include "fs/db/ap/airportwriter.h"
#include "fs/navdatabaseoptions.h"
#include "fs/db/meta/bglfilewriter.h"
#include "geo/calculations.h"
#include "atools.h"

namespace atools {
namespace fs {
namespace db {

using atools::fs::bgl::Boundary;
using atools::geo::LineString;
namespace  bl = bgl::boundaryline;

void BoundaryWriter::writeObject(const Boundary *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing BOUNDARY " << type->getName();

  bind(":boundary_id", getNextId());
  bind(":file_id", getDataWriter().getBglFileWriter()->getCurrentId());
  bind(":type", bgl::util::enumToStr(bgl::Boundary::boundaryTypeToStr, type->getType()));

  if(type->getName().isEmpty() || type->getName() == "NULL")
    bindNullString(":name");
  else
    bind(":name", type->getName());

  // Fields not used by P3D/FSX
  bindNullString(":multiple_code");
  bind(":time_code", "U");

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

  using namespace atools::geo;
  using namespace atools;

  bind(":max_altitude", roundToInt(meterToFeet(type->getMaxPosition().getAltitude())));
  bind(":max_lonx", type->getMaxPosition().getLonX());
  bind(":max_laty", type->getMaxPosition().getLatY());

  bind(":min_altitude", roundToInt(meterToFeet(type->getMinPosition().getAltitude())));
  bind(":min_lonx", type->getMinPosition().getLonX());
  bind(":min_laty", type->getMinPosition().getLatY());

  atools::fs::common::BinaryGeometry geo(fetchAirspaceLines(type));
  bind(":geometry", geo.writeToByteArray());
  executeStatement();
}

atools::geo::LineString BoundaryWriter::fetchAirspaceLines(const Boundary *type)
{
  const QList<bgl::BoundarySegment>& segments = type->getSegments();
  LineString processedLines;

  for(int i = 0; i < segments.size(); i++)
  {
    const bgl::BoundarySegment& segment = segments.at(i);

    if(segment.getType() == bl::ORIGIN)
      // Origin needed later
      continue;
    else if(segment.getType() == bl::CIRCLE)
      // Append line string build from circle parameters - one point every 15 degrees
      processedLines.append(LineString(segments.at(i - 1).getPosition(), segment.getRadius(), 24));
    else if(segment.getType() == bl::ARC_CCW || segment.getType() == bl::ARC_CW)
      // Build an arc
      processedLines.append(LineString(segments.at(i - 1).getPosition(),
                                       segments.at(i - 2).getPosition(),
                                       segments.at(i).getPosition(),
                                       segment.getType() == bl::ARC_CW, 24));
    else
      processedLines.append(segment.getPosition());
  }
  return processedLines;
}

} // namespace writer
} // namespace fs
} // namespace atools
