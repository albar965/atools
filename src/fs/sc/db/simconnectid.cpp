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

#include "fs/sc/db/simconnectid.h"

namespace atools {
namespace fs {
namespace sc {
namespace db {

QDebug operator<<(QDebug out, const FacilityId& obj)
{
  QDebugStateSaver saver(out);
  out.noquote().nospace() << "FacilityId["
                          << "ident " << obj.getIdentStr()
                          << ", region " << obj.getRegionStr()
                          << ", type " << obj.getTypeStr() << "]";
  return out;
}

QDebug operator<<(QDebug out, const IcaoId& obj)
{
  QDebugStateSaver saver(out);
  out.noquote().nospace() << "IcaoId[ident " << obj.getIdentStr() << "]";
  return out;
}

} // namespace db
} // namespace sc
} // namespace fs
} // namespace atools
