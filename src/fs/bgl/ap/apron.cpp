/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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

#include "io/binarystream.h"
#include "fs/navdatabaseoptions.h"

namespace atools {
namespace fs {
namespace bgl {

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
using Qt::hex;
using Qt::dec;
using Qt::endl;
#endif

Apron::Apron(const atools::fs::NavDatabaseOptions *options, atools::io::BinaryStream *stream,
             atools::fs::bgl::rec::AirportRecordType type)
  : bgl::Record(options, stream)
{
  surface = static_cast<Surface>(stream->readUByte() & SURFACE_MASK);

  if(options->getSimulatorType() == atools::fs::FsPaths::MSFS)
  {
    stream->skip(5);
    materialUuid = stream->readUuid();
    stream->skip(16);
  }

  if(type == rec::APRON_FIRST_P3D_V5)
    stream->skip(21);

  if(type == rec::APRON_FIRST_MSFS_NEW)
    stream->skip(4);

  int numVertices = stream->readShort();
  if(options->getSimulatorType() == atools::fs::FsPaths::MSFS)
    stream->skip(2);

  if(options->isIncludedNavDbObject(type::GEOMETRY))
    for(int i = 0; i < numVertices; i++)
      vertices.append(BglPosition(stream));
}

Apron::~Apron()
{

}

QDebug operator<<(QDebug out, const Apron& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record)
                          << " Runway[surface " << surface::surfaceToDbStr(record.surface) << "/"
                          << surface::surfaceToDbStr(record.surface) << endl;
  out << record.vertices;
  out << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
