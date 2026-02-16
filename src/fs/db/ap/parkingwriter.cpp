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

#include "fs/db/ap/parkingwriter.h"
#include "fs/db/datawriter.h"
#include "fs/bgl/util.h"
#include "fs/navdatabaseoptions.h"
#include "fs/db/ap/airportwriter.h"
#include "geo/calculations.h"
#include "atools.h"

namespace atools {
namespace fs {
namespace db {

using atools::fs::bgl::Parking;
using atools::geo::meterToFeet;

void ParkingWriter::writeObject(const Parking *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing Parking for airport "
             << getDataWriter().getAirportWriter()->getCurrentAirportIdent();

  bind(QStringLiteral(":parking_id"), getNextId());
  bind(QStringLiteral(":airport_id"), getDataWriter().getAirportWriter()->getCurrentId());
  bind(QStringLiteral(":type"), bgl::util::enumToStr(Parking::parkingTypeToStr, type->getType()));
  bind(QStringLiteral(":suffix"), bgl::util::enumToStr(Parking::parkingSuffixToStr, type->getSuffix()));
  bind(QStringLiteral(":pushback"), bgl::util::enumToStr(Parking::pushBackToStr, type->getPushBack()));
  bind(QStringLiteral(":name"), Parking::parkingNameToStr(type->getName())); // Also allow NONE and UNKNOWN
  bind(QStringLiteral(":number"), type->getNumber());
  bind(QStringLiteral(":airline_codes"), type->getAirlineCodes().join(QStringLiteral(",")));
  bind(QStringLiteral(":radius"), roundToInt(meterToFeet(type->getRadius())));
  bind(QStringLiteral(":heading"), type->getHeading());
  bindBool(QStringLiteral(":has_jetway"), type->hasJetway());
  bind(QStringLiteral(":lonx"), type->getPosD().getLonX());
  bind(QStringLiteral(":laty"), type->getPosD().getLatY());

  executeStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
