/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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

#include "fs/bgl/ap/rw/runwayapplights.h"
#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

QString RunwayApproachLights::appLightSystemToStr(rw::ApproachLightSystem type)
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
  qWarning().nospace().noquote() << "Invalid runway ALS type " << type;
  return "INVALID";
}

enum AppLightFlags
{
  ALS_SYSTEM_MASK = 0x1f,
  ENDLIGHTS = 0x20,
  REIL = 0x40,
  TOUCHDOWN = 0x80

};

RunwayApproachLights::RunwayApproachLights()
  : system(atools::fs::bgl::rw::NO_ALS), endlights(false), reils(false), touchdown(false), numStrobes(0)
{
}

RunwayApproachLights::RunwayApproachLights(const NavDatabaseOptions *options, BinaryStream *bs)
  : Record(options, bs)
{
  int flags = bs->readUByte();
  numStrobes = bs->readUByte();

  system = static_cast<rw::ApproachLightSystem>(flags & ALS_SYSTEM_MASK);
  endlights = (flags & ENDLIGHTS) == ENDLIGHTS;
  reils = (flags & REIL) == REIL;
  touchdown = (flags & TOUCHDOWN) == TOUCHDOWN;
}

QDebug operator<<(QDebug out, const RunwayApproachLights& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record)
  << " AppLights[system " << RunwayApproachLights::appLightSystemToStr(record.system)
  << ", hasEndlights " << record.endlights
  << ", hasReils " << record.reils
  << ", hasTouchdown " << record.touchdown
  << ", numStrobes " << record.numStrobes
  << "]";

  return out;
}

RunwayApproachLights::~RunwayApproachLights()
{
}

} // namespace bgl
} // namespace fs
} // namespace atools
