/*
 * BglBase.h
 *
 *  Created on: 19.04.2015
 *      Author: alex
 */

#ifndef BGL_BGLBASE_H_
#define BGL_BGLBASE_H_

#include "io/binarystream.h"

#include <QDebug>

namespace atools {
namespace fs {
namespace bgl {

class BglBase
{
public:
  virtual ~BglBase();

  void seekToStart()
  {
    bs->seekg(startOffset);
  }

  qint64 getStartOffset() const
  {
    return startOffset;
  }

protected:
  friend QDebug operator<<(QDebug out, const BglBase& base);

  BglBase()
    : startOffset(0), bs(nullptr)
  {
  }

  BglBase(atools::io::BinaryStream *stream)
    : startOffset(stream->tellg()), bs(stream)
  {
  }

  qint64 startOffset;

  atools::io::BinaryStream *bs;
};

} // namespace fs
} // namespace atools
} // namespace bgl

#endif /* BGL_BGLBASE_H_ */
