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

#include "fs/writer/ap/taxipathwriter.h"
#include "fs/writer/datawriter.h"
#include "fs/bgl/util.h"
#include "fs/bglreaderoptions.h"
#include "fs/writer/ap/airportwriter.h"
#include "fs/bgl/ap/rw/runway.h"
#include "fs/bgl/ap/taxipoint.h"

namespace atools {
namespace fs {
namespace writer {

using atools::fs::bgl::TaxiPath;
using atools::fs::bgl::TaxiPoint;
using atools::fs::bgl::Runway;
using atools::sql::SqlQuery;

void TaxiPathWriter::writeObject(const TaxiPath *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing TaxiPath for airport "
             << getDataWriter().getAirportWriter()->getCurrentAirportIdent();

  bind(":taxi_path_id", getNextId());
  bind(":airport_id", getDataWriter().getAirportWriter()->getCurrentId());
  bind(":type", TaxiPath::pathTypeToString(type->getType()));
  bind(":surface", Runway::surfaceToStr(type->getSurface()));
  bind(":width", bgl::util::meterToFeet(type->getWidth()));

  if(type->getWeightLimit() == 0)
    bindNullInt(":weight_limit");
  else
    bind(":weight_limit", type->getWeightLimit());

  bind(":name", type->getName());

  bind(":is_draw_surface", type->isDrawSurface() ? 1 : 0);
  bind(":is_draw_detail", type->isDrawDetail() ? 1 : 0);

  bind(":has_centerline", type->hasCenterline() ? 1 : 0);
  bind(":has_centerline_light", type->hasCenterlineLight() ? 1 : 0);
  bind(":has_left_edge_light", type->hasLeftEdgeLight() ? 1 : 0);
  bind(":has_right_edge_light", type->hasRightEdgeLight() ? 1 : 0);

  bind(":start_type", bgl::util::enumToStr(TaxiPoint::pointTypeToString, type->getStartPoint().getType()));
  bind(":start_dir", bgl::util::enumToStr(TaxiPoint::dirToString, type->getStartPoint().getDir()));
  bind(":start_lonx", type->getStartPoint().getPosition().getLonX());
  bind(":start_laty", type->getStartPoint().getPosition().getLatY());

  bind(":end_type", bgl::util::enumToStr(TaxiPoint::pointTypeToString, type->getEndPoint().getType()));
  bind(":end_dir", bgl::util::enumToStr(TaxiPoint::dirToString, type->getStartPoint().getDir()));
  bind(":end_lonx", type->getStartPoint().getPosition().getLonX());
  bind(":end_laty", type->getStartPoint().getPosition().getLatY());

  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
