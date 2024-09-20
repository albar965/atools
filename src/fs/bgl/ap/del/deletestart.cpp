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

#include "fs/bgl/ap/del/deletestart.h"
#include "io/binarystream.h"
#include "fs/bgl/converter.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

DeleteStart::DeleteStart(const NavDatabaseOptions *options, BinaryStream *stream)
  : BglBase(options, stream)
{

  runwayNumber = stream->readUByte();

  int flags = stream->readUByte();
  runwayDesignator = stream->readUByte();
  type = static_cast<start::StartType>((flags >> 4) & 0xf);
}

QString DeleteStart::getRunwayName() const
{
  return converter::runwayToStr(runwayNumber, runwayDesignator);
}

QDebug operator<<(QDebug out, const DeleteStart& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const BglBase&>(record)
                          << " Start[type " << Start::startTypeToStr(record.type)
                          << ", rwy " << record.getRunwayName()
                          << "]";
  return out;
}

DeleteStart::~DeleteStart()
{
}

} // namespace bgl
} // namespace fs
} // namespace atools
