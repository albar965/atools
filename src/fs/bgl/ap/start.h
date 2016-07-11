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

#ifndef ATOOLS_BGL_AIRPORTSTART_H
#define ATOOLS_BGL_AIRPORTSTART_H

#include "fs/bgl/record.h"
#include "fs/bgl/ap/rw/runway.h"

#include <QString>

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {

namespace start {
enum StartType
{
  RUNWAY = 1,
  WATER = 2,
  HELIPAD = 3 // TOOD fix in wiki
};

} // namespace start

/*
 * Runway start position. Subrecord of airport.
 */
class Start :
  public atools::fs::bgl::Record
{
public:
  Start(const atools::fs::BglReaderOptions *options, atools::io::BinaryStream *bs);
  virtual ~Start();

  /*
   * @return full runway name like "12C" or 06R"
   */
  QString getRunwayName() const;

  const atools::fs::bgl::BglPosition& getPosition() const
  {
    return position;
  }

  /*
   * @return heading in degrees true
   */
  float getHeading() const
  {
    return heading;
  }

  atools::fs::bgl::start::StartType getType() const
  {
    return type;
  }

  static QString startTypeToStr(atools::fs::bgl::start::StartType type);

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Start& record);

  atools::fs::bgl::start::StartType type;
  int runwayNumber, runwayDesignator;
  atools::fs::bgl::BglPosition position;
  float heading = 0.f;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_AIRPORTSTART_H
