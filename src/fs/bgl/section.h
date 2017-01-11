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

/*
 * BGL Section which contains a type and an offset to its subsections
 */
class Section :
  public atools::fs::bgl::BglBase
{
public:
  /*
   * Read the section (not including the subsections)
   */
  Section(const NavDatabaseOptions *options, atools::io::BinaryStream *bs);
  virtual ~Section();

  /*
   * @return position in the file where the first subsection starts.
   */
  unsigned int getFirstSubsectionOffset() const
  {
    return firstSubsectionOffset;
  }

  /*
   * @return Number of subsections in this section
   */
  unsigned int getNumSubsections() const
  {
    return numSubsections;
  }

  /*
   * @return size of this section
   */
  unsigned int getSubsectionSize() const
  {
    return subsectionSize;
  }

  /*
   * @return Total size of all subsections in this section
   */
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
  unsigned int subsectionSize, numSubsections, firstSubsectionOffset, totalSubsectionSize;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_SECTION_H
