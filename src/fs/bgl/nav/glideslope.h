/*
 * Glideslope.h
 *
 *  Created on: 26.04.2015
 *      Author: alex
 */

#ifndef BGL_NAV_GLIDESLOPE_H_
#define BGL_NAV_GLIDESLOPE_H_

#include "fs/bgl/record.h"
#include "fs/bgl/bglposition.h"

namespace atools {
namespace fs {
namespace bgl {

class Glideslope :
  public atools::fs::bgl::Record
{
public:
  Glideslope(atools::io::BinaryStream *bs);
  virtual ~Glideslope();

  float getPitch() const
  {
    return pitch;
  }

  const atools::fs::bgl::BglPosition& getPosition() const
  {
    return position;
  }

  float getRange() const
  {
    return range;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Glideslope& record);

  atools::fs::bgl::BglPosition position;
  float range, pitch;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_NAV_GLIDESLOPE_H_ */
