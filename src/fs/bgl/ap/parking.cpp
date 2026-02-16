/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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
      return QStringLiteral("UNKNOWN");

    case ap::RAMP_GA:
      return QStringLiteral("RGA");

    case ap::RAMP_GA_SMALL:
      return QStringLiteral("RGAS");

    case ap::RAMP_GA_MEDIUM:
      return QStringLiteral("RGAM");

    case ap::RAMP_GA_LARGE:
      return QStringLiteral("RGAL");

    case ap::RAMP_CARGO:
      return QStringLiteral("RC");

    case ap::RAMP_MIL_CARGO:
      return QStringLiteral("RMC");

    case ap::RAMP_MIL_COMBAT:
      return QStringLiteral("RMCB");

    case ap::GATE_SMALL:
      return QStringLiteral("GS");

    case ap::GATE_MEDIUM:
      return QStringLiteral("GM");

    case ap::GATE_HEAVY:
      return QStringLiteral("GH");

    case ap::DOCK_GA:
      return QStringLiteral("DGA");

    case ap::FUEL:
      return QStringLiteral("FUEL");

    case ap::VEHICLES:
      return QStringLiteral("V");

    case ap::RAMP_GA_EXTRA:
      return QStringLiteral("RE");

    case ap::GATE_EXTRA:
      return QStringLiteral("GE");

    case ap::MSFS_2024_UNKNOWN:
      return QStringLiteral("UNKN");
  }
  qWarning().nospace().noquote() << "Invalid parking type " << type;
  return QStringLiteral("INVALID");
}

QString Parking::parkingNameToStr(ap::ParkingName type)
{
  switch(type)
  {
    case ap::NO_PARKING:
      return QStringLiteral("NONE");

    case ap::PARKING:
      return QStringLiteral("P");

    case ap::N_PARKING:
      return QStringLiteral("NP");

    case ap::NE_PARKING:
      return QStringLiteral("NEP");

    case ap::E_PARKING:
      return QStringLiteral("EP");

    case ap::SE_PARKING:
      return QStringLiteral("SEP");

    case ap::S_PARKING:
      return QStringLiteral("SP");

    case ap::SW_PARKING:
      return QStringLiteral("SWP");

    case ap::W_PARKING:
      return QStringLiteral("WP");

    case ap::NW_PARKING:
      return QStringLiteral("NWP");

    case ap::GATE:
      return QStringLiteral("G");

    case ap::DOCK:
      return QStringLiteral("D");

    case ap::GATE_A:
      return QStringLiteral("GA");

    case ap::GATE_B:
      return QStringLiteral("GB");

    case ap::GATE_C:
      return QStringLiteral("GC");

    case ap::GATE_D:
      return QStringLiteral("GD");

    case ap::GATE_E:
      return QStringLiteral("GE");

    case ap::GATE_F:
      return QStringLiteral("GF");

    case ap::GATE_G:
      return QStringLiteral("GG");

    case ap::GATE_H:
      return QStringLiteral("GH");

    case ap::GATE_I:
      return QStringLiteral("GI");

    case ap::GATE_J:
      return QStringLiteral("GJ");

    case ap::GATE_K:
      return QStringLiteral("GK");

    case ap::GATE_L:
      return QStringLiteral("GL");

    case ap::GATE_M:
      return QStringLiteral("GM");

    case ap::GATE_N:
      return QStringLiteral("GN");

    case ap::GATE_O:
      return QStringLiteral("GO");

    case ap::GATE_P:
      return QStringLiteral("GP");

    case ap::GATE_Q:
      return QStringLiteral("GQ");

    case ap::GATE_R:
      return QStringLiteral("GR");

    case ap::GATE_S:
      return QStringLiteral("GS");

    case ap::GATE_T:
      return QStringLiteral("GT");

    case ap::GATE_U:
      return QStringLiteral("GU");

    case ap::GATE_V:
      return QStringLiteral("GV");

    case ap::GATE_W:
      return QStringLiteral("GW");

    case ap::GATE_X:
      return QStringLiteral("GX");

    case ap::GATE_Y:
      return QStringLiteral("GY");

    case ap::GATE_Z:
      return QStringLiteral("GZ");
  }
  qWarning().nospace().noquote() << "Invalid parking name " << type;
  return QStringLiteral("INVALID");
}

