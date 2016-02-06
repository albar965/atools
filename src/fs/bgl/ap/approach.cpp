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

#include "fs/bgl/ap/approach.h"
#include "fs/bgl/converter.h"
#include "fs/bgl/recordtypes.h"
#include "io/binarystream.h"
#include "fs/bglreaderoptions.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

Approach::Approach(const BglReaderOptions *options, BinaryStream *bs)
  : Record(options, bs)
{
  bs->skip(1); // suffix
  runwayNumber = bs->readUByte();

  int typeFlags = bs->readUByte();
  type = static_cast<ap::ApproachType>(typeFlags & 0xf);
  runwayDesignator = (typeFlags >> 4) & 0x7;
  gpsOverlay = (typeFlags & 0x80) == 0x80;

  numTransitions = bs->readUByte();
  int numLegs = bs->readUByte();
  Q_UNUSED(numLegs);
  int numMissedLegs = bs->readUByte();
  Q_UNUSED(numMissedLegs);

  unsigned int fixFlags = bs->readUInt();
  fixType = static_cast<ap::ApproachFixType>(fixFlags & 0xf);
  fixIdent = converter::intToIcao((fixFlags >> 5) & 0xfffffff, true);

  unsigned int fixIdentFlags = bs->readUInt();
  fixRegion = converter::intToIcao(fixIdentFlags & 0x7ff, true);
  fixAirportIdent = converter::intToIcao((fixIdentFlags >> 11) & 0x1fffff, true);

  altitude = bs->readFloat();
  heading = bs->readFloat(); // TODO wiki heading is float degress
  missedAltitude = bs->readFloat();

  while(bs->tellg() < startOffset + size)
  {
    Record r(options, bs);
    rec::ApprRecordType t = r.getId<rec::ApprRecordType>();

    switch(t)
    {
      case rec::TRANSITION:
        r.seekToStart();
        transitions.push_back(Transition(options, bs));
        break;

      case rec::LEGS:
        {
          int num = bs->readUShort();
          for(int i = 0; i < num; i++)
            legs.push_back(ApproachLeg(bs, false));
        }
        break;
      case rec::MISSED_LEGS:
        {
          int num = bs->readUShort();
          for(int i = 0; i < num; i++)
            missedLegs.push_back(ApproachLeg(bs, true));
        }
        break;

      case atools::fs::bgl::rec::TRANSITION_LEGS:
      default:
        qWarning().nospace().noquote() << "Unexpected record type in approach record 0x" << hex << t << dec
                                       << " for airport ident " << fixAirportIdent;
    }
    r.seekToEnd();
  }
}

QDebug operator<<(QDebug out, const Approach& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record)
  << " Approach[type "
  << ap::approachTypeToStr(record.type)
  << ", rwy " << record.getRunwayName()
  << ", gps overlay " << record.gpsOverlay
  << ", fix type " << ap::approachFixTypeToStr(record.fixType)
  << ", fix " << record.fixIdent
  << ", fix region " << record.fixRegion
  << ", ap icao " << record.fixAirportIdent
  << ", alt " << record.altitude
  << ", hdg " << record.heading << endl;
  out << record.transitions;
  out << record.legs;
  out << record.missedLegs;
  out << "]";
  return out;
}

Approach::~Approach()
{
}

QString Approach::getRunwayName() const
{
  return converter::runwayToStr(runwayNumber, runwayDesignator);
}

} // namespace bgl
} // namespace fs
} // namespace atools
