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

#ifndef ATOOLS_BGL_AP_PARKING_H
#define ATOOLS_BGL_AP_PARKING_H

#include "fs/bgl/bglposition.h"
#include "fs/bgl/recordtypes.h"

#include <QString>
#include <QStringList>

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {

class Airport;

namespace ap {

enum ParkingType
{
  UNKNOWN = 0x0,
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
  FUEL = 0xc,
  VEHICLES = 0xd
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

/*
 * Parking spot. Subrecord of airport. Includes fuel and vehicle parking.s
 */
class Parking
{
public:
  Parking();
  Parking(atools::io::BinaryStream *bs, atools::fs::bgl::rec::AirportRecordType rectype);
  virtual ~Parking();

  /*
   * @return true if parking is a gate
   */
  bool isGate() const;

  /*
   * @return true if parking is a general aviation ramp
   */
  bool isGaRamp() const;

  /*
   * @return true if parking is a cargo or military cargo ramp
   */
  bool isCargo() const;

  /*
   * @return true if parking is military cargo or military combat
   */
  bool isMilitary() const;

  /*
   * @return heading in degree true
   */
  float getHeading() const
  {
    return heading;
  }

  /*
   * @return parking number unique across the airport
   */
  int getNumber() const
  {
    return number;
  }

  /*
   * @return parking radius in meter
   */
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

  /*
   * @return push back direction if available
   */
  atools::fs::bgl::ap::PushBack getPushBack() const
  {
    return pushBack;
  }

  /*
   * @return true if jetway is available
   */
  bool hasJetway() const
  {
    return jetway;
  }

  /*
   * @return list of airline codes for this parking
   */
  const QStringList& getAirlineCodes() const
  {
    return airlineCodes;
  }

  static QString parkingTypeToStr(atools::fs::bgl::ap::ParkingType type);
  static QString parkingNameToStr(atools::fs::bgl::ap::ParkingName type);
  static QString pushBackToStr(atools::fs::bgl::ap::PushBack type);

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Parking& record);

  friend class atools::fs::bgl::Airport;

  atools::fs::bgl::ap::ParkingType type = atools::fs::bgl::ap::UNKNOWN;
  atools::fs::bgl::ap::ParkingName name = atools::fs::bgl::ap::NO_PARKING;
  atools::fs::bgl::ap::PushBack pushBack = atools::fs::bgl::ap::NONE;
  int number = 0;
  float radius = 0.f, heading = 0.f;
  atools::fs::bgl::BglPosition position;
  bool jetway = false;
  QStringList airlineCodes;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_AP_PARKING_H
