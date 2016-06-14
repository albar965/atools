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

#ifndef ATOOLS_BGL_BGLPOSITION_H
#define ATOOLS_BGL_BGLPOSITION_H

#include "geo/pos.h"

#include "logging/loggingdefs.h"

namespace atools {
namespace io {
class BinaryStream;
}
namespace fs {
namespace bgl {

class BglPosition final
{
public:
  BglPosition();
  BglPosition(float lonX, float latY, float alt = 0.0f);
  BglPosition(atools::io::BinaryStream * bs, bool hasAltitude = false, float altitudeFactor = 1.f);

  float getLonX() const
  {
    return pos.getLonX();
  }

  float getLatY() const
  {
    return pos.getLatY();
  }

  float getAltitude() const
  {
    return pos.getAltitude();
  }

  const atools::geo::Pos& getPos() const
  {
    return pos;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::BglPosition& record);

  atools::geo::Pos pos;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_BGLPOSITION_H
