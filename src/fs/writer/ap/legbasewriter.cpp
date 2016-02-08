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

#include "fs/writer/ap/legbasewriter.h"
#include "fs/writer/datawriter.h"
#include "fs/bgl/util.h"
#include "fs/bglreaderoptions.h"

namespace atools {
namespace fs {
namespace writer {

using atools::fs::bgl::ApproachLeg;
using atools::sql::SqlQuery;

void LegBaseWriter::writeObject(const ApproachLeg *type)
{
  bind(":type", bgl::util::enumToStr(bgl::ApproachLeg::legTypeToString, type->getType()));
  bind(":alt_descriptor",
       bgl::util::enumToStr(bgl::ApproachLeg::altDescriptorToString, type->getAltDescriptor()));
  bind(":turn_direction", bgl::util::enumToStr(bgl::ApproachLeg::turnDirToString, type->getTurnDirection()));
  bindNullInt(":fix_nav_id");
  bind(":fix_type", bgl::util::enumToStr(bgl::ap::approachFixTypeToStr, type->getFixType()));
  bind(":fix_ident", type->getFixIdent());
  bind(":fix_region", type->getFixRegion());
  bind(":fix_airport_ident", type->getFixAirportIdent());
  bindNullInt(":recommended_fix_nav_id");
  bind(":recommended_fix_type",
       bgl::util::enumToStr(bgl::ap::approachFixTypeToStr, type->getRecommendedFixType()));
  bind(":recommended_fix_ident", type->getRecommendedFixIdent());
  bind(":recommended_fix_region", type->getRecommendedFixRegion());
  bindBool(":is_flyover", type->isFlyover());
  bindBool(":is_true_course", type->isTrueCourse());
  bind(":course", type->getCourse());

  if(type->isTime())
  {
    bind(":time", type->getDistOrTime());
    bindNullFloat(":distance");
  }
  else
  {
    bind(":distance", type->getDistOrTime());
    bindNullFloat(":time");
  }

  bind(":theta", type->getTheta());
  bind(":rho", type->getRho());
  bind(":altitude1", bgl::util::meterToFeet(type->getAltitude1()));
  bind(":altitude2", bgl::util::meterToFeet(type->getAltitude2()));

  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
