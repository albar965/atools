/*
 * BglBase.h
 *
 *  Created on: 19.04.2015
 *      Author: alex
 */

#ifndef BGL_BGLBASE_H_
#define BGL_BGLBASE_H_

#include "logging/loggingdefs.h"


namespace atools {

namespace io {
class BinaryStream;
}

namespace fs {
namespace bgl {

class BglBase
{
public:
  virtual ~BglBase();

  void seekToStart();

  qint64 getStartOffset() const
  {
    return startOffset;
  }

protected:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::BglBase& base);

  BglBase();

  BglBase(atools::io::BinaryStream *stream);

  qint64 startOffset;

  atools::io::BinaryStream *bs;
};

} // namespace fs
} // namespace atools
} // namespace bgl

#endif /* BGL_BGLBASE_H_ */
