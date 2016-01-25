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

#include "fs/writer/ap/rw/runwayendwriter.h"
#include "fs/writer//datawriter.h"
#include "fs/bgl/util.h"
#include "fs/bglreaderoptions.h"
#include "fs/writer/ap/airportwriter.h"

namespace atools {
namespace fs {
namespace writer {

using bgl::RunwayEnd;
using atools::sql::SqlQuery;

void RunwayEndWriter::writeObject(const RunwayEnd *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing Runway end " << type->getName() << " for airport "
             << getDataWriter().getAirportWriter()->getCurrentAirportIdent();

  bind(":runway_end_id", getNextId());
  bind(":name", type->getName());
  bind(":offsetThreshold", bgl::util::meterToFeet(type->getOffsetThreshold()));
  bind(":blastPad", bgl::util::meterToFeet(type->getBlastPad()));
  bind(":overrun", bgl::util::meterToFeet(type->getOverrun()));
  bind(":has_closed_markings", type->hasClosedMarkings());
  bind(":has_stol_markings", type->hasStolMarkings());
  bind(":is_takeoff", type->isTakeoff());
  bind(":is_landing", type->isLanding());
  bind(":is_pattern", bgl::RunwayEnd::patternToStr(type->getPattern()));
  bind(":app_light_system_type",
       bgl::util::enumToStr(bgl::RunwayAppLights::appLightSystemToStr,
                            type->getApproachLights().getSystem()));
  bind(":has_end_lights", type->getApproachLights().hasEndlights());
  bind(":has_reils", type->getApproachLights().hasReils());
  bind(":has_touchdown_lights", type->getApproachLights().hasTouchdown());
  bind(":num_strobes", type->getApproachLights().getNumStrobes());

  QString leftVt = bgl::util::enumToStr(bgl::RunwayVasi::vasiTypeToStr, type->getLeftVasi().getType());
  if(!leftVt.isEmpty())
  {
    bind(":left_vasi_type", leftVt);
    bind(":left_vasi_pitch", type->getLeftVasi().getPitch());
  }

  QString rightVt = bgl::util::enumToStr(bgl::RunwayVasi::vasiTypeToStr, type->getRightVasi().getType());
  if(!rightVt.isEmpty())
  {
    bind(":right_vasi_type", rightVt);
    bind(":right_vasi_pitch", type->getRightVasi().getPitch());
  }

  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
