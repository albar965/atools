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
  Vor(const atools::fs::BglReaderOptions *options, atools::io::BinaryStream *bs);
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
