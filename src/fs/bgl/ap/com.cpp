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

#include "fs/bgl/ap/com.h"
#include "io/binarystream.h"

#include <QDebug>

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
      return "MC";

    case com::UNICOM:
      return "UC";

    case com::CTAF:
      return "CTAF";

    case com::GROUND:
      return "G";

    case com::TOWER:
      return "T";

    case com::CLEARANCE:
      return "C";

    case com::APPROACH:
      return "A";

    case com::DEPARTURE:
      return "D";

    case com::CENTER:
      return "CTR";

    case com::FSS:
      return "FSS";

    case com::AWOS:
      return "AWOS";

    case com::ASOS:
      return "ASOS";

    case com::CLEARANCE_PRE_TAXI:
      return "CPT";

    case com::REMOTE_CLEARANCE_DELIVERY:
      return "RCD";
  }
  qWarning().nospace().noquote() << "Invalid COM type " << type;
  return "INVALID";
}

Com::Com()
  : type(atools::fs::bgl::com::NONE), frequency(0.0f)
{
}

Com::Com(const NavDatabaseOptions *options, BinaryStream *bs)
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
