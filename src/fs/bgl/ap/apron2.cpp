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

#include "apron2.h"

#include "io/binarystream.h"
#include "fs/navdatabaseoptions.h"

namespace atools {
namespace fs {
namespace bgl {

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
using Qt::endl;
#endif

Apron2::Apron2(const atools::fs::NavDatabaseOptions *options, atools::io::BinaryStream *bs, StructureType structureType)
  : bgl::Record(options, bs)
{
  surface = static_cast<Surface>(bs->readUByte() & SURFACE_MASK);

  int flags = bs->readUByte();
  drawSurface = (flags & 1) == 1;
  drawDetail = (flags & 2) == 2;

  if(structureType == STRUCT_P3DV4 || structureType == STRUCT_P3DV5)
    // Skip P3D material set GUID for seasons
    bs->skip(16);

  if(structureType == STRUCT_P3DV5)
    bs->skip(4);

  int numVertices = bs->readShort();
  int numTriangles = bs->readShort();

  if(options->isIncludedNavDbObject(type::GEOMETRY))
  {
    for(int i = 0; i < numVertices; i++)
      vertices.append(BglPosition(bs));

    for(int i = 0; i < numTriangles; i++)
    {
      triangles.append(bs->readUShort());
      triangles.append(bs->readUShort());
      triangles.append(bs->readUShort());
    }
  }
}

Apron2::~Apron2()
{

}

QDebug operator<<(QDebug out, const Apron2& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record)
                          << " Apron2[surface " << surface::surfaceToDbStr(record.surface) << "/"
                          << surface::surfaceToDbStr(record.surface)
                          << ", drawSurface " << record.drawSurface
                          << ", drawDetail " << record.drawDetail << endl;
  out << record.vertices;
  out << record.triangles;
  out << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
