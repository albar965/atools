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

#ifndef ATOOLS_BGL_AIRPORTHELIPAD_H
#define ATOOLS_BGL_AIRPORTHELIPAD_H

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

namespace helipad {
enum HelipadType
{
  NONE = 0,
  H = 1,
  SQUARE = 2,
  CIRCLE = 3,
  MEDICAL = 4
};

} // namespace helipad

/*
 * Helicopter landing pad. Subrecord of airport.
 */
class Helipad :
  public atools::fs::bgl::Record
{
public:
  Helipad();
  Helipad(const atools::fs::NavDatabaseOptions *options, atools::io::BinaryStream *bs);
  virtual ~Helipad();

  static QString helipadTypeToStr(atools::fs::bgl::helipad::HelipadType type);

  atools::fs::bgl::Surface getSurface() const
  {
    return surface;
  }

  atools::fs::bgl::helipad::HelipadType getType() const
  {
    return type;
  }

  const atools::fs::bgl::BglPosition& getPosition() const
  {
    return position;
  }

  /*
   * @return length in meter
   */
  float getLength() const
  {
    return length;
  }

  /*
   * @return width in meter
   */
  float getWidth() const
  {
    return width;
  }

  /*
   * @return heading degrees true
   */
  float getHeading() const
  {
    return heading;
  }

  bool isClosed() const
  {
    return closed;
  }

  /*
   * @return true if helipad should be drawn without pavement
   */
  bool isTransparent() const
  {
    return transparent;
  }

  int getStartIndex() const
  {
    return startIndex;
  }

  void setStartIndex(int value)
  {
    startIndex = value;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Helipad& record);

  atools::fs::bgl::Surface surface = atools::fs::bgl::UNKNOWN;
  atools::fs::bgl::helipad::HelipadType type = atools::fs::bgl::helipad::NONE;
  atools::fs::bgl::BglPosition position;
  float length = 0.f, width = 0.f, heading = 0.f;
  int startIndex = 0;
  bool transparent = false, closed = false;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_AIRPORTHELIPAD_H
