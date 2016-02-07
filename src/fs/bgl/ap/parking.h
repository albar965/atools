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

#ifndef BGL_AP_PARKING_H_
#define BGL_AP_PARKING_H_

#include "fs/bgl/bglposition.h"

#include <QString>

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {

namespace ap {

enum ParkingType
{
  UNKNOWN_PARKING = 0x0,
  RAMP_GA = 0x1,
  RAMP_GA_SMALL = 0x2,
  RAMP_GA_MEDIUM = 0x3,
  RAMP_GA_LARGE = 0x4,
  RAMP_CARGO = 0x5,
  RAMP_MIL_CARGO = 0x6,
  RAMP_MIL_COMBAT = 0x7,
  GATE_SMALL = 0x8,
  GATE_MEDIUM = 0x9,
  GATE_HEAVY = 0xa,
  DOCK_GA = 0xb,
  FUEL = 0xc, // wiki error reported
  VEHICLES = 0xd // wiki error reported
};

enum PushBack
{
  NONE = 0,
  LEFT = 1,
  RIGHT = 2,
  BOTH = 3
};

enum ParkingName
{
  NO_PARKING = 0x00,
  PARKING = 0x01,
  N_PARKING = 0x02,
  NE_PARKING = 0x03,
  E_PARKING = 0x04,
  SE_PARKING = 0x05,
  S_PARKING = 0x06,
  SW_PARKING = 0x07,
  W_PARKING = 0x08,
  NW_PARKING = 0x09,
  GATE = 0x0a,
  DOCK = 0x0b,
  GATE_A = 0x0c,
  GATE_B = 0x0d,
  GATE_C = 0x0e,
  GATE_D = 0x0f,
  GATE_E = 0x10,
  GATE_F = 0x11,
  GATE_G = 0x12,
  GATE_H = 0x13,
  GATE_I = 0x14,
  GATE_J = 0x15,
  GATE_K = 0x16,
  GATE_L = 0x17,
  GATE_M = 0x18,
  GATE_N = 0x19,
  GATE_O = 0x1a,
  GATE_P = 0x1b,
  GATE_Q = 0x1c,
  GATE_R = 0x1d,
  GATE_S = 0x1e,
  GATE_T = 0x1f,
  GATE_U = 0x20,
  GATE_V = 0x21,
  GATE_W = 0x22,
  GATE_X = 0x23,
  GATE_Y = 0x24,
  GATE_Z = 0x25
};

} // namespace ap

class Parking
{
public:
  Parking(atools::io::BinaryStream *bs);
  virtual ~Parking();

  float getHeading() const
  {
    return heading;
  }

  int getNumber() const
  {
    return number;
  }

  float getRadius() const
  {
    return radius;
  }

  atools::fs::bgl::ap::ParkingType getType() const
  {
    return type;
  }

  const atools::fs::bgl::BglPosition& getPosition() const
  {
    return position;
  }

  atools::fs::bgl::ap::ParkingName getName() const
  {
    return name;
  }

  atools::fs::bgl::ap::PushBack getPushBack() const
  {
    return pushBack;
  }

  static QString parkingTypeToStr(atools::fs::bgl::ap::ParkingType type);
  static QString parkingNameToStr(atools::fs::bgl::ap::ParkingName type);
  static QString pushBackToStr(atools::fs::bgl::ap::PushBack type);

  bool hasJetway() const
  {
    return jetway;
  }

  void setHasJetway(bool value)
  {
    jetway = value;
  }

  const QStringList& getAirlineCodes() const
  {
    return airlineCodes;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Parking& record);

  atools::fs::bgl::ap::ParkingType type;
  atools::fs::bgl::ap::ParkingName name;
  atools::fs::bgl::ap::PushBack pushBack;
  int number;
  float radius, heading;
  atools::fs::bgl::BglPosition position;
  bool jetway;
  QStringList airlineCodes;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_AP_PARKING_H_ */
