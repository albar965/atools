/*
 * RunwayVasi.cpp
 *
 *  Created on: 22.04.2015
 *      Author: alex
 */

#include "runwayvasi.h"

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
