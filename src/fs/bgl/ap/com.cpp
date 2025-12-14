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

#include "fs/bgl/ap/com.h"
#include "io/binarystream.h"
#include "fs/navdatabaseoptions.h"
#include "atools.h"

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
    case com::ATIS_P3D_V5:
      return "ATIS";

    case com::MULTICOM:
    case com::MULTICOM_P3D_V5:
      return "MC";

    case com::UNICOM:
    case com::UNICOM_P3D_V5:
      return "UC";

    case com::CTAF:
    case com::CTAF_P3D_V5:
      return "CTAF";

    case com::GROUND:
    case com::GROUND_P3D_V5:
      return "G";

    case com::TOWER:
    case com::TOWER_P3D_V5:
      return "T";

    case com::CLEARANCE:
    case com::CLEARANCE_P3D_V5:
      return "C";

    case com::APPROACH:
    case com::APPROACH_P3D_V5:
      return "A";

    case com::DEPARTURE:
    case com::DEPARTURE_P3D_V5:
      return "D";

    case com::CENTER:
    case com::CENTER_P3D_V5:
      return "CTR";

    case com::FSS:
    case com::FSS_P3D_V5:
      return "FSS";

    case com::AWOS:
    case com::AWOS_P3D_V5:
      return "AWOS";

    case com::ASOS:
    case com::ASOS_P3D_V5:
      return "ASOS";

    case com::CLEARANCE_PRE_TAXI:
    case com::CLEARANCE_PRE_TAXI_P3D_V5:
      return "CPT";

    case com::REMOTE_CLEARANCE_DELIVERY:
    case com::REMOTE_CLEARANCE_DELIVERY_P3D_V5:
      return "RCD";
  }
  qWarning().nospace().noquote() << "Invalid COM type " << type;
  return "INVALID";
}

Com::Com()
  : type(atools::fs::bgl::com::NONE), frequency(0.0f)
{
}

Com::Com(const NavDatabaseOptions *options, BinaryStream *stream)
  : Record(options, stream)
{
  type = static_cast<com::ComType>(stream->readShort());
  frequency = stream->readInt() / 1000;

  atools::io::Encoding encoding = options->getSimulatorType() ==
                                  atools::fs::FsPaths::MSFS ? atools::io::UTF8 : atools::io::LATIN1;

  name = atools::removeNonAlphaNum(stream->readString(0x30, encoding));

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
