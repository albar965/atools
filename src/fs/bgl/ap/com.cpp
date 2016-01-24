/*
 * AirportCom.cpp
 *
 *  Created on: 23.04.2015
 *      Author: alex
 */

#include "com.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

QString Com::comTypeToStr(com::ComType type)
{
  switch(type)
  {
    case com::NONE:
      return "NONE";

    case com::ATIS:
      return "ATIS";

    case com::MULTICOM:
      return "MULTICOM";

    case com::UNICOM:
      return "UNICOM";

    case com::CTAF:
      return "CTAF";

    case com::GROUND:
      return "GROUND";

    case com::TOWER:
      return "TOWER";

    case com::CLEARANCE:
      return "CLEARANCE";

    case com::APPROACH:
      return "APPROACH";

    case com::DEPARTURE:
      return "DEPARTURE";

    case com::CENTER:
      return "CENTER";

    case com::FSS:
      return "FSS";

    case com::AWOS:
      return "AWOS";

    case com::ASOS:
      return "ASOS";

    case com::CLEARANCE_PRE_TAXI:
      return "CLEARANCE_PRE_TAXI";

    case com::REMOTE_CLEARANCE_DELIVERY:
      return "REMOTE_CLEARANCE_DELIVERY";
  }
  qWarning().nospace().noquote() << "Unknown COM type " << type;
  return "";
}

Com::Com(BinaryStream *bs)
  : Record(bs)
{
  type = static_cast<com::ComType>(bs->readShort());
  frequency = bs->readInt() / 1000;
  name = bs->readString(0x30);
}

QDebug operator<<(QDebug out, const Com& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record)
  << " Com[type " << Com::comTypeToStr(record.type)
  << ", name " << record.name
  << ", frequency " << record.frequency
  << "]";

  return out;
}

Com::~Com()
{
}

} // namespace bgl
} // namespace fs
} // namespace atools
