/*
 * NavBase.h
 *
 *  Created on: 27.04.2015
 *      Author: alex
 */

#ifndef BGL_NAV_NAVBASE_H_
#define BGL_NAV_NAVBASE_H_

#include "fs/bgl/bglposition.h"
#include "fs/bgl/record.h"
#include <QString>

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {

class NavBase :
  public atools::fs::bgl::Record
{
public:
  NavBase(atools::io::BinaryStream *bs)
    : Record(bs), frequency(0.0f), range(0.0f), magVar(0.0f)
  {
  }

  virtual ~NavBase();

  const QString& getAirportIdent() const
  {
    return airportIdent;
  }

  int getFrequency() const
  {
    return frequency;
  }

  const QString& getIdent() const
  {
    return ident;
  }

  float getMagVar() const
  {
    return magVar;
  }

  const QString& getName() const
  {
    return name;
  }

  const atools::fs::bgl::BglPosition& getPosition() const
  {
    return position;
  }

  float getRange() const
  {
    return range;
  }

  const QString& getRegion() const
  {
    return region;
  }

protected:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::NavBase& record);

  atools::fs::bgl::BglPosition position;
  int frequency;
  float range, magVar;
  QString ident, region, airportIdent, name;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_NAV_NAVBASE_H_ */
