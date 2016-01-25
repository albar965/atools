/*
 * Vor.h
 *
 *  Created on: 25.04.2015
 *      Author: alex
 */

#ifndef BGL_NAV_VOR_H_
#define BGL_NAV_VOR_H_

#include "fs/bgl/nav/navbase.h"
#include "fs/bgl/nav/ilsvor.h"

namespace atools {
namespace fs {
namespace bgl {

class Dme;

class Vor :
  public atools::fs::bgl::NavBase
{
public:
  Vor(atools::io::BinaryStream *bs);
  virtual ~Vor();

  const atools::fs::bgl::Dme *getDme() const
  {
    return dme;
  }

  bool isDmeOnly() const
  {
    return dmeOnly;
  }

  atools::fs::bgl::nav::IlsVorType getType() const
  {
    return type;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Vor& record);

  atools::fs::bgl::nav::IlsVorType type;
  bool dmeOnly;
  atools::fs::bgl::Dme *dme;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_NAV_VOR_H_ */
