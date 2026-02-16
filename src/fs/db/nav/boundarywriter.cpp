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

#include "fs/db/nav/boundarywriter.h"

#include "atools.h"
#include "fs/bgl/util.h"
#include "fs/common/binarygeometry.h"
#include "fs/db/datawriter.h"
#include "fs/db/meta/bglfilewriter.h"
#include "fs/navdatabaseoptions.h"
#include "fs/util/fsutil.h"
#include "geo/calculations.h"

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

  bind(QStringLiteral(":boundary_id"), getNextId());
  bind(QStringLiteral(":file_id"), getDataWriter().getBglFileWriter()->getCurrentId());
  bind(QStringLiteral(":type"), bgl::util::enumToStr(bgl::Boundary::boundaryTypeToStr, type->getType()));

  if(type->getName().isEmpty() || type->getName() == QStringLiteral("NULL"))
    bindNullString(QStringLiteral(":name"));
  else
    bind(QStringLiteral(":name"), type->getName());

  // Fields not used by P3D/FSX
  bindNullString(QStringLiteral(":multiple_code"));
  bindNullString(QStringLiteral(":restrictive_type"));
  bindNullString(QStringLiteral(":restrictive_designation"));
  bind(QStringLiteral(":time_code"), QStringLiteral("U"));

  if(type->hasCom())
  {
    bind(QStringLiteral(":com_type"), bgl::util::enumToStr(bgl::Com::comTypeToStr, type->getComType()));
    bind(QStringLiteral(":com_frequency"), type->getComFrequency());
    bind(QStringLiteral(":com_name"), type->getComName());
  }
  else
  {
    bindNullString(QStringLiteral(":com_type"));
    bindNullInt(QStringLiteral(":com_frequency"));
    bindNullString(QStringLiteral(":com_name"));
  }

  bind(QStringLiteral(":min_altitude_type"), bgl::util::enumToStr(bgl::Boundary::altTypeToStr, type->getMinAltType()));
  bind(QStringLiteral(":max_altitude_type"), bgl::util::enumToStr(bgl::Boundary::altTypeToStr, type->getMaxAltType()));

  using namespace atools::geo;
  using namespace atools;

  bind(QStringLiteral(":max_altitude"), roundToInt(meterToFeet(type->getMaxPosition().getAltitude())));
  bind(QStringLiteral(":max_lonx"), type->getMaxPosition().getLonX());
  bind(QStringLiteral(":max_laty"), type->getMaxPosition().getLatY());

  bind(QStringLiteral(":min_altitude"), roundToInt(meterToFeet(type->getMinPosition().getAltitude())));
  bind(QStringLiteral(":min_lonx"), type->getMinPosition().getLonX());
  bind(QStringLiteral(":min_laty"), type->getMinPosition().getLatY());

  bind(QStringLiteral(":geometry"),
       atools::fs::common::BinaryGeometry(atools::fs::util::correctBoundary(fetchAirspaceLines(type))).writeToByteArray());
  executeStatement();
}

atools::geo::LineString BoundaryWriter::fetchAirspaceLines(const Boundary *type)
{
  // Related to full circle - 7.5° - number is checked in MapPainterAirspace::render()
  const static int CIRCLE_SEGMENTS = 48;

  const QList<bgl::BoundarySegment>& segments = type->getSegments();
  LineString processedLines;

  for(int i = 0; i < segments.size(); i++)
  {
    const bgl::BoundarySegment& segment = segments.at(i);

    if(segment.getType() == bl::ORIGIN)
      // Origin needed later
      continue;
    else if(segment.getType() == bl::CIRCLE)
      // Append line string build from circle parameters - one point every 5 degrees
      processedLines.append(LineString(segments.at(i - 1).getPosition(), segment.getRadius(), CIRCLE_SEGMENTS));
    else if(segment.getType() == bl::ARC_CCW || segment.getType() == bl::ARC_CW)
      // Build an arc
      processedLines.append(LineString(segments.at(i - 1).getPosition(),
                                       segments.at(i - 2).getPosition(),
                                       segments.at(i).getPosition(),
                                       segment.getType() == bl::ARC_CW, CIRCLE_SEGMENTS));
    else
      processedLines.append(segment.getPosition());
  }
  return processedLines;
}

} // namespace writer
} // namespace fs
} // namespace atools
