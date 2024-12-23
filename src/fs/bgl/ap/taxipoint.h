/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_BGL_AIRPORTTAXIPOINT_H
#define ATOOLS_BGL_AIRPORTTAXIPOINT_H

#include "fs/bgl/bglposition.h"
#include "fs/bgl/ap/parking.h"
#include "fs/bgl/ap/parking.h"
#include "fs/bgl/record.h"

#include <QString>

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {

namespace taxipoint {
enum PointType
{
  UNKNOWN = 0,
  NORMAL = 1,
  HOLD_SHORT = 2,
  ILS_HOLD_SHORT = 4,
  HOLD_SHORT_NO_DRAW = 5,
  ILS_HOLD_SHORT_NO_DRAW = 6,
  PARKING /* not a BGL type */
};

/* Orientation of a hold short point */
enum PointDir
{
  UNKNOWN_DIR = 0,
  FORWARD = 1,
  REVERSE = 2
};

}

/*
 * Start or end point of a taxiway segment.
 */
class TaxiPoint
{
public:
  TaxiPoint();
  TaxiPoint(atools::io::BinaryStream *stream, atools::fs::bgl::StructureType structureType);
  TaxiPoint(const atools::fs::bgl::Parking& parking);

  atools::fs::bgl::taxipoint::PointType getType() const
  {
    return type;
  }

  /*
   * @return get orientation of a hold short point (only valid for hold short point types)
   */
  atools::fs::bgl::taxipoint::PointDir getOrientation() const
  {
    return dir;
  }

  const atools::fs::bgl::BglPosition& getPosition() const
  {
    return position;
  }

  /*
   * @return get parking record class. Only valid if type is PARKING.
   */
  const atools::fs::bgl::Parking& getParking() const
  {
    return parking;
  }

  static QString pointTypeToString(atools::fs::bgl::taxipoint::PointType type);
  static QString dirToString(atools::fs::bgl::taxipoint::PointDir dir);

  bool isValid() const
  {
    return position.isValid();
  }

private:
  friend QDebug operator<<(QDebug out, const TaxiPoint& record);

  atools::fs::bgl::taxipoint::PointType type = atools::fs::bgl::taxipoint::UNKNOWN;
  atools::fs::bgl::taxipoint::PointDir dir = atools::fs::bgl::taxipoint::UNKNOWN_DIR;
  atools::fs::bgl::BglPosition position;
  atools::fs::bgl::Parking parking;
};

QDebug operator<<(QDebug out, const atools::fs::bgl::TaxiPoint& record);

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_AIRPORTTAXIPOINT_H
