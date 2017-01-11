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

#include "fs/db/nav/waypointwriter.h"
#include "fs/db/nav/airwaysegmentwriter.h"
#include "fs/db/meta/bglfilewriter.h"
#include "fs/db/datawriter.h"
#include "fs/db/airportindex.h"

namespace atools {
namespace fs {
namespace db {

using atools::fs::bgl::Waypoint;

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

  bind(":num_victor_airway", type->getNumVictorAirway());
  bind(":num_jet_airway", type->getNumJetAirway());

  bind(":mag_var", type->getMagVar());
  bind(":lonx", type->getPosition().getLonX());
  bind(":laty", type->getPosition().getLatY());

  bindNullInt(":airport_id");
  QString apIdent = type->getAirportIdent();
  if(!apIdent.isEmpty() && getOptions().isIncludedAirportIdent(apIdent))
  {
    QString msg("Waypoint ID " + QString::number(getCurrentId()) + " ident " + type->getIdent());
    int id = getAirportIndex()->getAirportId(apIdent, msg);
    if(id != -1)
      bind(":airport_id", id);
  }

  executeStatement();

  AirwaySegmentWriter *tempAirwayWriter = getDataWriter().getAirwaySegmentWriter();
  tempAirwayWriter->write(type->getAirways());
}

} // namespace writer
} // namespace fs
} // namespace atools
