/*
 * SceneryException.h
 *
 *  Created on: 30.03.2015
 *      Author: alex
 */

#ifndef BGL_BGLEXCEPTION_H_
#define BGL_BGLEXCEPTION_H_

#include "exception.h"

#include <QString>

namespace atools {
namespace io {
class BinaryStream;
}

namespace fs {
namespace bgl {

class BglException :
  public atools::Exception
{
public:
  BglException(const QString& msg)
    : Exception(msg)
  {
  }

  BglException(const QString& msg, const atools::io::BinaryStream *bs);

  virtual ~BglException() noexcept;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_BGLEXCEPTION_H_ */
