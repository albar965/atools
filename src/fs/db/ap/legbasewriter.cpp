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

#include "fs/db/ap/legbasewriter.h"

#include "atools.h"
#include "fs/bgl/util.h"
#include "fs/db/datawriter.h"
#include "fs/db/meta/bglfilewriter.h"
#include "geo/calculations.h"

namespace atools {
namespace fs {
namespace db {

using atools::fs::bgl::ApproachLeg;

LegBaseWriter::LegBaseWriter(sql::SqlDatabase& db, DataWriter& dataWriter, const QLatin1String& table)
  : WriterBase(db, dataWriter, table)
{
}

void LegBaseWriter::writeObject(const ApproachLeg *type)
{
  // id and leg_id have to be bound by the caller

  QString typeStr = bgl::util::enumToStr(bgl::ApproachLeg::legTypeToString, type->getType());
  if(typeStr.isEmpty())
  {
    // Should not happen since this is filtered out before
    qWarning() << Q_FUNC_INFO << "Invalid approach leg type. Skipping."
               << getDataWriter().getBglFileWriter()->getCurrentFilepath();
    return;
  }

  bind(QStringLiteral(":type"), typeStr);
  bind(QStringLiteral(":alt_descriptor"),
       bgl::util::enumToStr(bgl::ApproachLeg::altDescriptorToString, type->getAltDescriptor()));
  bind(QStringLiteral(":turn_direction"), bgl::util::enumToStr(bgl::ApproachLeg::turnDirToString, type->getTurnDirection()));
  bind(QStringLiteral(":fix_type"), bgl::util::enumToStr(bgl::ap::approachFixTypeToStr, type->getFixType()));
  bind(QStringLiteral(":fix_ident"), type->getFixIdent());
  bind(QStringLiteral(":fix_region"), type->getFixRegion());
  bind(QStringLiteral(":fix_airport_ident"), type->getFixAirportIdent());
  bind(QStringLiteral(":recommended_fix_type"),
       bgl::util::enumToStr(bgl::ap::approachFixTypeToStr, type->getRecommendedFixType()));
  bind(QStringLiteral(":recommended_fix_ident"), type->getRecommendedFixIdent());
  bind(QStringLiteral(":recommended_fix_region"), type->getRecommendedFixRegion());
  bindBool(QStringLiteral(":is_flyover"), type->isFlyover());
  bindBool(QStringLiteral(":is_true_course"), type->isTrueCourse());
  bind(QStringLiteral(":course"), type->getCourse());

  if(type->isTime())
  {
    bind(QStringLiteral(":time"), type->getDistOrTime());
    bindNullFloat(QStringLiteral(":distance"));
  }
  else
  {
    bind(QStringLiteral(":distance"), roundToInt(atools::geo::meterToNm(type->getDistOrTime())));
    bindNullFloat(QStringLiteral(":time"));
  }

  bind(QStringLiteral(":theta"), type->getTheta());
  bind(QStringLiteral(":rho"), atools::geo::meterToNm(type->getRho()));
  bind(QStringLiteral(":altitude1"), roundToInt(atools::geo::meterToFeet(type->getAltitude1())));
  bind(QStringLiteral(":altitude2"), roundToInt(atools::geo::meterToFeet(type->getAltitude2())));

  if(type->getSpeedLimit() > 10.f)
  {
    bind(QStringLiteral(":speed_limit"), roundToInt(type->getSpeedLimit()));
    bind(QStringLiteral(":speed_limit_type"), QStringLiteral("-")); // Assume limit for maximum speed - type is not given in BGL
  }
  else
  {
    bindNullInt(QStringLiteral(":speed_limit"));
    bindNullString(QStringLiteral(":speed_limit_type"));
  }

  if(std::abs(type->getVerticalAngle()) > 0.f)
    bind(QStringLiteral(":vertical_angle"), -type->getVerticalAngle() / 100.f);
  else
    bindNullInt(QStringLiteral(":vertical_angle"));

  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
