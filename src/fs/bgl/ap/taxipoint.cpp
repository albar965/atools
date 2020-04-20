/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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

#include "fs/bgl/ap/taxipoint.h"
#include "fs/bgl/ap/parking.h"
#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

TaxiPoint::TaxiPoint(io::BinaryStream *bs, atools::fs::bgl::StructureType structureType)
{
  type = static_cast<taxipoint::PointType>(bs->readUByte());
  dir = static_cast<taxipoint::PointDir>(bs->readUByte());
  bs->skip(2);
  pos = BglPosition(bs);

  if(structureType == STRUCT_P3DV5)
    bs->skip(4);
}

TaxiPoint::TaxiPoint()
{
  type = taxipoint::UNKNOWN;
  dir = taxipoint::UNKNOWN_DIR;
}

TaxiPoint::TaxiPoint(const Parking& parkingSpot)
{
  // Create a taxi point from a parking spot. This does not have a BGL equivalent.
  type = taxipoint::PARKING;
  dir = taxipoint::UNKNOWN_DIR;
  pos = parkingSpot.getPosition();
  parking = parkingSpot;
}

QString TaxiPoint::pointTypeToString(taxipoint::PointType type)
{
  switch(type)
  {
    case atools::fs::bgl::taxipoint::PARKING:
      return "P";

    case atools::fs::bgl::taxipoint::UNKNOWN:
      return "UNKNOWN";

    case atools::fs::bgl::taxipoint::NORMAL:
      return "N";

    case atools::fs::bgl::taxipoint::HOLD_SHORT:
      return "HS";

    case atools::fs::bgl::taxipoint::HOLD_SHORT_NO_DRAW:
      return "HSND";

    case atools::fs::bgl::taxipoint::ILS_HOLD_SHORT:
      return "IHS";

    case atools::fs::bgl::taxipoint::ILS_HOLD_SHORT_NO_DRAW:
      return "IHSND";
  }
  qWarning().nospace().noquote() << "Invalid taxi point type " << type;
  return "INVALID";
}

QString TaxiPoint::dirToString(taxipoint::PointDir dir)
{
  switch(dir)
  {
    case atools::fs::bgl::taxipoint::UNKNOWN_DIR:
      return "UNKNOWN_DIR";

    case atools::fs::bgl::taxipoint::FORWARD:
      return "F";

    case atools::fs::bgl::taxipoint::REVERSE:
      return "R";
  }
  qWarning().nospace().noquote() << "Invalid taxi point dir " << dir;
  return "INVALID";
}

QDebug operator<<(QDebug out, const TaxiPoint& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << " TaxiPoint["
                          << "type " << TaxiPoint::pointTypeToString(record.type)
                          << ", dir " << TaxiPoint::dirToString(record.dir)
                          << ", pos " << record.pos << "]";

  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
