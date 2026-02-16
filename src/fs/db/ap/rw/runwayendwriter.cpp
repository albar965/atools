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

#include "fs/db/ap/rw/runwayendwriter.h"

#include "atools.h"
#include "fs/bgl/util.h"
#include "fs/db//datawriter.h"
#include "fs/db/ap/airportwriter.h"
#include "fs/navdatabaseoptions.h"
#include "geo/calculations.h"

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

  bind(QStringLiteral(":runway_end_id"), getNextId());
  bind(QStringLiteral(":name"), type->getName());
  bind(QStringLiteral(":offset_threshold"), roundToInt(meterToFeet(type->getOffsetThreshold())));
  bind(QStringLiteral(":blast_pad"), roundToInt(meterToFeet(type->getBlastPad())));
  bind(QStringLiteral(":overrun"), roundToInt(meterToFeet(type->getOverrun())));
  bind(QStringLiteral(":has_closed_markings"), type->hasClosedMarkings());
  bind(QStringLiteral(":has_stol_markings"), type->hasStolMarkings());
  bind(QStringLiteral(":is_takeoff"), type->isTakeoff());
  bind(QStringLiteral(":is_landing"), type->isLanding());
  bind(QStringLiteral(":is_pattern"), bgl::RunwayEnd::patternToStr(type->getPattern()));
  bind(QStringLiteral(":app_light_system_type"), bgl::util::enumToStr(bgl::RunwayApproachLights::appLightSystemToStr,
                                                                      type->getApproachLights().getSystem()));
  bind(QStringLiteral(":has_end_lights"), type->getApproachLights().hasEndlights());
  bind(QStringLiteral(":has_reils"), type->getApproachLights().hasReils());
  bind(QStringLiteral(":has_touchdown_lights"), type->getApproachLights().hasTouchdown());

  if(type->getIlsIdent().isEmpty())
    bindNullString(QStringLiteral(":ils_ident"));
  else
    bind(QStringLiteral(":ils_ident"), type->getIlsIdent());

  bind(QStringLiteral(":end_type"), type->isPrimaryEnd() ? QStringLiteral("P") : QStringLiteral("S"));

  bind(QStringLiteral(":altitude"), roundToInt(meterToFeet(type->getPosition().getAltitude())));
  bind(QStringLiteral(":lonx"), type->getPosition().getLonX());
  bind(QStringLiteral(":laty"), type->getPosition().getLatY());
  bind(QStringLiteral(":heading"), type->getHeading());

  // Write left VASI if available - otherwise bind values to null
  QString leftVt = bgl::util::enumToStr(bgl::RunwayVasi::vasiTypeToStr, type->getLeftVasi().getType());
  if(!leftVt.isEmpty())
  {
    bind(QStringLiteral(":left_vasi_type"), leftVt);
    bind(QStringLiteral(":left_vasi_pitch"), type->getLeftVasi().getPitch());
  }
  else
  {
    bindNullString(QStringLiteral(":left_vasi_type"));
    bindNullFloat(QStringLiteral(":left_vasi_pitch"));
  }

  // Write right VASI if available - otherwise bind values to null
  QString rightVt = bgl::util::enumToStr(bgl::RunwayVasi::vasiTypeToStr, type->getRightVasi().getType());
  if(!rightVt.isEmpty())
  {
    bind(QStringLiteral(":right_vasi_type"), rightVt);
    bind(QStringLiteral(":right_vasi_pitch"), type->getRightVasi().getPitch());
  }
  else
  {
    bindNullString(QStringLiteral(":right_vasi_type"));
    bindNullFloat(QStringLiteral(":right_vasi_pitch"));
  }

  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
