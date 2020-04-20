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

#include "apron.h"

#include "fs/bgl/converter.h"
#include "io/binarystream.h"
#include "fs/navdatabaseoptions.h"

namespace atools {
namespace fs {
namespace bgl {

Apron::Apron(const atools::fs::NavDatabaseOptions *options, atools::io::BinaryStream *bs, StructureType structureType)
  : bgl::Record(options, bs)
{
  surface = static_cast<rw::Surface>(bs->readUByte() & rw::SURFACE_MASK);

  if(structureType == STRUCT_P3DV5)
    bs->skip(21);

  int numVertices = bs->readShort();

  if(options->isIncludedNavDbObject(type::GEOMETRY))
    for(int i = 0; i < numVertices; i++)
      vertices.append(BglPosition(bs));
}

Apron::~Apron()
{

}

QDebug operator<<(QDebug out, const Apron& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record)
                          << " Runway[surface " << Runway::surfaceToStr(record.surface) << "/"
                          << Runway::surfaceToStr(record.surface) << endl;
  out << record.vertices;
  out << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
