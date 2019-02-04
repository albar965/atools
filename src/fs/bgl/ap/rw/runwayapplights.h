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

#ifndef ATOOLS_BGL_RUNWAYAPPLIGHTS_H
#define ATOOLS_BGL_RUNWAYAPPLIGHTS_H

#include "fs/bgl/record.h"

#include <QList>

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {

namespace rw {

enum ApproachLightSystem
{
  NO_ALS = 0x00,
  ODALS = 0x01,
  MALSF = 0x02,
  MALSR = 0x03,
  SSALF = 0x04,
  SSALR = 0x05,
  ALSF1 = 0x06,
  ALSF2 = 0x07,
  RAIL = 0x08,
  CALVERT = 0x09,
  CALVERT2 = 0x0a,
  MALS = 0x0b,
  SALS = 0x0c,
  SSALS = 0x0d,
  SALSF = 0x0e // TODO add to wiki
};

} // namespace rw

/*
 * Runway approach lights.
 */
class RunwayApproachLights :
  public atools::fs::bgl::Record
{
public:
  RunwayApproachLights();
  RunwayApproachLights(const atools::fs::NavDatabaseOptions *options, atools::io::BinaryStream *bs);

  RunwayApproachLights(const atools::fs::bgl::RunwayApproachLights& other)
    : atools::fs::bgl::Record(other)
  {
    this->operator=(other);

  }

  virtual ~RunwayApproachLights();

  RunwayApproachLights& operator=(const atools::fs::bgl::RunwayApproachLights& other)
  {
    system = other.system;
    endlights = other.endlights;
    reils = other.reils;
    touchdown = other.touchdown;
    numStrobes = other.numStrobes;
    return *this;
  }

  /*
   * @return True if it has runway end lights (green from approach, red from runway)
   */
  bool hasEndlights() const
  {
    return endlights;
  }

  /*
   * @return Number of strobes in approach lights
   */
  int getNumStrobes() const
  {
    return numStrobes;
  }

  /*
   * @return true if it has runway end identifier lights
   */
  bool hasReils() const
  {
    return reils;
  }

  atools::fs::bgl::rw::ApproachLightSystem getSystem() const
  {
    return system;
  }

  /*
   * @return true if touchdown zone lighting is available
   */
  bool hasTouchdown() const
  {
    return touchdown;
  }

  static QString appLightSystemToStr(atools::fs::bgl::rw::ApproachLightSystem type);

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::RunwayApproachLights& record);

  atools::fs::bgl::rw::ApproachLightSystem system;
  bool endlights;
  bool reils;
  bool touchdown;
  int numStrobes;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_RUNWAYAPPLIGHTS_H
