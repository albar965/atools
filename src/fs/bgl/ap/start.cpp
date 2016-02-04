/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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
    case atools::fs::bgl::start::RUNWAY:
      return "RUNWAY";

    case atools::fs::bgl::start::WATER:
      return "WATER";

    case atools::fs::bgl::start::HELIPAD:
      return "HELIPAD";
  }
  qWarning().nospace().noquote() << "Unknown START type " << type;
  return QString();
}

Start::Start()
{
}

Start::Start(const BglReaderOptions *options, BinaryStream *bs)
  : Record(options, bs)
{
  runwayNumber = bs->readUByte();

  int flags = bs->readUByte();
  runwayDesignator = flags & 0x0f;
  type = static_cast<start::StartType>((flags >> 4) & 0xf);
  position = BglPosition(bs, true, 1000.f);
  heading = bs->readFloat();// TODO wiki heading is float degress
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

Start::~Start()
{
}

} // namespace bgl
} // namespace fs
} // namespace atools
