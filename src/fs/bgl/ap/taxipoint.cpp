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

#include "fs/bgl/ap/taxipoint.h"
#include "fs/bgl/ap/parking.h"
#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

TaxiPoint::TaxiPoint(io::BinaryStream *bs)
{
  type = static_cast<taxi::PointType>(bs->readUByte());
  dir = static_cast<taxi::PointDir>(bs->readUByte());
  bs->skip(2);
  pos = BglPosition(bs);
}

TaxiPoint::TaxiPoint()
{
  type = taxi::UNKNOWN;
  dir = taxi::UNKNOWN_DIR;
}

TaxiPoint::TaxiPoint(const Parking& parking)
{
  type = taxi::UNKNOWN;
  dir = taxi::UNKNOWN_DIR;
  pos = parking.getPosition();
}

QString TaxiPoint::pointTypeToString(taxi::PointType type)
{
  switch(type)
  {
    case atools::fs::bgl::taxi::UNKNOWN:
      return "UNKNOWN";

    case atools::fs::bgl::taxi::NORMAL:
      return "NORMAL";

    case atools::fs::bgl::taxi::HOLD_SHORT:
      return "HOLD_SHORT";

    case atools::fs::bgl::taxi::HOLD_SHORT_NO_DRAW:
      return "HOLD_SHORT_NO_DRAW";

    case atools::fs::bgl::taxi::ILS_HOLD_SHORT:
      return "ILS_HOLD_SHORT";

    case atools::fs::bgl::taxi::ILS_HOLD_SHORT_NO_DRAW:
      return "ILS_HOLD_SHORT_NO_DRAW";
  }
  qWarning().nospace().noquote() << "Unknown taxi point type " << type;
  return QString();
}

QString TaxiPoint::dirToString(taxi::PointDir dir)
{
  switch(dir)
  {
    case atools::fs::bgl::taxi::UNKNOWN_DIR:
      return "UNKNOWN_DIR";

    case atools::fs::bgl::taxi::FORWARD:
      return "FORWARD";

    case atools::fs::bgl::taxi::REVERSE:
      return "REVERSE";
  }
  qWarning().nospace().noquote() << "Unknown taxi point dir " << dir;
  return QString();
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
