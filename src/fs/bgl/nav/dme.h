/*
 * Dme.h
 *
 *  Created on: 26.04.2015
 *      Author: alex
 */

#ifndef BGL_NAV_DME_H_
#define BGL_NAV_DME_H_

#include "fs/bgl/record.h"
#include "fs/bgl/bglposition.h"

#include <iosfwd>

namespace atools {
namespace fs {
namespace bgl {

class Dme :
  public atools::fs::bgl::Record
{
public:
  Dme(atools::io::BinaryStream *bs);
  virtual ~Dme();

  const atools::fs::bgl::BglPosition& getPosition() const
  {
    return position;
  }

  float getRange() const
  {
    return range;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Dme& record);

  atools::fs::bgl::BglPosition position;
  float range;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_NAV_DME_H_ */
