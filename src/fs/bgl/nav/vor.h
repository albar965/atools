/*
 * Vor.h
 *
 *  Created on: 25.04.2015
 *      Author: alex
 */

#ifndef BGL_NAV_VOR_H_
#define BGL_NAV_VOR_H_

#include "navbase.h"
#include "../bglposition.h"
#include "ilsvor.h"

namespace atools {
namespace fs {
namespace bgl {

class Dme;

class Vor :
  public NavBase
{
public:
  Vor(atools::io::BinaryStream *bs);
  virtual ~Vor();

  const Dme *getDme() const
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
  friend QDebug operator<<(QDebug out, const Vor& record);

  atools::fs::bgl::nav::IlsVorType type;
  bool dmeOnly;
  Dme *dme;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_NAV_VOR_H_ */
