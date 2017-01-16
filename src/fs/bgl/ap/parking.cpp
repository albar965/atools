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

#include "fs/bgl/ap/parking.h"
#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

QString Parking::parkingTypeToStr(ap::ParkingType type)
{
  switch(type)
  {
    case ap::UNKNOWN:
      return "UNKNOWN";

    case ap::RAMP_GA:
      return "RGA";

    case ap::RAMP_GA_SMALL:
      return "RGAS";

    case ap::RAMP_GA_MEDIUM:
      return "RGAM";

    case ap::RAMP_GA_LARGE:
      return "RGAL";

    case ap::RAMP_CARGO:
      return "RC";

    case ap::RAMP_MIL_CARGO:
      return "RMC";

    case ap::RAMP_MIL_COMBAT:
      return "RMCB";

    case ap::GATE_SMALL:
      return "GS";

    case ap::GATE_MEDIUM:
      return "GM";

    case ap::GATE_HEAVY:
      return "GH";

    case ap::DOCK_GA:
      return "DGA";

    case ap::FUEL:
      return "FUEL";

    case ap::VEHICLES:
      return "V";
  }
  qWarning().nospace().noquote() << "Invalid parking type " << type;
  return "INVALID";
}

QString Parking::parkingNameToStr(ap::ParkingName type)
{
  switch(type)
  {
    case ap::NO_PARKING:
      return "NONE";

    case ap::PARKING:
      return "P";

    case ap::N_PARKING:
      return "NP";

    case ap::NE_PARKING:
      return "NEP";

    case ap::E_PARKING:
      return "EP";

    case ap::SE_PARKING:
      return "SEP";

    case ap::S_PARKING:
      return "SP";

    case ap::SW_PARKING:
      return "SWP";

    case ap::W_PARKING:
      return "WP";

    case ap::NW_PARKING:
      return "NWP";

    case ap::GATE:
      return "G";

    case ap::DOCK:
      return "D";

    case ap::GATE_A:
      return "GA";

    case ap::GATE_B:
      return "GB";

    case ap::GATE_C:
      return "GC";

    case ap::GATE_D:
      return "GD";

    case ap::GATE_E:
      return "GE";

    case ap::GATE_F:
      return "GF";

    case ap::GATE_G:
      return "GG";

    case ap::GATE_H:
      return "GH";

    case ap::GATE_I:
      return "GI";

    case ap::GATE_J:
      return "GJ";

    case ap::GATE_K:
      return "GK";

    case ap::GATE_L:
      return "GL";

    case ap::GATE_M:
      return "GM";

    case ap::GATE_N:
      return "GN";

    case ap::GATE_O:
      return "GO";

    case ap::GATE_P:
      return "GP";

    case ap::GATE_Q:
      return "GQ";

    case ap::GATE_R:
      return "GR";

    case ap::GATE_S:
      return "GS";

    case ap::GATE_T:
      return "GT";

    case ap::GATE_U:
      return "GU";

    case ap::GATE_V:
      return "GV";

    case ap::GATE_W:
      return "GW";

    case ap::GATE_X:
      return "GX";

    case ap::GATE_Y:
      return "GY";

    case ap::GATE_Z:
      return "GZ";
  }
  qWarning().nospace().noquote() << "Invalid parking name " << type;
  return "INVALID";
}

QString Parking::pushBackToStr(ap::PushBack type)
{
  switch(type)
  {
    case atools::fs::bgl::ap::NONE:
      return "NONE";

    case atools::fs::bgl::ap::LEFT:
      return "L";

    case atools::fs::bgl::ap::RIGHT:
      return "R";

    case atools::fs::bgl::ap::BOTH:
      return "B";
  }
  qWarning().nospace().noquote() << "Invalid parking name " << type;
  return "INVALID";
}

Parking::Parking()
{

}

Parking::Parking(BinaryStream *bs, rec::AirportRecordType rectype)
{
  unsigned int flags = bs->readUInt();
  name = static_cast<ap::ParkingName>(flags & 0x3f);
  pushBack = static_cast<ap::PushBack>((flags >> 6) & 0x3);
  type = static_cast<ap::ParkingType>((flags >> 8) & 0xf);
  number = (flags >> 12) & 0xfff;
  int numAirlineCodes = (flags >> 24) & 0xff;

  radius = bs->readFloat();
  heading = bs->readFloat(); // TODO wiki heading is float degrees

  if(rectype == rec::TAXI_PARKING) // TODO wiki mention FS9 format
    bs->skip(16);  // teeOffset 1-4

  position = BglPosition(bs);

  for(int i = 0; i < numAirlineCodes; i++)
    airlineCodes.append(bs->readString(4));
}

Parking::~Parking()
{

}

bool Parking::isGate() const
{
  if(type == ap::GATE_SMALL)
    return true;
  else if(type == ap::GATE_MEDIUM)
    return true;
  else if(type == ap::GATE_HEAVY)
    return true;

  return false;
}

bool Parking::isGaRamp() const
{
  if(type == ap::RAMP_GA)
    return true;
  else if(type == ap::RAMP_GA_SMALL)
    return true;
  else if(type == ap::RAMP_GA_MEDIUM)
    return true;
  else if(type == ap::RAMP_GA_LARGE)
    return true;

  return false;
}

bool Parking::isCargo() const
{
  if(type == ap::RAMP_CARGO)
    return true;
  else if(type == ap::RAMP_MIL_CARGO)
    return true;

  return false;
}

bool Parking::isMilitary() const
{
  if(type == ap::RAMP_MIL_CARGO)
    return true;
  else if(type == ap::RAMP_MIL_COMBAT)
    return true;

  return false;
}

QDebug operator<<(QDebug out, const Parking& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << " Parking[type " << Parking::parkingTypeToStr(record.type)
  << ", name " << Parking::parkingNameToStr(record.name)
  << ", number " << record.number
  << ", radius " << record.radius
  << ", heading " << record.heading
  << ", jetway " << record.jetway
  << ", " << record.position
  << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
