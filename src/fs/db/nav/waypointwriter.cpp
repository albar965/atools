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

#include "fs/db/nav/waypointwriter.h"
#include "fs/db/nav/temproutewriter.h"
#include "fs/db/meta/bglfilewriter.h"
#include "fs/db/datawriter.h"
#include "fs/db/airportindex.h"

namespace atools {
namespace fs {
namespace db {

using atools::fs::bgl::Waypoint;
using atools::sql::SqlQuery;

void WaypointWriter::writeObject(const Waypoint *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing Waypoint " << type->getIdent();

  bind(":waypoint_id", getNextId());
  bind(":file_id", getDataWriter().getBglFileWriter()->getCurrentId());
  bindNullInt(":nav_id");
  bind(":ident", type->getIdent());
  bind(":region", type->getRegion());
  bind(":type", bgl::Waypoint::waypointTypeToStr(type->getType()));

  bind(":num_victor_route", type->getNumVictorRoute());
  bind(":num_jet_route", type->getNumJetRoute());

  bind(":mag_var", type->getMagVar());
  bind(":lonx", type->getPosition().getLonX());
  bind(":laty", type->getPosition().getLatY());

  bindNullInt(":airport_id");
  QString apIdent = type->getAirportIdent();
  if(!apIdent.isEmpty() && getOptions().includeAirport(apIdent))
  {
    QString msg("Waypoint ID " + QString::number(getCurrentId()) + " ident " + type->getIdent());
    int id = getAirportIndex()->getAirportId(apIdent, msg);
    if(id != -1)
      bind(":airport_id", id);
  }

  executeStatement();

  TempRouteWriter *tempRouteWriter = getDataWriter().getTempRouteWriter();
  tempRouteWriter->write(type->getRoutes());
}

} // namespace writer
} // namespace fs
} // namespace atools
