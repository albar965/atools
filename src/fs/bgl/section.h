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

#ifndef ATOOLS_BGL_SECTION_H
#define ATOOLS_BGL_SECTION_H

#include "fs/bgl/bglbase.h"
#include "fs/bgl/sectiontype.h"

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {

class Section :
  public atools::fs::bgl::BglBase
{
public:
  Section(const BglReaderOptions *options, atools::io::BinaryStream *bs);
  virtual ~Section();

  unsigned int getFirstSubsectionOffset() const
  {
    return firstSubsectionOffset;
  }

  unsigned int getNumSubsections() const
  {
    return numSubsections;
  }

  unsigned int getSize() const
  {
    return size;
  }

  unsigned int getTotalSubsectionSize() const
  {
    return totalSubsectionSize;
  }

  atools::fs::bgl::section::SectionType getType() const
  {
    return type;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Section& section);

  atools::fs::bgl::section::SectionType type;
  unsigned int size, numSubsections, firstSubsectionOffset, totalSubsectionSize;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_SECTION_H
