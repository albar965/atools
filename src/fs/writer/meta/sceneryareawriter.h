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

#ifndef WRITER_SCENERYAREAWRITER_H_
#define WRITER_SCENERYAREAWRITER_H_

#include "fs/writer/writerbase.h"
#include "fs/scenery/sceneryarea.h"

namespace atools {
namespace fs {
namespace writer {

class SceneryAreaWriter :
  public atools::fs::writer::WriterBase<scenery::SceneryArea>
{
public:
  SceneryAreaWriter(atools::sql::SqlDatabase& db, atools::fs::writer::DataWriter& dataWriter)
    : WriterBase(db, dataWriter, "scenery_area")
  {
  }

  virtual ~SceneryAreaWriter()
  {
  }

protected:
  virtual void writeObject(const scenery::SceneryArea *type)override;

};

} // namespace writer
} // namespace fs
} // namespace atools

#endif /* WRITER_SCENERYAREAWRITER_H_ */
