/*
 * Record.h
 *
 *  Created on: 19.04.2015
 *      Author: alex
 */

#ifndef BGL_RECORD_H_
#define BGL_RECORD_H_

#include "bglbase.h"
#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {

class Subsection;

class Record :
  public BglBase
{
public:
  Record()
    : id(0), size(0)
  {
  }

  Record(atools::io::BinaryStream *bs);

  virtual ~Record();

  void seekToEnd() const
  {
    bs->seekg(startOffset + size);
  }

  int getSize() const
  {
    return size;
  }

  template<typename ENUM>
  ENUM getId() const
  {
    return static_cast<ENUM>(id);
  }

protected:
  friend QDebug operator<<(QDebug out, const Record& record);

  int id, size;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_RECORD_H_ */
