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

#ifndef BGL_AIRPORTTAXIPOINT_H_
#define BGL_AIRPORTTAXIPOINT_H_

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

class Parking;

namespace taxipoint {
enum PointType
{
  UNKNOWN = 0,
  NORMAL = 1,
  HOLD_SHORT = 2,
  ILS_HOLD_SHORT = 4,
  HOLD_SHORT_NO_DRAW = 5, // TODO wiki add
  ILS_HOLD_SHORT_NO_DRAW = 6, // TODO wiki add
  PARKING // not a BGL type
};

enum PointDir
{
  UNKNOWN_DIR = 0,
  FORWARD = 1,
  REVERSE = 2
};

}

class TaxiPoint
{
public:
  TaxiPoint(atools::io::BinaryStream *bs);
  TaxiPoint();
  TaxiPoint(const atools::fs::bgl::Parking& parking);

  atools::fs::bgl::taxipoint::PointType getType() const
  {
    return type;
  }

  atools::fs::bgl::taxipoint::PointDir getDir() const
  {
    return dir;
  }

  const atools::fs::bgl::BglPosition& getPosition() const
  {
    return pos;
  }

  static QString pointTypeToString(atools::fs::bgl::taxipoint::PointType type);
  static QString dirToString(atools::fs::bgl::taxipoint::PointDir dir);

private:
  friend QDebug operator<<(QDebug out, const TaxiPoint& record);

  atools::fs::bgl::taxipoint::PointType type;
  atools::fs::bgl::taxipoint::PointDir dir;
  atools::fs::bgl::BglPosition pos;
};

QDebug operator<<(QDebug out, const atools::fs::bgl::TaxiPoint& record);

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_AIRPORTTAXIPOINT_H_ */
