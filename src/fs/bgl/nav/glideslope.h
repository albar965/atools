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

#ifndef ATOOLS_BGL_NAV_GLIDESLOPE_H
#define ATOOLS_BGL_NAV_GLIDESLOPE_H

#include "fs/bgl/record.h"
#include "fs/bgl/bglposition.h"

namespace atools {
namespace fs {
namespace bgl {

/*
 * Glideslope is an optional subrecord of ILS (which is a localizer if GS is missing)
 */
class Glideslope :
  public atools::fs::bgl::Record
{
public:
  Glideslope(const atools::fs::NavDatabaseOptions *options, atools::io::BinaryStream *bs);
  virtual ~Glideslope() override;

  /*
   * @return Glideslope pitch in degree
   */
  float getPitch() const
  {
    return pitch;
  }

  /*
   * @return Position of the glideslope transmitter
   */
  const atools::fs::bgl::BglPosition& getPosition() const
  {
    return position;
  }

  /*
   * @return Glideslope range in meter
   */
  float getRange() const
  {
    return range;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Glideslope& record);

  atools::fs::bgl::BglPosition position;
  float range, pitch;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_NAV_GLIDESLOPE_H
