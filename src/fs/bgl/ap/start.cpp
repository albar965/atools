/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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

#include "fs/bgl/ap/start.h"
#include "io/binarystream.h"
#include "fs/bgl/converter.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

QString Start::startTypeToStr(start::StartType type)
{
  switch(type)
  {
    case start::RUNWAY:
      return QStringLiteral("R");

    case start::WATER:
      return QStringLiteral("W");

    case start::HELIPAD:
      return QStringLiteral("H");

    case start::TRACK:
      return QStringLiteral("T");

    case start::UNKNOWN:
      return QStringLiteral("UNKNOWN");
  }
  qWarning().nospace().noquote() << "Invalid START type " << type;
  return QStringLiteral("INVALID");
}

Start::Start(const NavDatabaseOptions *options, BinaryStream *stream)
  : Record(options, stream)
{
  runwayNumber = stream->readUByte();

  int flags = stream->readUByte();
  runwayDesignator = flags & 0x0f;
  type = static_cast<start::StartType>((flags >> 4) & 0xf);
  position = BglPosition(stream, true, 1000.f);
  heading = stream->readFloat(); // Heading is float degrees
}

Start::~Start()
{
}

QString Start::getRunwayName() const
{
  return converter::runwayToStr(runwayNumber, runwayDesignator);
}

QDebug operator<<(QDebug out, const Start& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record)
                          << " Start[type " << Start::startTypeToStr(record.type)
                          << ", rwy " << record.getRunwayName()
                          << ", heading " << record.heading
                          << ", " << record.position
                          << "]";

  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
