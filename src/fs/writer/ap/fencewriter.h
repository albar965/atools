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

#ifndef WRITER_FENCEWRITER_H_
#define WRITER_FENCEWRITER_H_

#include "fs/writer/writerbase.h"
#include "fs/bgl/ap/fence.h"

namespace atools {
namespace fs {
namespace writer {

class FenceWriter :
  public atools::fs::writer::WriterBase<atools::fs::bgl::Fence>
{
public:
  FenceWriter(atools::sql::SqlDatabase& db, atools::fs::writer::DataWriter& dataWriter)
    : WriterBase(db, dataWriter, "fence")
  {
  }

  virtual ~FenceWriter()
  {
  }

protected:
  virtual void writeObject(const bgl::Fence *type) override;

};

} // namespace writer
} // namespace fs
} // namespace atools

#endif /* WRITER_FENCEWRITER_H_ */
