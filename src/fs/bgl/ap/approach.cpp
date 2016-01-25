/*
 * Approach.cpp
 *
 *  Created on: 25.04.2015
 *      Author: alex
 */

#include "fs/bgl/ap/approach.h"
#include "fs/bgl/converter.h"
#include "fs/bgl/recordtypes.h"
#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

QString Approach::approachTypeToStr(ap::ApproachType type)
{
  switch(type)
  {
    case ap::GPS:
      return "GPS";

    case ap::VOR:
      return "VOR";

    case ap::NDB:
      return "NDB";

    case ap::ILS:
      return "ILS";

    case ap::LOCALIZER:
      return "LOCALIZER";

    case ap::SDF:
      return "SDF";

    case ap::LDA:
      return "LDA";

    case ap::VORDME:
      return "VORDME";

    case ap::NDBDME:
      return "NDBDME";

    case ap::RNAV:
      return "RNAV";

    case ap::LOCALIZER_BACKCOURSE:
      return "LOCALIZER_BACKCOURSE";
  }
  qWarning().nospace().noquote() << "Unknown approach type " << type;
  return "";
}

QString Approach::approachFixTypeToStr(ap::ApproachFixType type)
{
  switch(type)
  {
    case ap::FIX_VOR:
      return "FIX_VOR";

    case ap::FIX_NDB:
      return "FIX_NDB";

    case ap::FIX_TERMINAL_NDB:
      return "FIX_TERMINAL_NDB";

    case ap::FIX_WAYPOINT:
      return "FIX_WAYPOINT";

    case ap::FIX_TERMINAL_WAYPOINT:
      return "FIX_TERMINAL_WAYPOINT";

    case ap::FIX_RUNWAY:
      return "FIX_RUNWAY";
  }
  qWarning().nospace().noquote() << "Unknown approach fix type " << type;
  return "";
}

Approach::Approach(BinaryStream *bs)
  : Record(bs)
{
  bs->skip(1); // suffix
  runwayNumber = bs->readByte();

  // TODO use enum for flags
  int typeFlags = bs->readByte();
  type = static_cast<ap::ApproachType>(typeFlags & 0xf);
  runwayDesignator = (typeFlags >> 4) & 0x7;
  gpsOverlay = (typeFlags & 0x80) == 0x80;

  numTransitions = bs->readByte();
  numLegs = bs->readByte();
  numMissedLegs = bs->readByte();

  unsigned int fixFlags = bs->readUInt();
  fixType = static_cast<ap::ApproachFixType>(fixFlags & 0xf);
  fixIdent = converter::intToIcao((fixFlags >> 5) & 0xfffffff, true);

  unsigned int fixIdentFlags = bs->readUInt();
  fixRegion = converter::intToIcao(fixIdentFlags & 0x7ff, true);
  fixAirportIdent = converter::intToIcao((fixIdentFlags >> 11) & 0x1fffff, true);

  altitude = bs->readFloat();
  heading = bs->readFloat();
  missedAltitude = bs->readFloat();

  while(bs->tellg() < startOffset + size)
  {
    Record r(bs);
    rec::ApprRecordType t = r.getId<rec::ApprRecordType>();

    switch(t)
    {
      case rec::TRANSITION:
        r.seekToStart();
        transitions.push_back(Transition(bs));
        break;

      case rec::LEGS:
      case rec::MISSED_LEGS:
      case rec::TRANSITION_LEGS:
        break;

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
  << Approach::approachTypeToStr(record.type)
  << ", rwy " << record.getRunwayName()
  << ", gps overlay " << record.gpsOverlay
  << ", fix type " << Approach::approachFixTypeToStr(record.fixType)
  << ", fix " << record.fixIdent
  << ", fix region " << record.fixRegion
  << ", ap icao " << record.fixAirportIdent
  << ", alt " << record.altitude
  << ", hdg " << record.heading << endl;
  out << record.transitions;
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
