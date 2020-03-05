/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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

#include "fs/bgl/ap/jetway.h"
#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

Jetway::Jetway() :
  parkingNumber(0)
{
}

Jetway::Jetway(const NavDatabaseOptions *options, BinaryStream *bs)
  : Record(options, bs)
{
  parkingNumber = bs->readShort();
  gateName = static_cast<atools::fs::bgl::ap::ParkingName>(bs->readShort());
  // WORD Gate Name
  // DWORD Size of the scenery object data to follow(0x40)
  // 64 LibraryObject record
}

QDebug operator<<(QDebug out, const Jetway& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record)
  << " Jetway[ parking index " << record.parkingNumber
  << ", name " << Parking::parkingNameToStr(record.gateName) << "]";

  return out;
}

Jetway::~Jetway()
{
}

} // namespace bgl
} // namespace fs
} // namespace atools
