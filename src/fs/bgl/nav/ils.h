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
  Ils(const atools::fs::BglReaderOptions *options, atools::io::BinaryStream *bs);
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
