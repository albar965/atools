/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_BGL_NAV_DME_H
#define ATOOLS_BGL_NAV_DME_H

#include "fs/bgl/record.h"
#include "fs/bgl/bglposition.h"

#include <iosfwd>

namespace atools {
namespace fs {
namespace bgl {

/*
 * DME station. This is a subrecord of ILS and VOR.
 */
class Dme :
  public atools::fs::bgl::Record
{
public:
  Dme(const atools::fs::NavDatabaseOptions *options, atools::io::BinaryStream *bs);
  virtual ~Dme();

  /*
   * @return Position of the DME transmitter
   */
  const atools::fs::bgl::BglPosition& getPosition() const
  {
    return position;
  }

  /*
   * @return Range of this DME station in meter.
   */
  float getRange() const
  {
    return range;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Dme& record);

  atools::fs::bgl::BglPosition position;
  float range;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_NAV_DME_H
