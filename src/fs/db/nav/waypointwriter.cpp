/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

#include <exception.h>

namespace atools {
namespace fs {
namespace db {

using atools::fs::bgl::Waypoint;

void WaypointWriter::writeObject(const Waypoint *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing Waypoint " << type->getIdent();

  if(type->getIdent().isEmpty())
  {
    if(getOptions().getSimulatorType() != atools::fs::FsPaths::MSFS)
      qWarning() << Q_FUNC_INFO << "Found waypoint with empty ident in file" << getDataWriter().getBglFileWriter()->getCurrentFilepath();
    return;
  }

  if(type->getRegion().isEmpty())
  {
    qWarning() << Q_FUNC_INFO << "Found waypoint" << type->getIdent() << "with empty region in file"
               << getDataWriter().getBglFileWriter()->getCurrentFilepath();
    return;
  }

  // One letter codes like "P" and "K" are valid and used for unnamed, charted intersections
  // The invalid code "KZ" can be removed
  if((getOptions().getSimulatorType() == atools::fs::FsPaths::MSFS_2024 || getOptions().getSimulatorType() == atools::fs::FsPaths::MSFS) &&
     type->getRegion() == "KZ" && /*type->getNumJetAirway() == 0 && type->getNumVictorAirway() == 0 && */ type->getAirportIdent().isEmpty())
  {
    qWarning() << Q_FUNC_INFO << "Found invalid waypoint" << type->getIdent() << "with invalid region" << type->getRegion() << "in file"
               << getDataWriter().getBglFileWriter()->getCurrentFilepath();
    return;
  }

  QString waypointType = bgl::Waypoint::waypointTypeToStr(type->getType());
  if(waypointType.isEmpty())
  {
    qWarning() << Q_FUNC_INFO << "Found waypoint" << type->getIdent() << "with invalid type" << type->getType() << "in file"
               << getDataWriter().getBglFileWriter()->getCurrentFilepath();
    return;
  }

  bind(":waypoint_id", getNextId());
  bind(":file_id", getDataWriter().getBglFileWriter()->getCurrentId());
  bindNullInt(":nav_id");
  bind(":ident", type->getIdent());
  bind(":region", type->getRegion());
  bind(":type", waypointType);

  bindNullInt(":airport_id");
  bind(":airport_ident", type->getAirportIdent());

  if((type->getType() == bgl::nav::NDB || type->getType() == bgl::nav::VOR))
    bind(":artificial", 1);
  else
    bindNullInt(":artificial");

  bind(":num_victor_airway", type->getNumVictorAirway());
  bind(":num_jet_airway", type->getNumJetAirway());
  bind(":mag_var", getDataWriter().getMagVar(type->getPosition().getPos(), type->getMagVar()));
  bind(":lonx", type->getPosition().getLonX());
  bind(":laty", type->getPosition().getLatY());

  executeStatement();

  AirwaySegmentWriter *tempAirwayWriter = getDataWriter().getAirwaySegmentWriter();
  tempAirwayWriter->write(type->getAirways());
}

} // namespace writer
} // namespace fs
} // namespace atools
