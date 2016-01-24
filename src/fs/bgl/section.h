/*
 * Section.h
 *
 *  Created on: 19.04.2015
 *      Author: alex
 */

#ifndef BGL_SECTION_H_
#define BGL_SECTION_H_

#include "bglbase.h"
#include "sectiontype.h"

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {

class Section :
  public BglBase
{
public:
  Section(atools::io::BinaryStream *bs);
  virtual ~Section();

  int getFirstSubsectionOffset() const
  {
    return firstSubsectionOffset;
  }

  int getNumSubsections() const
  {
    return numSubsections;
  }

  int getSize() const
  {
    return size;
  }

  int getTotalSubsectionSize() const
  {
    return totalSubsectionSize;
  }

  atools::fs::bgl::section::SectionType getType() const
  {
    return type;
  }

private:
  friend QDebug operator<<(QDebug out, const Section& section);

  atools::fs::bgl::section::SectionType type;
  int size, numSubsections, firstSubsectionOffset, totalSubsectionSize;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_SECTION_H_ */
