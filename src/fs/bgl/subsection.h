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

#ifndef BGL_SUBSECTION_H_
#define BGL_SUBSECTION_H_

#include "fs/bgl/bglbase.h"

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {

class Section;

class Subsection :
  public atools::fs::bgl::BglBase
{
public:
  Subsection(atools::io::BinaryStream *bs, const Section& parentSect);
  virtual ~Subsection();

  int getDataSize() const
  {
    return dataSize;
  }

  int getFirstDataRecordOffset() const
  {
    return firstDataRecordOffset;
  }

  int getId() const
  {
    return id;
  }

  int getNumDataRecords() const
  {
    return numDataRecords;
  }

  const Section& getParent() const
  {
    return *parent;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Subsection& section);

  int id, numDataRecords, firstDataRecordOffset, dataSize;
  const atools::fs::bgl::Section *parent;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_SUBSECTION_H_ */
