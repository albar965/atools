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

#include "fs/bgl/ap/taxipath.h"
#include "fs/bgl/converter.h"
#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

TaxiPath::TaxiPath(io::BinaryStream *bs)
{
  startPoint = bs->readShort();
  int flags = bs->readShort();
  endPoint = flags & 0xfff;
  runwayDesignator = (flags >> 12) & 0xf;

  flags = bs->readUByte();
  type = static_cast<taxi::PathType>(flags & 0xf);
  drawSurface = flags & (1 << 5);
  drawDetail = flags & (1 << 6);

  runwayNumTaxiName = bs->readUByte();

  flags = bs->readUByte();
  centerline = flags & 1;
  centerlineLight = flags & 2;
  leftEdge = static_cast<taxi::EdgeType>((flags >> 2) & 0x3);
  leftEdgeLight = flags & (1 << 4);
  rightEdge = static_cast<taxi::EdgeType>((flags >> 5) & 0x3);
  rightEdgeLight = flags & (1 << 7);

  surface = static_cast<rw::Surface>(bs->readUByte());
  width = bs->readFloat();
  weightLimit = bs->readFloat();
  bs->skip(4);
}

QString TaxiPath::getName() const
{
  if(type == taxi::RUNWAY)
    return converter::runwayToStr(runwayNumTaxiName, runwayDesignator);
  else
    return taxiName;
}

QString TaxiPath::pathTypeToString(taxi::PathType type)
{
  switch(type)
  {
    case atools::fs::bgl::taxi::VEHICLE:
      return "VEHICLE";

    case atools::fs::bgl::taxi::UNKNOWN_PATH_TYPE:
      return "UNKNOWN_PATH_TYPE";

    case atools::fs::bgl::taxi::TAXI:
      return "TAXI";

    case atools::fs::bgl::taxi::RUNWAY:
      return "RUNWAY";

    case atools::fs::bgl::taxi::PARKING:
      return "PARKING";

    case atools::fs::bgl::taxi::PATH:
      return "PATH";

    case atools::fs::bgl::taxi::CLOSED:
      return "CLOSED";
  }
  qWarning().nospace().noquote() << "Unknown taxi path type " << type;
  return QString();
}

QString TaxiPath::edgeTypeToString(taxi::EdgeType type)
{
  switch(type)
  {
    case atools::fs::bgl::taxi::NONE:
      return "NONE";

    case atools::fs::bgl::taxi::SOLID:
      return "SOLID";

    case atools::fs::bgl::taxi::DASHED:
      return "DASHED";

    case atools::fs::bgl::taxi::SOLID_DASHED:
      return "SOLID_DASHED";
  }
  qWarning().nospace().noquote() << "Unknown taxi path edge type " << type;
  return QString();
}

QDebug operator<<(QDebug out, const TaxiPath& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << " TaxiPath["
  << "type " << TaxiPath::pathTypeToString(record.type)
  << ", surface " << Runway::surfaceToStr(record.surface)
  << ", name " << record.getName()
  << ", left edge " << TaxiPath::edgeTypeToString(record.leftEdge)
  << ", right edge " << TaxiPath::edgeTypeToString(record.rightEdge)
  << ", start pos " << record.start
  << ", end pos " << record.end
  << "]";

  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
