/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_FS_FB_SIDSTARAPPROACHLEGWRITER_H
#define ATOOLS_FS_FB_SIDSTARAPPROACHLEGWRITER_H

#include "fs/db/ap/legbasewriter.h"
#include "fs/bgl/ap/approachleg.h"
#include "fs/db/ap/approachlegwriter.h"
#include "fs/db/datawriter.h"

namespace atools {
namespace fs {
namespace db {

class SidStarApproachLegWriter :
  public atools::fs::db::LegBaseWriter
{
public:
  SidStarApproachLegWriter(atools::sql::SqlDatabase& db, atools::fs::db::DataWriter& dataWriter)
    : LegBaseWriter(db, dataWriter, "approach_leg")
  {
  }

  // NOTE: Since this writer shares the same table as the normal ApproachLegWriter,
  // we need to keep their IDs in sync. This feels terrible, but we're going to pull
  // our rowid from the ApproachLegWriter's object...
  int getCurrentId() const
  {
    return const_cast<SidStarApproachLegWriter *>(this)->getDataWriter().getApproachLegWriter()->getCurrentId();
  }

  int getNextId()
  {
    return getDataWriter().getApproachLegWriter()->getNextId();
  }

  int setCurrentId(int value) const
  {
    return const_cast<SidStarApproachLegWriter *>(this)->getDataWriter().getApproachLegWriter()->setCurrentId(value);
  }

protected:
  virtual void writeObject(const atools::fs::bgl::ApproachLeg *type) override;

};

} // namespace db
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_FB_SIDSTARAPPROACHLEGWRITER_H
