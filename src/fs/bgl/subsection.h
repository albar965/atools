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

#ifndef ATOOLS_BGL_SUBSECTION_H
#define ATOOLS_BGL_SUBSECTION_H

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

/*
 * Subsection of a section contains the offset to the first data record
 */
class Subsection :
  public atools::fs::bgl::BglBase
{
public:
  /*
   * Read the subsection
   */
  explicit Subsection(const NavDatabaseOptions *options, atools::io::BinaryStream *stream, const Section& parentSect);
  virtual ~Subsection() override;

  /*
   * @return Size in bytes of this subsection's data (all records and subrecords)
   */
  int getDataSize() const
  {
    return dataSize;
  }

  /*
   * @return Offset of the first data record in this file
   */
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
  const atools::fs::bgl::Section *parent = nullptr;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_SUBSECTION_H
