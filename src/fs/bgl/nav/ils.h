/*
 * Ils.h
 *
 *  Created on: 25.04.2015
 *      Author: alex
 */

#ifndef BGL_NAV_ILS_H_
#define BGL_NAV_ILS_H_

#include "fs/bgl/nav/navbase.h"

namespace atools {
namespace fs {
namespace bgl {

class Localizer;
class Glideslope;
class Dme;

class Ils :
  public atools::fs::bgl::NavBase
{
public:
  Ils(atools::io::BinaryStream *bs);
  virtual ~Ils();

  const atools::fs::bgl::Dme *getDme() const
  {
    return dme;
  }

  const atools::fs::bgl::Glideslope *getGlideslope() const
  {
    return glideslope;
  }

  bool isIsBackcourse() const
  {
    return isBackcourse;
  }

  const atools::fs::bgl::Localizer *getLocalizer() const
  {
    return localizer;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Ils& record);

  bool isBackcourse;

  atools::fs::bgl::Localizer *localizer;
  atools::fs::bgl::Glideslope *glideslope;
  atools::fs::bgl::Dme *dme;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_NAV_ILS_H_ */
