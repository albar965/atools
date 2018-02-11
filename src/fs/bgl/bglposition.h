/*****************************************************************************
* Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
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

#include <QDebug>

namespace atools {
namespace io {
class BinaryStream;
}
namespace fs {
namespace bgl {

/*
 * Reads position information from the binary stream.
 */
class BglPosition
{
public:
  BglPosition();

  /* Create position from coordinate parameters */
  BglPosition(float lonX, float latY, float altitude = 0.0f);

  /*
   * Reads position from the stream.
   * @param bs
   * @param hasAltitude true if there is an altitude saved in the binary data
   * @param altitudeDivisor Divide altitude by this value after readings
   */
  BglPosition(atools::io::BinaryStream *bs, bool hasAltitude = false, float altitudeDivisor = 1.f);
  ~BglPosition();

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

  bool isValid() const
  {
    return pos.isValid();
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::BglPosition& record);

  atools::geo::Pos pos;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_BGLPOSITION_H
