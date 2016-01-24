/*
 * Dme.h
 *
 *  Created on: 26.04.2015
 *      Author: alex
 */

#ifndef BGL_NAV_DME_H_
#define BGL_NAV_DME_H_

#include "../record.h"
#include "../bglposition.h"

#include <iosfwd>

namespace atools {
namespace fs {
namespace bgl {

class Dme :
  public Record
{
public:
  Dme(atools::io::BinaryStream *bs);
  virtual ~Dme();

  const BglPosition& getPosition() const
  {
    return position;
  }

  float getRange() const
  {
    return range;
  }

private:
  friend QDebug operator<<(QDebug out, const Dme& record);

  BglPosition position;
  float range;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_NAV_DME_H_ */
