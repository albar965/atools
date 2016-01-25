/*
 * Pos.h
 *
 *  Created on: 21.04.2015
 *      Author: alex
 */

#ifndef BGL_BGLPOSITION_H_
#define BGL_BGLPOSITION_H_

#include "geo/pos.h"

#include "logging/loggingdefs.h"


namespace atools {
namespace io {
class BinaryStream;
}
namespace fs {
namespace bgl {

class BglPosition :
  public atools::geo::Pos
{
public:
  BglPosition()
    : geo::Pos(), altitude(0)
  {
  }

  BglPosition(float lonX, float latY, float alt = 0.0f)
    : geo::Pos(lonX, latY), altitude(alt)
  {
  }

  BglPosition(atools::io::BinaryStream *bs, float altitudeFactor = 1.f, bool hasAltitude = true);

  virtual ~BglPosition();

  float getAltitude() const
  {
    return altitude;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::BglPosition& pos);

  float altitude;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_BGLPOSITION_H_ */
