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
    case ap::UNKNOWN_PARKING:
      return "UNKNOWN";

    case ap::RAMP_GA:
      return "RAMP_GA";

    case ap::RAMP_GA_SMALL:
      return "RAMP_GA_SMALL";

    case ap::RAMP_GA_MEDIUM:
      return "RAMP_GA_MEDIUM";

    case ap::RAMP_GA_LARGE:
      return "RAMP_GA_LARGE";

    case ap::RAMP_CARGO:
      return "RAMP_CARGO";

    case ap::RAMP_MIL_CARGO:
      return "RAMP_MIL_CARGO";

    case ap::RAMP_MIL_COMBAT:
      return "RAMP_MIL_COMBAT";

    case ap::GATE_SMALL:
      return "GATE_SMALL";

    case ap::GATE_MEDIUM:
      return "GATE_MEDIUM";

    case ap::GATE_HEAVY:
      return "GATE_HEAVY";

    case ap::DOCK_GA:
      return "DOCK_GA";

    case ap::FUEL:
      return "FUEL";

    case ap::VEHICLES:
      return "VEHICLES";
  }
  qWarning().nospace().noquote() << "Unknown parking type " << type;
  return "";
}

QString Parking::parkingNameToStr(ap::ParkingName type)
{
  switch(type)
  {
    case ap::NO_PARKING:
      return "NONE";

    case ap::PARKING:
      return "PARKING";

    case ap::N_PARKING:
      return "N_PARKING";

    case ap::NE_PARKING:
      return "NE_PARKING";

    case ap::E_PARKING:
      return "E_PARKING";

    case ap::SE_PARKING:
      return "SE_PARKING";

    case ap::S_PARKING:
      return "S_PARKING";

    case ap::SW_PARKING:
      return "SW_PARKING";

    case ap::W_PARKING:
      return "W_PARKING";

    case ap::NW_PARKING:
      return "NW_PARKING";

    case ap::GATE:
      return "GATE";

    case ap::DOCK:
      return "DOCK";

    case ap::GATE_A:
      return "GATE_A";

    case ap::GATE_B:
      return "GATE_B";

    case ap::GATE_C:
      return "GATE_C";

    case ap::GATE_D:
      return "GATE_D";

    case ap::GATE_E:
      return "GATE_E";

    case ap::GATE_F:
      return "GATE_F";

    case ap::GATE_G:
      return "GATE_G";

    case ap::GATE_H:
      return "GATE_H";

    case ap::GATE_I:
      return "GATE_I";

    case ap::GATE_J:
      return "GATE_J";

    case ap::GATE_K:
      return "GATE_K";

    case ap::GATE_L:
      return "GATE_L";

    case ap::GATE_M:
      return "GATE_M";

    case ap::GATE_N:
      return "GATE_N";

    case ap::GATE_O:
      return "GATE_O";

    case ap::GATE_P:
      return "GATE_P";

    case ap::GATE_Q:
      return "GATE_Q";

    case ap::GATE_R:
      return "GATE_R";

    case ap::GATE_S:
      return "GATE_S";

    case ap::GATE_T:
      return "GATE_T";

    case ap::GATE_U:
      return "GATE_U";

    case ap::GATE_V:
      return "GATE_V";

    case ap::GATE_W:
      return "GATE_W";

    case ap::GATE_X:
      return "GATE_X";

    case ap::GATE_Y:
      return "GATE_Y";

    case ap::GATE_Z:
      return "GATE_Z";
  }
  qWarning().nospace().noquote() << "Unknown parking name " << type;
  return "";
}





Parking::Parking(BinaryStream *bs)
{
    unsigned int flags = bs->readUInt();
    name = static_cast<ap::ParkingName>(flags & 0x3f);
  type = static_cast<ap::ParkingType>((flags >> 8) & 0xf);
  number = (flags >> 12) & 0xfff;
  int numAirlineCodes = (flags >> 24) & 0xff;

  radius = bs->readFloat();
  heading = bs->readFloat();
  bs->skip(16); // teeOffset 1-4
  position = BglPosition(bs, 1.0f, false);

  for(int i = 0; i < numAirlineCodes; ++i)
    bs->readString(4);
}

Parking::~Parking()
{

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
