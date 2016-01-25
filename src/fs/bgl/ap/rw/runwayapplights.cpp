/*
 * RunwayApproachLights.cpp
 *
 *  Created on: 22.04.2015
 *      Author: alex
 */

#include "fs/bgl/ap/rw/runwayapplights.h"
#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

QString RunwayAppLights::appLightSystemToStr(rw::ApproachLightSystem type)
{
  switch(type)
  {
    case rw::NO_ALS:
      return "NONE";

    case rw::ODALS:
      return "ODALS";

    case rw::MALSF:
      return "MALSF";

    case rw::MALSR:
      return "MALSR";

    case rw::SSALF:
      return "SSALF";

    case rw::SSALR:
      return "SSALR";

    case rw::ALSF1:
      return "ALSF1";

    case rw::ALSF2:
      return "ALSF2";

    case rw::RAIL:
      return "RAIL";

    case rw::CALVERT:
      return "CALVERT";

    case rw::CALVERT2:
      return "CALVERT2";

    case rw::MALS:
      return "MALS";

    case rw::SALS:
      return "SALS";

    case rw::SSALS:
      return "SSALS";

    case rw::SALSF:
      return "SALSF";
  }
  qWarning().nospace().noquote() << "Unknown runway ALS type " << type;
  return "";
}

enum AppLightFlags
{
  ALS_SYSTEM_MASK = 0x1f,
  ENDLIGHTS = 0x20,
  REIL = 0x40,
  TOUCHDOWN = 0x80

};

RunwayAppLights::RunwayAppLights(BinaryStream *bs)
  : Record(bs)
{
  int flags = bs->readByte();
  numStrobes = bs->readByte();

  system = static_cast<rw::ApproachLightSystem>(flags & ALS_SYSTEM_MASK);
  endlights = (flags & ENDLIGHTS) == ENDLIGHTS;
  reils = (flags & REIL) == REIL;
  touchdown = (flags & TOUCHDOWN) == TOUCHDOWN;
}

QDebug operator<<(QDebug out, const RunwayAppLights& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record)
  << " AppLights[system " << RunwayAppLights::appLightSystemToStr(record.system)
  << ", hasEndlights " << record.endlights
  << ", hasReils " << record.reils
  << ", hasTouchdown " << record.touchdown
  << ", numStrobes " << record.numStrobes
  << "]";

  return out;
}

RunwayAppLights::~RunwayAppLights()
{
}

} // namespace bgl
} // namespace fs
} // namespace atools
