/*
 * Record.h
 *
 *  Created on: 19.04.2015
 *      Author: alex
 */

#ifndef BGL_RECORD_H_
#define BGL_RECORD_H_

#include "fs/bgl/bglbase.h"

namespace atools {
namespace io {
class BinaryStream;
}

namespace fs {
namespace bgl {

class Subsection;

class Record :
  public atools::fs::bgl::BglBase
{
public:
  Record()
    : id(0), size(0)
  {
  }

  Record(atools::io::BinaryStream *bs);

  virtual ~Record();

  void seekToEnd() const;

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
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Record& record);

  int id, size;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_RECORD_H_ */
