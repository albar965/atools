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

#include "fs/writer/nav/waypointwriter.h"
#include "fs/writer/nav/temproutewriter.h"
#include "fs/writer/meta/bglfilewriter.h"
#include "fs/writer/datawriter.h"
#include "fs/writer/airportindex.h"

namespace atools {
namespace fs {
namespace writer {

using atools::fs::bgl::Waypoint;
using atools::sql::SqlQuery;

void WaypointWriter::writeObject(const Waypoint *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing Waypoint " << type->getIdent();

  bind(":waypoint_id", getNextId());
  bind(":file_id", getDataWriter().getBglFileWriter()->getCurrentId());
  bind(":ident", type->getIdent());
  bind(":region", type->getRegion());
  bind(":type", bgl::Waypoint::waypointTypeToStr(type->getType()));
  bind(":mag_var", type->getMagVar());
  bind(":lonx", type->getPosition().getLonX());
  bind(":laty", type->getPosition().getLatY());

  QString apIdent = type->getAirportIdent();
  if(!apIdent.isEmpty() && getOptions().includeAirport(apIdent))
  {
    QString msg("Waypoint ID " + QString::number(getCurrentId()) + " ident " + type->getIdent());
    int id = getAirportIndex()->getAirportId(apIdent, msg);
    if(id != -1)
      bind(":airport_id", id);
    else
      bind(":airport_id", QVariant(QVariant::Int));
  }

  executeStatement();

  TempRouteWriter *tempRouteWriter = getDataWriter().getTempRouteWriter();
  tempRouteWriter->write(type->getRoutes());
}

} // namespace writer
} // namespace fs
} // namespace atools
