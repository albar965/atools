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

#ifndef WRITER_HELIPADWRITER_H_
#define WRITER_HELIPADWRITER_H_

#include "fs/writer/writerbase.h"
#include "fs/bgl/ap/helipad.h"

namespace atools {
namespace fs {
namespace writer {

class HelipadWriter :
  public atools::fs::writer::WriterBase<atools::fs::bgl::Helipad>
{
public:
  HelipadWriter(atools::sql::SqlDatabase& db, atools::fs::writer::DataWriter& dataWriter)
    : WriterBase(db, dataWriter, "helipad")
  {
  }

  virtual ~HelipadWriter()
  {
  }

protected:
  virtual void writeObject(const atools::fs::bgl::Helipad *type)override;

};

} // namespace writer
} // namespace fs
} // namespace atools

#endif /* WRITER_HELIPADWRITER_H_ */
