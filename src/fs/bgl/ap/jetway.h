/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_BGL_AIRPORTJETWAY_H
#define ATOOLS_BGL_AIRPORTJETWAY_H

#include "fs/bgl/record.h"
#include "fs/bgl/ap/parking.h"

#include <QString>

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {

/*
 * Jetway for parking. Subrecord for airport that is used only internally.
 */
class Jetway :
  public atools::fs::bgl::Record
{
public:
  Jetway();
  Jetway(const atools::fs::NavDatabaseOptions *options, atools::io::BinaryStream *bs);
  virtual ~Jetway();

  /*
   * @return number of parking spot that this jetway is assigned to
   */
  int getParkingNumber() const
  {
    return parkingNumber;
  }

  atools::fs::bgl::ap::ParkingName getGateName() const
  {
    return gateName;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Jetway& record);

  int parkingNumber;
  atools::fs::bgl::ap::ParkingName gateName;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_AIRPORTJETWAY_H
