/*
 * Header.h
 *
 *  Created on: 19.04.2015
 *      Author: alex
 */

#ifndef BGL_HEADER_H_
#define BGL_HEADER_H_

#include "fs/bgl/bglbase.h"

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {

class Header :
  public atools::fs::bgl::BglBase
{
public:
  Header()
    : magicNumber1(0), headerSize(0), lowDateTime(0), highDateTime(0), magicNumber2(0), numSections(0),
      creationTimestamp(0)
  {
  }

  Header(atools::io::BinaryStream *bs);
  virtual ~Header();

  QString getCreationTimestampString() const
  {
    return ctime(&creationTimestamp);
  }

  time_t getCreationTimestamp() const
  {
    return creationTimestamp;
  }

  int getNumSections() const
  {
    return numSections;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Header& header);

  int magicNumber1, headerSize, lowDateTime, highDateTime, magicNumber2, numSections;
  time_t creationTimestamp;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_HEADER_H_ */
