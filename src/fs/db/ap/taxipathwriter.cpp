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

#include "fs/db/ap/taxipathwriter.h"

#include "atools.h"
#include "fs/bgl/ap/rw/runway.h"
#include "fs/bgl/ap/taxipoint.h"
#include "fs/bgl/surface.h"
#include "fs/bgl/util.h"
#include "fs/db/ap/airportwriter.h"
#include "fs/db/datawriter.h"
#include "fs/navdatabaseoptions.h"
#include "geo/calculations.h"

namespace atools {
namespace fs {
namespace db {

using atools::fs::bgl::TaxiPath;
using atools::fs::bgl::TaxiPoint;
using atools::fs::bgl::Runway;
using atools::geo::meterToFeet;

void TaxiPathWriter::writeObject(const TaxiPath *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing TaxiPath for airport "
             << getDataWriter().getAirportWriter()->getCurrentAirportIdent();

  bind(QStringLiteral(":taxi_path_id"), getNextId());
  bind(QStringLiteral(":airport_id"), getDataWriter().getAirportWriter()->getCurrentId());
  bind(QStringLiteral(":type"), TaxiPath::pathTypeToString(type->getType()));

  if(!type->getMaterialUuid().isNull())
    bind(QStringLiteral(":surface"), atools::fs::bgl::surface::surfaceToDbStr(getDataWriter().getSurface(type->getMaterialUuid())));
  else
    bind(QStringLiteral(":surface"), atools::fs::bgl::surface::surfaceToDbStr(type->getSurface()));

  bind(QStringLiteral(":width"), roundToInt(meterToFeet(type->getWidth())));
  bind(QStringLiteral(":name"), type->getName());

  bindBool(QStringLiteral(":is_draw_surface"), type->isDrawSurface());
  bindBool(QStringLiteral(":is_draw_detail"), type->isDrawDetail());

  bind(QStringLiteral(":start_type"), bgl::util::enumToStr(TaxiPoint::pointTypeToString, type->getStartPoint().getType()));
  bind(QStringLiteral(":start_dir"), bgl::util::enumToStr(TaxiPoint::dirToString, type->getStartPoint().getOrientation()));
  bind(QStringLiteral(":start_lonx"), type->getStartPoint().getPosD().getLonX());
  bind(QStringLiteral(":start_laty"), type->getStartPoint().getPosD().getLatY());

  bind(QStringLiteral(":end_type"), bgl::util::enumToStr(TaxiPoint::pointTypeToString, type->getEndPoint().getType()));
  bind(QStringLiteral(":end_dir"), bgl::util::enumToStr(TaxiPoint::dirToString, type->getEndPoint().getOrientation()));
  bind(QStringLiteral(":end_lonx"), type->getEndPoint().getPosD().getLonX());
  bind(QStringLiteral(":end_laty"), type->getEndPoint().getPosD().getLatY());

  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
