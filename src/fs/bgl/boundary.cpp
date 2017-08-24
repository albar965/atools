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
      return "C";

    case atools::fs::bgl::boundary::CLASS_A:
      return "CA";

    case atools::fs::bgl::boundary::CLASS_B:
      return "CB";

    case atools::fs::bgl::boundary::CLASS_C:
      return "CC";

    case atools::fs::bgl::boundary::CLASS_D:
      return "CD";

    case atools::fs::bgl::boundary::CLASS_E:
      return "CE";

    case atools::fs::bgl::boundary::CLASS_F:
      return "CF";

    case atools::fs::bgl::boundary::CLASS_G:
      return "CG";

    case atools::fs::bgl::boundary::TOWER:
      return "T";

    case atools::fs::bgl::boundary::CLEARANCE:
      return "CL";

    case atools::fs::bgl::boundary::GROUND:
      return "G";

    case atools::fs::bgl::boundary::DEPARTURE:
      return "D";

    case atools::fs::bgl::boundary::APPROACH:
      return "A";

    case atools::fs::bgl::boundary::MOA:
      return "M";

    case atools::fs::bgl::boundary::RESTRICTED:
      return "R";

    case atools::fs::bgl::boundary::PROHIBITED:
      return "P";

    case atools::fs::bgl::boundary::WARNING:
      return "W";

    case atools::fs::bgl::boundary::ALERT:
      return "AL";

    case atools::fs::bgl::boundary::DANGER:
      return "DA";

    case atools::fs::bgl::boundary::NATIONAL_PARK:
      return "NP";

    case atools::fs::bgl::boundary::MODEC:
      return "MD";

    case atools::fs::bgl::boundary::RADAR:
      return "RD";

    case atools::fs::bgl::boundary::TRAINING:
      return "TR";
  }
  qWarning().nospace().noquote() << "Invalid BOUNDARY " << type;
  return "INVALID";
}

QString Boundary::altTypeToStr(boundary::AltitudeType type)
{
  switch(type)
  {
    case atools::fs::bgl::boundary::UNKNOWN:
      return "UNKNOWN";

    case atools::fs::bgl::boundary::MEAN_SEA_LEVEL:
      return "MSL";

    case atools::fs::bgl::boundary::ABOVE_GROUND_LEVEL:
      return "AGL";

    case atools::fs::bgl::boundary::UNLIMITED:
      return "UL";
  }
  qWarning().nospace().noquote() << "Invalid ALTITUDETYPE " << type;
  return "INVALID";
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
    qWarning() << "Not a boundary record" << hex << "0x" << id << dec;
    excluded = true;
    return;
  }
  type = static_cast<boundary::BoundaryType>(bs->readUByte());
  if(type < boundary::NONE || type > boundary::TRAINING)
  {
    valid = false;
    return;
  }

  int flags = bs->readUByte();
  // TODO fix in wiki
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

  minPosition = BglPosition(bs, true, 1000.f);
  maxPosition = BglPosition(bs, true, 1000.f);

  int numFreq = 0;
  while(bs->tellg() < startOffset + size)
  {
    Record r(options, bs);
    rec::BoundaryRecordType t = r.getId<rec::BoundaryRecordType>();
    if(checkSubRecord(r))
      return;

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
                          << ", maxPosition " << record.maxPosition << endl;
  out << record.lines;
  out << "]";

  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
