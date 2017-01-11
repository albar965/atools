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

#include "fs/bgl/ap/del/deletecom.h"
#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

DeleteCom::DeleteCom(const NavDatabaseOptions *options, BinaryStream *bs)
  : BglBase(options, bs)
{
  unsigned int flags = bs->readUInt();
  type = static_cast<com::ComType>((flags >> 28) & 0xf);
  frequency = (flags & 0x0fffffff) / 1000;
}

QDebug operator<<(QDebug out, const DeleteCom& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const BglBase&>(record)
  << "DeleteCom[type " << record.type
  << ", frequency " << record.frequency << "]";
  return out;
}

DeleteCom::~DeleteCom()
{
}

} // namespace bgl
} // namespace fs
} // namespace atools
