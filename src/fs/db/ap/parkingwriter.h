/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_FS_DB_PARKINGWRITER_H
#define ATOOLS_FS_DB_PARKINGWRITER_H

#include "fs/db/writerbase.h"
#include "fs/bgl/ap/parking.h"

namespace atools {
namespace fs {
namespace db {

class ParkingWriter :
  public atools::fs::db::WriterBase<atools::fs::bgl::Parking>
{
public:
  ParkingWriter(atools::sql::SqlDatabase& db, atools::fs::db::DataWriter& dataWriter)
    : WriterBase(db, dataWriter, "parking")
  {
  }

  virtual ~ParkingWriter()
  {
  }

protected:
  virtual void writeObject(const atools::fs::bgl::Parking *type) override;

};

} // namespace writer
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_DB_PARKINGWRITER_H
