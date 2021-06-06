/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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
#include "fs/navdatabaseoptions.h"

#include <QDebug>

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

Approach::Approach(const NavDatabaseOptions *options, BinaryStream *bs, bool sid, bool star)
  : Record(options, bs)
{
  if(sid || star)
  {
    // Disabled attempt to decode SID/STAR - not reliable since structure size varies
#if 0
    type = ap::GPS;
    gpsOverlay = true;
    if(sid)
      suffix = 'D';
    else if(star)
      suffix = 'A';
    else
      suffix = '\0';

    bs->skip(6);
    fixIdent = bs->readString(8);
    bs->skip(7);
    runwayNumber = bs->readUByte();
    runwayDesignator = bs->readUByte();
    bs->skip(3 + 4 + 2);
    int num = bs->readUShort();
    for(int i = 0; i < num; i++)
      legs.append(ApproachLeg(bs, false, sid || star));
#endif
  }
  else
  {
    suffix = bs->readByte();
    runwayNumber = bs->readUByte();

    int typeFlags = bs->readUByte();
    type = static_cast<ap::ApproachType>(typeFlags & 0xf);
    runwayDesignator = (typeFlags >> 4) & 0x7;
    gpsOverlay = (typeFlags & 0x80) == 0x80;

    // TODO compare numbers with actual record occurence
    numTransitions = bs->readUByte();
    int numLegs = bs->readUByte();
    Q_UNUSED(numLegs)
    int numMissedLegs = bs->readUByte();
    Q_UNUSED(numMissedLegs)

    unsigned int fixFlags = bs->readUInt();
    fixType = static_cast<ap::fix::ApproachFixType>(fixFlags & 0xf);
    fixIdent = converter::intToIcao((fixFlags >> 5) & 0xfffffff, true);

    unsigned int fixIdentFlags = bs->readUInt();
    fixRegion = converter::intToIcao(fixIdentFlags & 0x7ff, true);
    fixAirportIdent = converter::intToIcao((fixIdentFlags >> 11) & 0x1fffff, true);

    altitude = bs->readFloat();
    heading = bs->readFloat(); // Heading is float degrees
    missedAltitude = bs->readFloat();

    // Read subrecords
    while(bs->tellg() < startOffset + size)
    {
      Record r(options, bs);
      rec::ApprRecordType t = r.getId<rec::ApprRecordType>();
      if(checkSubRecord(r))
        return;

      switch(t)
      {

        case rec::LEGS:
        case rec::LEGS_MSFS:
          if(options->isIncludedNavDbObject(type::APPROACHLEG))
          {
            int num = bs->readUShort();
            for(int i = 0; i < num; i++)
              legs.append(ApproachLeg(bs, false, t == rec::LEGS_MSFS));
          }
          break;

        case rec::MISSED_LEGS:
        case rec::MISSED_LEGS_MSFS:
          if(options->isIncludedNavDbObject(type::APPROACHLEG))
          {
            int num = bs->readUShort();
            for(int i = 0; i < num; i++)
              missedLegs.append(ApproachLeg(bs, true, t == rec::MISSED_LEGS_MSFS));
          }
          break;

        case rec::TRANSITION:
        case rec::TRANSITION_MSFS:
          r.seekToStart();
          transitions.append(Transition(options, bs));
          break;

        default:
          qWarning().nospace().noquote() << "Unexpected record type in approach record 0x" << Qt::hex << t << Qt::dec
                                         << " for airport ident " << fixAirportIdent << bs->tellg();
      }
      r.seekToEnd();
    }
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
                          << ", hdg " << record.heading << Qt::endl;
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
