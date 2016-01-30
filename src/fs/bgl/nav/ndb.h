/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

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
  Ndb(const atools::fs::BglReaderOptions *options, atools::io::BinaryStream *bs);
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
