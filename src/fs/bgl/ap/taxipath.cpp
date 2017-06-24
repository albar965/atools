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

#include "fs/bgl/ap/taxipath.h"
#include "fs/bgl/converter.h"
#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

TaxiPath::TaxiPath(io::BinaryStream *bs, bool p3dV4Structure)
{
  startPoint = bs->readShort();
  int flags = bs->readShort();
  endPoint = flags & 0xfff;
  runwayDesignator = (flags >> 12) & 0xf;

  flags = bs->readUByte();
  type = static_cast<taxipath::Type>(flags & 0xf);
  drawSurface = flags & (1 << 5);
  drawDetail = flags & (1 << 6);

  runwayNumTaxiName = bs->readUByte();

  flags = bs->readUByte();
  centerline = flags & 1;
  centerlineLight = flags & 2;
  leftEdge = static_cast<taxipath::EdgeType>((flags >> 2) & 0x3);
  leftEdgeLight = flags & (1 << 4);
  rightEdge = static_cast<taxipath::EdgeType>((flags >> 5) & 0x3);
  rightEdgeLight = flags & (1 << 7);

  surface = static_cast<rw::Surface>(bs->readUByte() & rw::SURFACE_MASK);
  width = bs->readFloat();
  bs->readFloat(); // weight limit
  bs->skip(4);

  if(p3dV4Structure)
    // Skip P3D material set GUID for seasons
    bs->skip(16);
}

QString TaxiPath::getName() const
{
  if(type == taxipath::RUNWAY)
    return converter::runwayToStr(runwayNumTaxiName, runwayDesignator);
  else
    return taxiName;
}

QString TaxiPath::pathTypeToString(taxipath::Type type)
{
  switch(type)
  {
    case atools::fs::bgl::taxipath::VEHICLE:
      return "V";

    case atools::fs::bgl::taxipath::UNKNOWN:
      return "UNKNOWN";

    case atools::fs::bgl::taxipath::TAXI:
      return "T";

    case atools::fs::bgl::taxipath::RUNWAY:
      return "R";

    case atools::fs::bgl::taxipath::PARKING:
      return "P";

    case atools::fs::bgl::taxipath::PATH:
      return "PT";

    case atools::fs::bgl::taxipath::CLOSED:
      return "C";
  }
  qWarning().nospace().noquote() << "Invalid taxi path type " << type;
  return "INVALID";
}

QString TaxiPath::edgeTypeToString(taxipath::EdgeType type)
{
  switch(type)
  {
    case atools::fs::bgl::taxipath::NONE:
      return "NONE";

    case atools::fs::bgl::taxipath::SOLID:
      return "SOLID";

    case atools::fs::bgl::taxipath::DASHED:
      return "DASHED";

    case atools::fs::bgl::taxipath::SOLID_DASHED:
      return "SOLID_DASHED";
  }
  qWarning().nospace().noquote() << "Invalid taxi path edge type " << type;
  return "INVALID";
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
