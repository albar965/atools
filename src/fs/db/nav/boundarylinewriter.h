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

#ifndef ATOOLS_FS_DB_BOUNDARYLINEWRITER_H
#define ATOOLS_FS_DB_BOUNDARYLINEWRITER_H

#include "fs/db/writerbase.h"
#include "fs/bgl/boundarysegment.h"

namespace atools {
namespace fs {
namespace db {

class BoundaryLineWriter :
  public atools::fs::db::WriterBase<atools::fs::bgl::BoundarySegment>
{
public:
  BoundaryLineWriter(atools::sql::SqlDatabase& db, atools::fs::db::DataWriter& dataWriter)
    : WriterBase(db, dataWriter, "boundary_line")
  {
  }

  virtual ~BoundaryLineWriter()
  {
  }

protected:
  virtual void writeObject(const atools::fs::bgl::BoundarySegment *type) override;

};

} // namespace writer
} // namespace fs
} // namespace atools

#endif /* ATOOLS_FS_DB_BOUNDARYLINEWRITER_H */
