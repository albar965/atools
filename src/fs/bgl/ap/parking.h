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

#ifndef ATOOLS_BGL_AP_PARKING_H
#define ATOOLS_BGL_AP_PARKING_H

#include "fs/bgl/bglposition.h"
#include "fs/bgl/record.h"

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
  UNKNOWN = 0x00,
  RAMP_GA = 0x01,
  RAMP_GA_SMALL = 0x02,
  RAMP_GA_MEDIUM = 0x03,
  RAMP_GA_LARGE = 0x04,
  RAMP_CARGO = 0x05,
  RAMP_MIL_CARGO = 0x06,
  RAMP_MIL_COMBAT = 0x07,
  GATE_SMALL = 0x08,
  GATE_MEDIUM = 0x09,
  GATE_HEAVY = 0x0a,
  DOCK_GA = 0x0b,
  FUEL = 0x0c,
  VEHICLES = 0x0d,

  RAMP_GA_EXTRA = 0x0e,
  GATE_EXTRA = 0x0f,
  MSFS_2024_UNKNOWN = 0x10 // Unknown parking type. Most likely a gate
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

enum ParkingNameSuffix
{
  SUFFIX_NONE = 0x00,
  SUFFIX_A = 0x0c,
  SUFFIX_B = 0x0d,
  SUFFIX_C = 0x0e,
  SUFFIX_D = 0x0f,
  SUFFIX_E = 0x10,
  SUFFIX_F = 0x11,
  SUFFIX_G = 0x12,
  SUFFIX_H = 0x13,
  SUFFIX_I = 0x14,
  SUFFIX_J = 0x15,
  SUFFIX_K = 0x16,
  SUFFIX_L = 0x17,
  SUFFIX_M = 0x18,
  SUFFIX_N = 0x19,
  SUFFIX_O = 0x1a,
  SUFFIX_P = 0x1b,
  SUFFIX_Q = 0x1c,
  SUFFIX_R = 0x1d,
  SUFFIX_S = 0x1e,
  SUFFIX_T = 0x1f,
  SUFFIX_U = 0x20,
  SUFFIX_V = 0x21,
  SUFFIX_W = 0x22,
  SUFFIX_X = 0x23,
  SUFFIX_Y = 0x24,
  SUFFIX_Z = 0x25
};

} // namespace ap

inline bool isGate(bgl::ap::ParkingType type)
{
  return type == atools::fs::bgl::ap::GATE_HEAVY || type == atools::fs::bgl::ap::GATE_MEDIUM || type == atools::fs::bgl::ap::GATE_SMALL ||
         type == atools::fs::bgl::ap::GATE_EXTRA;
}

inline bool isRamp(bgl::ap::ParkingType type)
{
  return type == atools::fs::bgl::ap::RAMP_GA || type == atools::fs::bgl::ap::RAMP_GA_LARGE ||
         type == atools::fs::bgl::ap::RAMP_GA_MEDIUM || type == atools::fs::bgl::ap::RAMP_GA_SMALL ||
         type == atools::fs::bgl::ap::RAMP_GA_EXTRA;
}

inline bool isCargo(bgl::ap::ParkingType type)
{
  return type == atools::fs::bgl::ap::RAMP_CARGO;
}

inline bool isMilCargo(bgl::ap::ParkingType type)
{
  return type == atools::fs::bgl::ap::RAMP_MIL_CARGO;
}

inline bool isMilCombat(bgl::ap::ParkingType type)
{
  return type == atools::fs::bgl::ap::RAMP_MIL_COMBAT;
}

/*
 * Parking spot. Subrecord of airport. Includes fuel and vehicle parking.s
 */
class Parking
{
public:
  Parking();
  Parking(atools::io::BinaryStream *stream, atools::fs::bgl::StructureType structureType);

  /*
   * @return true if parking is a gate
   */
  bool isGate() const
  {
    return atools::fs::bgl::isGate(type);
  }

  /*
   * @return true if parking is a general aviation ramp
   */
  bool isGaRamp() const
  {
    return atools::fs::bgl::isRamp(type);
  }

  /*
   * @return true if parking is a cargo or military cargo ramp
   */
  bool isCargo() const
  {
    return atools::fs::bgl::isCargo(type) || atools::fs::bgl::isMilCargo(type);
  }

  /*
   * @return true if parking is military cargo or military combat
   */
  bool isMilitary() const
  {
    return atools::fs::bgl::isMilCargo(type) || atools::fs::bgl::isMilCombat(type);
  }

  bool isFuel() const
  {
    return type == ap::FUEL;
  }

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

  atools::fs::bgl::ap::ParkingNameSuffix getSuffix() const
  {
    return suffix;
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
  static QString parkingSuffixToStr(atools::fs::bgl::ap::ParkingNameSuffix type);

  bool isValid() const
  {
    return position.isValid();
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Parking& record);

  friend class atools::fs::bgl::Airport;

  atools::fs::bgl::ap::ParkingType type = atools::fs::bgl::ap::UNKNOWN;
  atools::fs::bgl::ap::ParkingName name = atools::fs::bgl::ap::NO_PARKING;
  atools::fs::bgl::ap::PushBack pushBack = atools::fs::bgl::ap::NONE;
  atools::fs::bgl::ap::ParkingNameSuffix suffix = atools::fs::bgl::ap::SUFFIX_NONE;

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
