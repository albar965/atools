/*
 * RunwayEndWriter.cpp
 *
 *  Created on: 01.05.2015
 *      Author: alex
 */

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
