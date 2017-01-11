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

#include "fs/db/ap/approachlegwriter.h"
#include "fs/db/datawriter.h"
#include "fs/db/ap/airportwriter.h"
#include "fs/db/ap/approachwriter.h"
#include "fs/bgl/util.h"
#include "fs/navdatabaseoptions.h"

#include <QDebug>

namespace atools {
namespace fs {
namespace db {

using atools::fs::bgl::ApproachLeg;

void ApproachLegWriter::writeObject(const ApproachLeg *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing approach leg for airport "
             << getDataWriter().getAirportWriter()->getCurrentAirportIdent();

  bind(":approach_leg_id", getNextId());
  bind(":approach_id", getDataWriter().getApproachWriter()->getCurrentId());
  bindBool(":is_missed", type->isMissed());

  LegBaseWriter::writeObject(type);
}

} // namespace writer
} // namespace fs
} // namespace atools
