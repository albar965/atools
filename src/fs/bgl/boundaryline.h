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

#ifndef ATOOLS_BGL_AIRPORTBOUNDARYLINE_H
#define ATOOLS_BGL_AIRPORTBOUNDARYLINE_H

#include "fs/bgl/bglbase.h"
#include "fs/bgl/bglposition.h"

#include <QString>

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {

namespace boundaryline {
enum PointType
{
  UNKNOWN = 0,
  START = 1,
  LINE = 2,
  ORIGIN = 3,
  ARC_CW = 4,
  ARC_CCW = 5,
  CIRCLE = 6
};

}

class BoundaryLine :
  public atools::fs::bgl::BglBase
{
public:
  BoundaryLine();
  BoundaryLine(const atools::fs::BglReaderOptions *options, atools::io::BinaryStream *bs);
  virtual ~BoundaryLine();

  atools::fs::bgl::boundaryline::PointType getType() const
  {
    return type;
  }

  const atools::fs::bgl::BglPosition& getPosition() const
  {
    return position;
  }

  static QString boundarylineTypeToStr(atools::fs::bgl::boundaryline::PointType type);

  float getRadius() const
  {
    return radius;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::BoundaryLine& record);

  atools::fs::bgl::boundaryline::PointType type;
  atools::fs::bgl::BglPosition position;
  float radius = 0.f;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_AIRPORTBOUNDARYLINE_H
