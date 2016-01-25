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

#include "fs/bgl/ap/rw/runwayvasi.h"
#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

QString RunwayVasi::vasiTypeToStr(rw::VasiType type)
{
  switch(type)
  {
    case rw::NONE:
      return "NONE";

    case rw::VASI21:
      return "VASI21";

    case rw::VASI31:
      return "VASI31";

    case rw::VASI22:
      return "VASI22";

    case rw::VASI32:
      return "VASI32";

    case rw::VASI23:
      return "VASI23";

    case rw::VASI33:
      return "VASI33";

    case rw::PAPI2:
      return "PAPI2";

    case rw::PAPI4:
      return "PAPI4";

    case rw::TRICOLOR:
      return "TRICOLOR";

    case rw::PVASI:
      return "PVASI";

    case rw::TVASI:
      return "TVASI";

    case rw::BALL:
      return "BALL";

    case rw::APAP_PANELS:
      return "APAP_PANELS";
  }
  qWarning().nospace().noquote() << "Unknown VASI type " << type;
  return "";
}

RunwayVasi::RunwayVasi(BinaryStream *bs)
  : Record(bs)
{
  type = static_cast<rw::VasiType>(bs->readShort());
  bs->skip(12); // BiasX  BiasZ  Spacing
  pitch = bs->readFloat();
}

QDebug operator<<(QDebug out, const RunwayVasi& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record)
  << " Vasi[type " << RunwayVasi::vasiTypeToStr(record.type)
  << ", pitch " << record.pitch
  << "]";

  return out;
}

RunwayVasi::~RunwayVasi()
{
}

} // namespace bgl
} // namespace fs
} // namespace atools
