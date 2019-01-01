/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel albar965@mailbox.org
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

#include "apronedgelight.h"

#include "fs/bgl/converter.h"
#include "io/binarystream.h"
#include "fs/navdatabaseoptions.h"

namespace atools {
namespace fs {
namespace bgl {

ApronEdgeLight::ApronEdgeLight(const atools::fs::NavDatabaseOptions *options, atools::io::BinaryStream *bs)
  : bgl::Record(options, bs)
{
  bs->skip(2); // unknown

  int numVertices = bs->readShort();
  int numEdges = bs->readShort();

  // 0xFF0000FF: Unknown, probably color of lights
  // 0x3F800000: Unknown (value 1)
  // 0x44480000: Unknown (value 800)
  bs->skip(12);

  if(options->isIncludedNavDbObject(type::GEOMETRY))
  {
    for(int i = 0; i < numVertices; i++)
      vertices.append(BglPosition(bs));

    for(int i = 0; i < numEdges; i++)
    {
      bs->skip(4); // FLOAT Unknown (value 60.96)
      edges.append(bs->readShort()); // Start
      edges.append(bs->readShort()); // End
    }
  }
}

ApronEdgeLight::~ApronEdgeLight()
{

}

QDebug operator<<(QDebug out, const ApronEdgeLight& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record) << " ApronLight[";
  out << record.vertices;
  out << record.edges;
  out << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
