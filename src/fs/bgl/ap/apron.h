/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_BGL_AP_APRON_H
#define ATOOLS_BGL_AP_APRON_H

#include "fs/bgl/record.h"
#include "fs/bgl/ap/rw/runway.h"
#include "fs/bgl/bglposition.h"

namespace atools {
namespace fs {
namespace bgl {

/*
 *  First of two apron records. Subrecord of airport. This one contains the boundary.
 */
class Apron :
  public atools::fs::bgl::Record
{
public:
  Apron(const atools::fs::NavDatabaseOptions *options, atools::io::BinaryStream *bs);
  virtual ~Apron();

  atools::fs::bgl::rw::Surface getSurface() const
  {
    return surface;
  }

  /*
   * @return Apron boundary vertices
   */
  const QList<atools::fs::bgl::BglPosition>& getVertices() const
  {
    return vertices;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Apron& record);

  atools::fs::bgl::rw::Surface surface = atools::fs::bgl::rw::UNKNOWN;
  QList<atools::fs::bgl::BglPosition> vertices;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_AP_APRON_H
