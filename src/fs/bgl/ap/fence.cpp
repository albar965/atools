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

#include "fence.h"

#include "fs/bgl/converter.h"
#include "io/binarystream.h"
#include "fs/bgl/recordtypes.h"
#include "fs/navdatabaseoptions.h"

namespace atools {
namespace fs {
namespace bgl {

Fence::Fence(const atools::fs::NavDatabaseOptions *options, atools::io::BinaryStream *bs)
  : bgl::Record(options, bs)
{
  int numVertices = bs->readShort();

  bs->skip(32); // instance ID and profile

  if(options->isIncludedNavDbObject(type::GEOMETRY))
    for(int i = 0; i < numVertices; i++)
      vertices.append(BglPosition(bs));
}

Fence::~Fence()
{

}

fence::Type Fence::getType() const
{
  // Use record ID to find type
  if(id == rec::FENCE_BLAST)
    return fence::BLAST;
  else if(id == rec::FENCE_BOUNDARY)
    return fence::BOUNDARY;
  else
    return fence::UNKNOWN;
}

QString Fence::fenceTypeToStr(fence::Type type)
{
  switch(type)
  {
    case atools::fs::bgl::fence::UNKNOWN:
      return "UNKNOWN";

    case atools::fs::bgl::fence::BLAST:
      return "BLAST";

    case atools::fs::bgl::fence::BOUNDARY:
      return "BOUNDARY";
  }
  qWarning().nospace().noquote() << "Invalid fence type " << type;
  return "INVALID";
}

QDebug operator<<(QDebug out, const Fence& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record)
  << " Fence[" << endl;
  out << record.vertices;
  out << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
