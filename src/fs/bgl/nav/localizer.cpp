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

#include "fs/bgl/nav/localizer.h"

#include "fs/bgl/converter.h"
#include "fs/bgl/recordtypes.h"
#include "fs/navdatabaseoptions.h"
#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

Localizer::Localizer(const NavDatabaseOptions *options, BinaryStream *stream, float magVar)
  : Record(options, stream)
{
  runwayNumber = stream->readUByte(); // Not set
  runwayDesignator = stream->readUByte(); // Not set
  heading = stream->readFloat();

  if(id == rec::LOCALIZER_MSFS2024)
    // MSFS 2024 gives heading in magnetic instead of true
    heading += magVar;

  // Limit width
  width = std::min(stream->readFloat(), 20.f);
}

Localizer::~Localizer()
{
}

QString Localizer::getRunwayName() const
{
  return converter::runwayToStr(runwayNumber, runwayDesignator);
}

QDebug operator<<(QDebug out, const Localizer& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record)
                          << " Localizer["
                          << "runway " << record.getRunwayName()
                          << ", heading " << record.heading
                          << ", width " << record.width
                          << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
