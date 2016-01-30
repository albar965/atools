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

#include "fs/bgl/ap/com.h"
#include "io/binarystream.h"

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

Com::Com(const BglReaderOptions *options, BinaryStream *bs)
  : Record(options, bs)
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
