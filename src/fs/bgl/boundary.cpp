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

#include "fs/bgl/boundary.h"
#include "io/binarystream.h"
#include "fs/bgl/recordtypes.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

QString Boundary::boundaryTypeToStr(boundary::BoundaryType type)
{
  switch(type)
  {
    case atools::fs::bgl::boundary::NONE:
      return "NONE";

    case atools::fs::bgl::boundary::CENTER:
      return "CENTER";

    case atools::fs::bgl::boundary::CLASS_A:
      return "CLASS_A";

    case atools::fs::bgl::boundary::CLASS_B:
      return "CLASS_B";

    case atools::fs::bgl::boundary::CLASS_C:
      return "CLASS_C";

    case atools::fs::bgl::boundary::CLASS_D:
      return "CLASS_D";

    case atools::fs::bgl::boundary::CLASS_E:
      return "CLASS_E";

    case atools::fs::bgl::boundary::CLASS_F:
      return "CLASS_F";

    case atools::fs::bgl::boundary::CLASS_G:
      return "CLASS_G";

    case atools::fs::bgl::boundary::TOWER:
      return "TOWER";

    case atools::fs::bgl::boundary::CLEARANCE:
      return "CLEARANCE";

    case atools::fs::bgl::boundary::GROUND:
      return "GROUND";

    case atools::fs::bgl::boundary::DEPARTURE:
      return "DEPARTURE";

    case atools::fs::bgl::boundary::APPROACH:
      return "APPROACH";

    case atools::fs::bgl::boundary::MOA:
      return "MOA";

    case atools::fs::bgl::boundary::RESTRICTED:
      return "RESTRICTED";

    case atools::fs::bgl::boundary::PROHIBITED:
      return "PROHIBITED";

    case atools::fs::bgl::boundary::WARNING:
      return "WARNING";

    case atools::fs::bgl::boundary::ALERT:
      return "ALERT";

    case atools::fs::bgl::boundary::DANGER:
      return "DANGER";

    case atools::fs::bgl::boundary::NATIONAL_PARK:
      return "NATIONAL_PARK";

    case atools::fs::bgl::boundary::MODEC:
      return "MODEC";

    case atools::fs::bgl::boundary::RADAR:
      return "RADAR";

    case atools::fs::bgl::boundary::TRAINING:
      return "TRAINING";
  }
  qWarning().nospace().noquote() << "Unknown BOUNDARY " << type;
  return QString();
}

QString Boundary::altTypeToStr(boundary::AltitudeType type)
{
  switch(type)
  {
    case atools::fs::bgl::boundary::UNKNOWN:
      return "UNKNOWN";

    case atools::fs::bgl::boundary::MEAN_SEA_LEVEL:
      return "MEAN_SEA_LEVEL";

    case atools::fs::bgl::boundary::ABOVE_GROUND_LEVEL:
      return "ABOVE_GROUND_LEVEL";

    case atools::fs::bgl::boundary::UNLIMITED:
      return "UNLIMITED";
  }
  qWarning().nospace().noquote() << "Unknown ALTITUDETYPE " << type;
  return QString();
}

Boundary::Boundary()
  : type(boundary::NONE), minAltType(boundary::UNKNOWN), maxAltType(boundary::UNKNOWN)
{
}

Boundary::Boundary(const NavDatabaseOptions *options, BinaryStream *bs)
  : Record(options, bs)
{
  if(id != rec::BOUNDARY)
  {
    qWarning() << "Not a boundary record" << hex << "0x" << id;
    excluded = true;
    return;
  }
  type = static_cast<boundary::BoundaryType>(bs->readUByte());
  int flags = bs->readUByte();
  maxAltType = static_cast<boundary::AltitudeType>(flags & 0xf);
  minAltType = static_cast<boundary::AltitudeType>((flags >> 4) & 0xf);

  minPosition = BglPosition(bs, true, 1000.f);
  maxPosition = BglPosition(bs, true, 1000.f);

  int numFreq = 0;
  while(bs->tellg() < startOffset + size)
  {
    Record r(options, bs);
    rec::BoundaryRecordType t = r.getId<rec::BoundaryRecordType>();
    switch(t)
    {
      case atools::fs::bgl::rec::BOUNDARY_COM:
        com = true;
        numFreq++;
        // Read COM record directly here
        comType = static_cast<com::ComType>(bs->readShort());
        comFrequency = bs->readInt() / 1000;
        comName = bs->readString(r.getSize() - 12);
        break;
      case rec::BOUNDARY_NAME:
        name = bs->readString(r.getSize() - Record::SIZE);
        break;
      case rec::BOUNDARY_LINES:
        {
          // Read geometry
          int numPoints = bs->readUShort();
          for(int i = 0; i < numPoints; i++)
            lines.append(BoundarySegment(options, bs));
        }
        break;
      default:
        qWarning().nospace().noquote() << "Unexpected record type in Boundary record 0x" << hex << t;
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
  << ", maxPosition " << record.maxPosition << endl;
  out << record.lines;
  out << "]";

  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
