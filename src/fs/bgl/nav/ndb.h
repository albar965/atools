/*
 * Ndb.h
 *
 *  Created on: 25.04.2015
 *      Author: alex
 */

#ifndef BGL_NAV_NDB_H_
#define BGL_NAV_NDB_H_

#include "fs/bgl/nav/navbase.h"

namespace atools {
namespace fs {
namespace bgl {

namespace nav {

enum NdbType
{
  COMPASS_POINT = 0,
  MH = 1,
  H = 2,
  HH = 3
};

} // namespace nav

class Ndb :
  public atools::fs::bgl::NavBase
{
public:
  Ndb(atools::io::BinaryStream *bs);
  virtual ~Ndb();

  atools::fs::bgl::nav::NdbType getType() const
  {
    return type;
  }

  static QString ndbTypeToStr(atools::fs::bgl::nav::NdbType type);

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Ndb& record);

  atools::fs::bgl::nav::NdbType type;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_NAV_NDB_H_ */
