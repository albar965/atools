/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

TaxiPath::TaxiPath(io::BinaryStream *stream, StructureType structureType)
{
  startIndex = stream->readUShort();
  int flags = stream->readShort();
  endIndex = flags & 0xfff; // Not MSFS
  runwayDesignator = (flags >> 12) & 0xf;

  flags = stream->readUByte();
  type = static_cast<taxipath::Type>(flags & 0xf);
  drawSurface = flags & (1 << 5);
  drawDetail = flags & (1 << 6);

  nameIndex = stream->readUByte();

  flags = stream->readUByte();
  centerline = flags & 1;
  centerlineLight = flags & 2;
  leftEdge = static_cast<taxipath::EdgeType>((flags >> 2) & 0x3);
  leftEdgeLight = flags & (1 << 4);
  rightEdge = static_cast<taxipath::EdgeType>((flags >> 5) & 0x3);
  rightEdgeLight = flags & (1 << 7);

  surface = static_cast<Surface>(stream->readUByte() & SURFACE_MASK);
  width = stream->readFloat();
  stream->skip(4); // weight limit
  stream->skip(4);

  if(structureType == STRUCT_P3DV4 || structureType == STRUCT_P3DV5)
    // Skip P3D material set GUID for seasons
    stream->skip(16);

  if(structureType == STRUCT_P3DV5)
    stream->skip(4);

  if(structureType == STRUCT_MSFS)
  {
    stream->skip(4);

    // UUID for taxi material {B037EA38-EDF8-4AE5-B41B-2CA423ADA3EF}
    // Raw 38EA37B0-F8ED-E54A-B41B-2CA423ADA3EF
    materialUuid = stream->readUuid();

    stream->skip(6);
    endIndex = stream->readUShort();
  }
}

QString TaxiPath::getName() const
{
  if(type == taxipath::RUNWAY)
    return converter::runwayToStr(nameIndex, runwayDesignator);
  else
    return taxiName;
}

QString TaxiPath::pathTypeToString(taxipath::Type type)
{
  switch(type)
  {
    case taxipath::VEHICLE:
    case taxipath::ROAD:
    case taxipath::PAINTEDLINE:
    case taxipath::UNKNOWN:
      return "UNKNOWN";

    case taxipath::TAXI:
      return "T";

    case taxipath::RUNWAY:
      return "R";

    case taxipath::PARKING:
      return "P";

    case taxipath::PATH:
      return "PT";

    case taxipath::CLOSED:
      return "C";
  }
  qWarning().nospace().noquote() << "Invalid taxi path type " << type;
  return "INVALID";
}

QString TaxiPath::edgeTypeToString(taxipath::EdgeType type)
{
  switch(type)
  {
    case taxipath::NONE:
      return "NONE";

    case taxipath::SOLID:
      return "SOLID";

    case taxipath::DASHED:
      return "DASHED";

    case taxipath::SOLID_DASHED:
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
                          << ", surface " << surface::surfaceToDbStr(record.surface)
                          << ", start " << record.startIndex << record.startPos
                          << ", end " << record.endIndex << record.endPos
                          << ", nameIndex " << record.nameIndex
                          << ", name " << record.getName()
                          << "]";

  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
