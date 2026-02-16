/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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

#include "fs/bgl/boundary.h"
#include "io/binarystream.h"
#include "fs/bgl/recordtypes.h"
#include "fs/navdatabaseoptions.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

QString Boundary::boundaryTypeToStr(boundary::BoundaryType type)
{
  switch(type)
  {
    case atools::fs::bgl::boundary::NONE:
      return QStringLiteral("NONE");

    case atools::fs::bgl::boundary::CENTER:
      return QStringLiteral("C");

    case atools::fs::bgl::boundary::CLASS_A:
      return QStringLiteral("CA");

    case atools::fs::bgl::boundary::CLASS_B:
      return QStringLiteral("CB");

    case atools::fs::bgl::boundary::CLASS_C:
      return QStringLiteral("CC");

    case atools::fs::bgl::boundary::CLASS_D:
      return QStringLiteral("CD");

    case atools::fs::bgl::boundary::CLASS_E:
      return QStringLiteral("CE");

    case atools::fs::bgl::boundary::CLASS_F:
      return QStringLiteral("CF");

    case atools::fs::bgl::boundary::CLASS_G:
      return QStringLiteral("CG");

    case atools::fs::bgl::boundary::TOWER:
      return QStringLiteral("T");

    case atools::fs::bgl::boundary::CLEARANCE:
      return QStringLiteral("CL");

    case atools::fs::bgl::boundary::GROUND:
      return QStringLiteral("G");

    case atools::fs::bgl::boundary::DEPARTURE:
      return QStringLiteral("D");

    case atools::fs::bgl::boundary::APPROACH:
      return QStringLiteral("A");

    case atools::fs::bgl::boundary::MOA:
      return QStringLiteral("M");

    case atools::fs::bgl::boundary::RESTRICTED:
      return QStringLiteral("R");

    case atools::fs::bgl::boundary::PROHIBITED:
      return QStringLiteral("P");

    case atools::fs::bgl::boundary::WARNING:
      return QStringLiteral("W");

    case atools::fs::bgl::boundary::ALERT:
      return QStringLiteral("AL");

    case atools::fs::bgl::boundary::DANGER:
      return QStringLiteral("DA");

    case atools::fs::bgl::boundary::NATIONAL_PARK:
      return QStringLiteral("NP");

    case atools::fs::bgl::boundary::MODEC:
      return QStringLiteral("MD");

    case atools::fs::bgl::boundary::RADAR:
      return QStringLiteral("RD");

    case atools::fs::bgl::boundary::TRAINING:
      return QStringLiteral("TR");

      // return "CN";Caution- DFD
      // return "WW";Wave window - OpenAir format
      // return "GP";Glider prohibited - OpenAir format
  }
  qWarning().nospace().noquote() << "Invalid BOUNDARY " << type;
  return QStringLiteral("INVALID");
}

QString Boundary::altTypeToStr(boundary::AltitudeType type)
{
  switch(type)
  {
    case atools::fs::bgl::boundary::UNKNOWN:
      return QStringLiteral("UNKNOWN");

    case atools::fs::bgl::boundary::MEAN_SEA_LEVEL:
      return QStringLiteral("MSL");

    case atools::fs::bgl::boundary::ABOVE_GROUND_LEVEL:
      return QStringLiteral("AGL");

    case atools::fs::bgl::boundary::UNLIMITED:
      return QStringLiteral("UL");
  }
  qWarning().nospace().noquote() << "Invalid ALTITUDETYPE " << type;
  return QStringLiteral("INVALID");
}

Boundary::Boundary()
  : type(boundary::NONE), minAltType(boundary::UNKNOWN), maxAltType(boundary::UNKNOWN)
{
}

Boundary::Boundary(const NavDatabaseOptions *options, BinaryStream *stream)
  : Record(options, stream)
{
  if(id != rec::BOUNDARY && id != rec::BOUNDARY_MSFS2024)
  {
    qWarning() << "Not a boundary record" << Qt::hex << "0x" << id << Qt::dec;
    excluded = true;
    return;
  }
  type = static_cast<boundary::BoundaryType>(stream->readUByte());
  if(type < boundary::NONE || type > boundary::TRAINING)
  {
    valid = false;
    return;
  }

  int flags = stream->readUByte();
  minAltType = static_cast<boundary::AltitudeType>(flags & 0xf);
  maxAltType = static_cast<boundary::AltitudeType>((flags >> 4) & 0xf);

  if(maxAltType < boundary::UNKNOWN || maxAltType > boundary::UNLIMITED)
  {
    valid = false;
    return;
  }

  if(minAltType < boundary::UNKNOWN || maxAltType > boundary::UNLIMITED)
  {
    valid = false;
    return;
  }

  minPosition = BglPosition(stream, true, 1000.f);
  maxPosition = BglPosition(stream, true, 1000.f);
  atools::io::Encoding encoding = options->getSimulatorType() == atools::fs::FsPaths::MSFS || id != rec::BOUNDARY_MSFS2024 ?
                                  atools::io::UTF8 : atools::io::LATIN1;

  if(id == rec::BOUNDARY_MSFS2024)
    stream->skip(20); // Skip unknown data

  int numFreq = 0;
  while(stream->tellg() < startOffset + size)
  {
    Record r(options, stream);
    rec::BoundaryRecordType t = r.getId<rec::BoundaryRecordType>();
    if(checkSubRecord(r))
      return;

    switch(t)
    {
      case atools::fs::bgl::rec::BOUNDARY_COM:
        com = true;
        numFreq++;
        // Read COM record directly here
        comType = static_cast<com::ComType>(stream->readShort());
        comFrequency = stream->readInt() / 1000;
        comName = stream->readString(r.getSize() - 12, encoding);
        break;

      case rec::BOUNDARY_NAME:
        name = stream->readString(r.getSize() - Record::SIZE, atools::io::LATIN1);
        break;

      case rec::BOUNDARY_LINES:
        {
          // Read geometry
          int numPoints = stream->readUShort();
          for(int i = 0; i < numPoints; i++)
            lines.append(BoundarySegment(options, stream));
        }
        break;
        // default:
        // valid = false;
        // return;
    }
    r.seekToEnd();
  }
  if(numFreq > 1)
    qWarning() << "Found more than 1 boundary com frequency";

}

Boundary::~Boundary()
{
}

QDebug operator<<(QDebug out, const Boundary& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record)
                          << " Boundary[type " << Boundary::boundaryTypeToStr(record.type)
                          << ", name " << record.name
                          << ", minAltType " << Boundary::altTypeToStr(record.minAltType)
                          << ", maxAltType " << Boundary::altTypeToStr(record.maxAltType)
                          << ", minPosition " << record.minPosition
                          << ", maxPosition " << record.maxPosition << Qt::endl;
  out << record.lines;
  out << "]";

  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
