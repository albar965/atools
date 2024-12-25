/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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

#include "fs/db/ap/rw/runwayendwriter.h"

#include "fs/db//datawriter.h"
#include "fs/bgl/util.h"
#include "fs/navdatabaseoptions.h"
#include "fs/db/ap/airportwriter.h"
#include "geo/calculations.h"
#include  "atools.h"

namespace atools {
namespace fs {
namespace db {

using bgl::RunwayEnd;
using atools::geo::meterToFeet;

void RunwayEndWriter::writeObject(const RunwayEnd *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing Runway end " << type->getName() << " for airport "
             << getDataWriter().getAirportWriter()->getCurrentAirportIdent();

  bind(":runway_end_id", getNextId());
  bind(":name", type->getName());
  bind(":offset_threshold", roundToInt(meterToFeet(type->getOffsetThreshold())));
  bind(":blast_pad", roundToInt(meterToFeet(type->getBlastPad())));
  bind(":overrun", roundToInt(meterToFeet(type->getOverrun())));
  bind(":has_closed_markings", type->hasClosedMarkings());
  bind(":has_stol_markings", type->hasStolMarkings());
  bind(":is_takeoff", type->isTakeoff());
  bind(":is_landing", type->isLanding());
  bind(":is_pattern", bgl::RunwayEnd::patternToStr(type->getPattern()));
  bind(":app_light_system_type", bgl::util::enumToStr(bgl::RunwayApproachLights::appLightSystemToStr,
                                                      type->getApproachLights().getSystem()));
  bind(":has_end_lights", type->getApproachLights().hasEndlights());
  bind(":has_reils", type->getApproachLights().hasReils());
  bind(":has_touchdown_lights", type->getApproachLights().hasTouchdown());

  if(type->getIlsIdent().isEmpty())
    bindNullString(":ils_ident");
  else
    bind(":ils_ident", type->getIlsIdent());

  bind(":end_type", type->isPrimaryEnd() ? QLatin1String("P") : QLatin1String("S"));

  bind(":altitude", roundToInt(meterToFeet(type->getPosition().getAltitude())));
  bind(":lonx", type->getPosition().getLonX());
  bind(":laty", type->getPosition().getLatY());
  bind(":heading", type->getHeading());

  // Write left VASI if available - otherwise bind values to null
  QString leftVt = bgl::util::enumToStr(bgl::RunwayVasi::vasiTypeToStr, type->getLeftVasi().getType());
  if(!leftVt.isEmpty())
  {
    bind(":left_vasi_type", leftVt);
    bind(":left_vasi_pitch", type->getLeftVasi().getPitch());
  }
  else
  {
    bindNullString(":left_vasi_type");
    bindNullFloat(":left_vasi_pitch");
  }

  // Write right VASI if available - otherwise bind values to null
  QString rightVt = bgl::util::enumToStr(bgl::RunwayVasi::vasiTypeToStr, type->getRightVasi().getType());
  if(!rightVt.isEmpty())
  {
    bind(":right_vasi_type", rightVt);
    bind(":right_vasi_pitch", type->getRightVasi().getPitch());
  }
  else
  {
    bindNullString(":right_vasi_type");
    bindNullFloat(":right_vasi_pitch");
  }

  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
