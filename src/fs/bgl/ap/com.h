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

#ifndef ATOOLS_BGL_AIRPORTCOM_H
#define ATOOLS_BGL_AIRPORTCOM_H

#include "fs/bgl/record.h"

#include <QString>

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {

namespace com {
enum ComType
{
  APPROACH = 0x0008,
  ASOS = 0x000D,
  ATIS = 0x0001,
  AWOS = 0x000C,
  CENTER = 0x000A,
  CLEARANCE = 0x0007,
  CLEARANCE_PRE_TAXI = 0x000E,
  CTAF = 0x0004,
  DEPARTURE = 0x0009,
  FSS = 0x000B,
  GROUND = 0x0005,
  MULTICOM = 0x0002,
  NONE = 0x0000,
  REMOTE_CLEARANCE_DELIVERY = 0x000F,
  TOWER = 0x0006,
  UNICOM = 0x0003,

  APPROACH_P3D_V5 = 0x0708,
  ASOS_P3D_V5 = 0x070D,
  ATIS_P3D_V5 = 0x0701,
  AWOS_P3D_V5 = 0x070C,
  CENTER_P3D_V5 = 0x070A,
  CLEARANCE_P3D_V5 = 0x0707,
  CLEARANCE_PRE_TAXI_P3D_V5 = 0x070E,
  CTAF_P3D_V5 = 0x0704,
  DEPARTURE_P3D_V5 = 0x0709,
  FSS_P3D_V5 = 0x070B,
  GROUND_P3D_V5 = 0x0705,
  MULTICOM_P3D_V5 = 0x0702,
  REMOTE_CLEARANCE_DELIVERY_P3D_V5 = 0x070F,
  TOWER_P3D_V5 = 0x0706,
  UNICOM_P3D_V5 = 0x0703
};

} // namespace com

/*
 * Communication frequency. Subrecord of airport or boundary.
 */
class Com :
  public atools::fs::bgl::Record
{
public:
  Com();
  Com(const atools::fs::NavDatabaseOptions *options, atools::io::BinaryStream *stream);
  virtual ~Com() override;

  /*
   * @return Frequency in MHz * 1000
   */
  int getFrequency() const
  {
    return frequency;
  }

  const QString& getName() const
  {
    return name;
  }

  atools::fs::bgl::com::ComType getType() const
  {
    return type;
  }

  static QString comTypeToStr(atools::fs::bgl::com::ComType type);

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Com& record);

  atools::fs::bgl::com::ComType type;
  int frequency;
  QString name;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_AIRPORTCOM_H
