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

#ifndef ATOOLS_FS_DB_RUNWAYWRITER_H
#define ATOOLS_FS_DB_RUNWAYWRITER_H

#include "fs/db/writerbase.h"
#include "fs/bgl/ap/rw/runway.h"

namespace atools {
namespace fs {
namespace db {

/* Writes to runway and runway_end tables */
class RunwayWriter :
  public atools::fs::db::WriterBase<atools::fs::bgl::Runway>
{
public:
  RunwayWriter(atools::sql::SqlDatabase& db, atools::fs::db::DataWriter& dataWriter)
    : WriterBase(db, dataWriter, "runway")
  {
  }

  virtual ~RunwayWriter()
  {
  }

protected:
  virtual void writeObject(const atools::fs::bgl::Runway *type) override;

};

} // namespace writer
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_DB_RUNWAYWRITER_H