QString Parking::parkingSuffixToStr(ap::ParkingNameSuffix type)
{
  switch(type)
  {
    case ap::SUFFIX_NONE:
      return QStringLiteral("NONE");

    case ap::SUFFIX_A:
      return QStringLiteral("A");

    case ap::SUFFIX_B:
      return QStringLiteral("B");

    case ap::SUFFIX_C:
      return QStringLiteral("C");

    case ap::SUFFIX_D:
      return QStringLiteral("D");

    case ap::SUFFIX_E:
      return QStringLiteral("E");

    case ap::SUFFIX_F:
      return QStringLiteral("F");

    case ap::SUFFIX_G:
      return QStringLiteral("G");

    case ap::SUFFIX_H:
      return QStringLiteral("H");

    case ap::SUFFIX_I:
      return QStringLiteral("I");

    case ap::SUFFIX_J:
      return QStringLiteral("J");

    case ap::SUFFIX_K:
      return QStringLiteral("K");

    case ap::SUFFIX_L:
      return QStringLiteral("L");

    case ap::SUFFIX_M:
      return QStringLiteral("M");

    case ap::SUFFIX_N:
      return QStringLiteral("N");

    case ap::SUFFIX_O:
      return QStringLiteral("O");

    case ap::SUFFIX_P:
      return QStringLiteral("P");

    case ap::SUFFIX_Q:
      return QStringLiteral("Q");

    case ap::SUFFIX_R:
      return QStringLiteral("R");

    case ap::SUFFIX_S:
      return QStringLiteral("S");

    case ap::SUFFIX_T:
      return QStringLiteral("T");

    case ap::SUFFIX_U:
      return QStringLiteral("U");

    case ap::SUFFIX_V:
      return QStringLiteral("V");

    case ap::SUFFIX_W:
      return QStringLiteral("W");

    case ap::SUFFIX_X:
      return QStringLiteral("X");

    case ap::SUFFIX_Y:
      return QStringLiteral("Y");

    case ap::SUFFIX_Z:
      return QStringLiteral("Z");

  }
  qWarning().nospace().noquote() << "Invalid suffix name " << type;
  return QStringLiteral("INVALID");
}

QString Parking::pushBackToStr(ap::PushBack type)
{
  switch(type)
  {
    case ap::NONE:
      return QStringLiteral("NONE");

    case ap::LEFT:
      return QStringLiteral("L");

    case ap::RIGHT:
      return QStringLiteral("R");

    case ap::BOTH:
      return QStringLiteral("B");
  }
  qWarning().nospace().noquote() << "Invalid parking name " << type;
  return QStringLiteral("INVALID");
}

Parking::Parking()
{

}

Parking::Parking(BinaryStream *stream, atools::fs::bgl::StructureType structureType)
{
  unsigned int flags = stream->readUInt();
  name = static_cast<ap::ParkingName>(flags & 0x3f);
  pushBack = static_cast<ap::PushBack>((flags >> 6) & 0x3);
  type = static_cast<ap::ParkingType>((flags >> 8) & 0xf);
  number = (flags >> 12) & 0xfff;
  int numAirlineCodes = (flags >> 24) & 0xff;

  radius = stream->readFloat();
  heading = stream->readFloat(); // Heading is float degrees

  if(structureType == STRUCT_FSX || structureType == STRUCT_P3DV4 || structureType == STRUCT_P3DV5 || structureType == STRUCT_MSFS)
    stream->skip(16); // teeOffset 1-4 not FS9

  position = BglPosition(stream);

  for(int i = 0; i < numAirlineCodes; i++)
    airlineCodes.append(stream->readString(4, atools::io::LATIN1));

  // Skip material and runway stuff
  if(structureType == STRUCT_P3DV5)
    stream->skip(4);
  else if(structureType == STRUCT_MSFS)
  {
    stream->skip(1);
    suffix = static_cast<ap::ParkingNameSuffix>(stream->readByte());
    stream->skip(18);
  }
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
