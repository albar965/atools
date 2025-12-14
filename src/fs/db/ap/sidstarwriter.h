/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_FS_DB_SIDSTARWRITER_H
#define ATOOLS_FS_DB_SIDSTARWRITER_H

#include "fs/bgl/ap/sidstar.h"
#include "fs/db/writerbase.h"
#include "fs/db/ap/approachwriter.h"
#include "fs/db/datawriter.h"

namespace atools {
namespace fs {
namespace db {

/*
 * Write an arrival/departure and all legs and transitions as multiple approaches.
 */
class SidStarWriter :
  public atools::fs::db::WriterBase<atools::fs::bgl::SidStar>
{
public:
  SidStarWriter(atools::sql::SqlDatabase& db, atools::fs::db::DataWriter& dataWriter)
    : WriterBase(db, dataWriter, "approach")
  {
  }

  const QString& getCurrentApproachName() const
  {
    return currentName;
  }

  const atools::fs::bgl::SidStar *getCurrentSidStar() const
  {
    return currentSidStar;
  }

  // NOTE: Since this writer shares the same table as the normal ApproachWriter,
  // we need to keep their IDs in sync. This feels terrible, but we're going to pull
  // our rowid from the ApproachWriter's object...
  int getCurrentId() const
  {
    return const_cast<SidStarWriter *>(this)->getDataWriter().getApproachWriter()->getCurrentId();
  }

  int getNextId()
  {
    return getDataWriter().getApproachWriter()->getNextId();
  }

  int setCurrentId(int value) const
  {
    return const_cast<SidStarWriter *>(this)->getDataWriter().getApproachWriter()->setCurrentId(value);
  }

protected:
  virtual void writeObject(const atools::fs::bgl::SidStar *type) override;

private:
  void writeDeparture(const atools::fs::bgl::SidStar *type,
                      const QList<atools::fs::bgl::ApproachLeg> *runwayTransLegs);

  void writeArrival(const atools::fs::bgl::SidStar *type,
                    const QList<atools::fs::bgl::ApproachLeg> *runwayTransLegs);

  const atools::fs::bgl::SidStar *currentSidStar = nullptr;
  QString currentName;
};

} // namespace db
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_DB_SIDSTARWRITER_H
