/*
 * Ils.h
 *
 *  Created on: 25.04.2015
 *      Author: alex
 */

#ifndef BGL_NAV_ILS_H_
#define BGL_NAV_ILS_H_

#include "navbase.h"
#include "../bglposition.h"

namespace atools {
namespace fs {
namespace bgl {

class Localizer;
class Glideslope;
class Dme;

class Ils :
  public NavBase
{
public:
  Ils(atools::io::BinaryStream *bs);
  virtual ~Ils();

  const Dme *getDme() const
  {
    return dme;
  }

  const Glideslope *getGlideslope() const
  {
    return glideslope;
  }

  bool isIsBackcourse() const
  {
    return isBackcourse;
  }

  const Localizer *getLocalizer() const
  {
    return localizer;
  }

private:
  friend QDebug operator<<(QDebug out, const Ils& record);

  bool isBackcourse;

  Localizer *localizer;
  Glideslope *glideslope;
  Dme *dme;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_NAV_ILS_H_ */
