/*
 * Glideslope.h
 *
 *  Created on: 26.04.2015
 *      Author: alex
 */

#ifndef BGL_NAV_GLIDESLOPE_H_
#define BGL_NAV_GLIDESLOPE_H_

#include "../record.h"
#include "../bglposition.h"

namespace atools {
namespace fs {
namespace bgl {

class Glideslope :
  public Record
{
public:
  Glideslope(atools::io::BinaryStream *bs);
  virtual ~Glideslope();

  float getPitch() const
  {
    return pitch;
  }

  const BglPosition& getPosition() const
  {
    return position;
  }

  float getRange() const
  {
    return range;
  }

private:
  friend QDebug operator<<(QDebug out, const Glideslope& record);

  BglPosition position;
  float range, pitch;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_NAV_GLIDESLOPE_H_ */
