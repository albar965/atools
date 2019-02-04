/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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

#include "fs/db/ap/legbasewriter.h"
#include "fs/db/datawriter.h"
#include "fs/bgl/util.h"
#include "fs/navdatabaseoptions.h"
#include "geo/calculations.h"
#include "atools.h"
#include "fs/db/ap/airportwriter.h"

namespace atools {
namespace fs {
namespace db {

using atools::fs::bgl::ApproachLeg;

LegBaseWriter::LegBaseWriter(sql::SqlDatabase& db, DataWriter& dataWriter, const QString& table)
  : WriterBase(db, dataWriter, table)
{
}

LegBaseWriter::~LegBaseWriter()
{
}

void LegBaseWriter::writeObject(const ApproachLeg *type)
{
  // id and leg_id have to be bound by the caller

  bind(":type", bgl::util::enumToStr(bgl::ApproachLeg::legTypeToString, type->getType()));
  bind(":alt_descriptor",
       bgl::util::enumToStr(bgl::ApproachLeg::altDescriptorToString, type->getAltDescriptor()));
  bind(":turn_direction", bgl::util::enumToStr(bgl::ApproachLeg::turnDirToString, type->getTurnDirection()));
  bind(":fix_type", bgl::util::enumToStr(bgl::ap::approachFixTypeToStr, type->getFixType()));
  bind(":fix_ident", type->getFixIdent());
  bind(":fix_region", type->getFixRegion());
  bind(":fix_airport_ident", type->getFixAirportIdent());
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
    bind(":distance", roundToInt(atools::geo::meterToNm(type->getDistOrTime())));
    bindNullFloat(":time");
  }

  bind(":theta", type->getTheta());
  bind(":rho", atools::geo::meterToNm(type->getRho()));
  bind(":altitude1", roundToInt(atools::geo::meterToFeet(type->getAltitude1())));
  bind(":altitude2", roundToInt(atools::geo::meterToFeet(type->getAltitude2())));

  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
