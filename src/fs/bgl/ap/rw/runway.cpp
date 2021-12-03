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

#include "runway.h"

#include "fs/bgl/recordtypes.h"
#include "fs/bgl/converter.h"
#include "io/binarystream.h"
#include "geo/calculations.h"
#include "fs/navdatabaseoptions.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

QString Runway::runwayMarkingsToStr(rw::RunwayMarkings flags)
{
  QString retval;
  if((flags& rw::EDGES) == rw::EDGES)
    retval += "EDGES,";
  if((flags& rw::THRESHOLD) == rw::THRESHOLD)
    retval += "THRESHOLD,";
  if((flags& rw::FIXED_DISTANCE) == rw::FIXED_DISTANCE)
    retval += "FIXED_DISTANCE,";
  if((flags& rw::TOUCHDOWN) == rw::TOUCHDOWN)
    retval += "TOUCHDOWN,";
  if((flags& rw::DASHES) == rw::DASHES)
    retval += "DASHES,";
  if((flags& rw::IDENT) == rw::IDENT)
    retval += "IDENT,";
  if((flags& rw::PRECISION) == rw::PRECISION)
    retval += "PRECISION,";
  if((flags& rw::EDGE_PAVEMENT) == rw::EDGE_PAVEMENT)
    retval += "EDGE_PAVEMENT,";
  if((flags& rw::SINGLE_END) == rw::SINGLE_END)
    retval += "SINGLE_END,";
  if((flags& rw::PRIMARY_CLOSED) == rw::PRIMARY_CLOSED)
    retval += "PRIMARY_CLOSED,";
  if((flags& rw::SECONDARY_CLOSED) == rw::SECONDARY_CLOSED)
    retval += "SECONDARY_CLOSED,";
  if((flags& rw::PRIMARY_STOL) == rw::PRIMARY_STOL)
    retval += "PRIMARY_STOL,";
  if((flags& rw::SECONDARY_STOL) == rw::SECONDARY_STOL)
    retval += "SECONDARY_STOL,";
  if((flags& rw::ALTERNATE_THRESHOLD) == rw::ALTERNATE_THRESHOLD)
    retval += "ALTERNATE_THRESHOLD,";
  if((flags& rw::ALTERNATE_FIXEDDISTANCE) == rw::ALTERNATE_FIXEDDISTANCE)
    retval += "ALTERNATE_FIXEDDISTANCE,";
  if((flags& rw::ALTERNATE_TOUCHDOWN) == rw::ALTERNATE_TOUCHDOWN)
    retval += "ALTERNATE_TOUCHDOWN,";
  if((flags& rw::ALTERNATE_PRECISION) == rw::ALTERNATE_PRECISION)
    retval += "ALTERNATE_PRECISION,";
  if((flags& rw::LEADING_ZERO_IDENT) == rw::LEADING_ZERO_IDENT)
    retval += "LEADING_ZERO_IDENT,";
  if((flags& rw::NO_THRESHOLD_END_ARROWS) == rw::NO_THRESHOLD_END_ARROWS)
    retval += "NO_THRESHOLD_END_ARROWS,";
  return retval;
}

QString Runway::lightToStr(rw::Light type)
{
  switch(type)
  {
    case rw::NO_LIGHT:
      return "NONE";

    case rw::LOW:
      return "L";

    case rw::MEDIUM:
      return "M";

    case rw::HIGH:
      return "H";
  }
  qWarning().nospace().noquote() << "Invalid runway lights type " << type;
  return "INVALID";
}

Runway::Runway(const NavDatabaseOptions *options, BinaryStream *bs, const QString& airportIdent, StructureType structureType)
  : Record(options, bs)
{
  surface = static_cast<Surface>(bs->readShort() & SURFACE_MASK);
  primary.number = bs->readUByte();
  primary.designator = bs->readUByte();
  secondary.number = bs->readUByte();
  secondary.designator = bs->readUByte();

  primary.ilsIdent = converter::intToIcao(bs->readUInt(), true);
  secondary.ilsIdent = converter::intToIcao(bs->readUInt(), true);

  position = BglPosition(bs, true, 1000.f);

  length = bs->readFloat();
  width = bs->readFloat();
  heading = bs->readFloat(); // Heading is float degrees

  primary.heading = heading;
  secondary.heading = atools::geo::opposedCourseDeg(heading);

  // Calculate runway end positions for drawing
  primary.pos = primaryPos = position.getPos().endpoint(length / 2.f, atools::geo::opposedCourseDeg(heading));
  primary.pos.setAltitude(position.getAltitude());
  secondary.pos = secondaryPos = position.getPos().endpoint(length / 2.f, heading);
  secondary.pos.setAltitude(position.getAltitude());

  patternAltitude = bs->readFloat();

  primary.primaryEnd = true;
  secondary.primaryEnd = false;

  // Read combined flags and set attributes for primary and secondary ends
  markingFlags = static_cast<rw::RunwayMarkings>(bs->readUShort());
  if((markingFlags& rw::PRIMARY_CLOSED) == rw::PRIMARY_CLOSED)
    primary.closedMarkings = true;
  if((markingFlags& rw::SECONDARY_CLOSED) == rw::SECONDARY_CLOSED)
    secondary.closedMarkings = true;
  if((markingFlags& rw::PRIMARY_STOL) == rw::PRIMARY_STOL)
    primary.stolMarkings = true;
  if((markingFlags& rw::SECONDARY_STOL) == rw::SECONDARY_STOL)
    secondary.stolMarkings = true;

  lightFlags = static_cast<rw::LightFlags>(bs->readUByte());
  edgeLight = static_cast<rw::Light>(lightFlags & rw::EDGE_MASK);
  centerLight = static_cast<rw::Light>((lightFlags& rw::CENTER_MASK) >> 2);
  centerRed = (lightFlags& rw::CENTER_RED) == rw::CENTER_RED;

  patternFlags = static_cast<rw::PatternFlags>(bs->readUByte());
  if((patternFlags& rw::PRIMARY_TAKEOFF) == 0)
    primary.takeoff = true;
  if((patternFlags& rw::PRIMARY_LANDING) == 0)
    primary.landing = true;
  if((patternFlags& rw::PRIMARY_PATTERN_RIGHT) == rw::PRIMARY_PATTERN_RIGHT)
    primary.pattern = rw::RIGHT;
  else
    primary.pattern = rw::LEFT;

  if((patternFlags& rw::SECONDARY_TAKEOFF) == 0)
    secondary.takeoff = true;
  if((patternFlags& rw::SECONDARY_LANDING) == 0)
    secondary.landing = true;
  if((patternFlags& rw::SECONDARY_PATTERN_RIGHT) == rw::SECONDARY_PATTERN_RIGHT)
    secondary.pattern = rw::RIGHT;
  else
    secondary.pattern = rw::LEFT;

  if(structureType == STRUCT_P3DV4 || structureType == STRUCT_P3DV5)
    // Skip P3D material set GUID for seasons
    bs->skip(16);
  else if(structureType == STRUCT_MSFS)
  {
    bs->skip(24);
    // UUID for runway material {B037EA38-EDF8-4AE5-B41B-2CA423ADA3EF}
    // Raw 38EA37B0-F8ED-E54A-B41B-2CA423ADA3EF
    materialUuid = bs->readUuid();
    bs->skip(4);
  }

  bool msfs = options->getSimulatorType() == atools::fs::FsPaths::MSFS;

  // Read all subrecords
  while(bs->tellg() < startOffset + size)
  {
    Record r(options, bs);
    rec::RunwayRecordType t = r.getId<rec::RunwayRecordType>();
    if(checkSubRecord(r))
      return;

    switch(t)
    {
      case rec::OFFSET_THRESHOLD_PRIM:
        primary.offsetThreshold = readRunwayExtLength(msfs);
        break;
      case rec::OFFSET_THRESHOLD_SEC:
        secondary.offsetThreshold = readRunwayExtLength(msfs);
        break;

      case rec::BLAST_PAD_PRIM:
        primary.blastPad = readRunwayExtLength(msfs);
        break;
      case rec::BLAST_PAD_SEC:
        secondary.blastPad = readRunwayExtLength(msfs);
        break;

      case rec::OVERRUN_PRIM:
        primary.overrun = readRunwayExtLength(msfs);
        break;
      case rec::OVERRUN_PRIM_MSFS:
        primary.overrun = readRunwayExtLength(false); // New types do not have a material definition
        break;
      case rec::OVERRUN_SEC:
        secondary.overrun = readRunwayExtLength(msfs);
        break;
      case rec::OVERRUN_SEC_MSFS:
        secondary.overrun = readRunwayExtLength(false); // New types do not have a material definition
        break;

      case rec::VASI_PRIM_LEFT:
        r.seekToStart();
        primary.leftVasi = RunwayVasi(options, bs);
        break;
      case rec::VASI_PRIM_RIGHT:
        r.seekToStart();
        primary.rightVasi = RunwayVasi(options, bs);
        break;
      case rec::VASI_SEC_LEFT:
        r.seekToStart();
        secondary.leftVasi = RunwayVasi(options, bs);
        break;
      case rec::VASI_SEC_RIGHT:
        r.seekToStart();
        secondary.rightVasi = RunwayVasi(options, bs);
        break;
      case rec::APP_LIGHTS_PRIM:
      case rec::APP_LIGHTS_PRIM_MSFS:
        r.seekToStart();
        primary.approachLights = RunwayApproachLights(options, bs);
        break;
      case rec::APP_LIGHTS_SEC:
      case rec::APP_LIGHTS_SEC_MSFS:
        r.seekToStart();
        secondary.approachLights = RunwayApproachLights(options, bs);
        break;

      case rec::MSFS_RUNWAY_DEFORMATION:
      case rec::MSFS_RUNWAY_FACILITY_MATERIAL:
        break;

      default:
#ifndef DEBUG_INFORMATION
        // Log unknown types only for other simulators than MSFS since this one comes up with  surprises
        if(opts->getSimulatorType() != atools::fs::FsPaths::SimulatorType::MSFS)
#endif
        qWarning().nospace().noquote() << Q_FUNC_INFO << " Unexpected record type in Runway record 0x"
                                       << hex << t << dec
                                       << " for ident " << airportIdent
                                       << " runway " << primary.getName() << "/" << secondary.getName()
                                       << " offset " << bs->tellg();
    }
    r.seekToEnd();
  }
}

Runway::~Runway()
{
}

// Read runway length after record fields id and size
int Runway::readRunwayExtLength(bool msfs)
{
  bs->readShort(); // surface (same as runway)

  if(msfs)
    bs->skip(16); // Material UUID in MSFS - use same as runway

  int len = static_cast<int>(bs->readFloat());

  bs->readFloat(); // width (same as runway)
  return len;
}

bool Runway::isWater() const
{
  return atools::fs::bgl::surface::isWater(surface);
}

bool Runway::isSoft() const
{
  return atools::fs::bgl::surface::isSoft(surface);
}

bool Runway::isHard() const
{
  return atools::fs::bgl::surface::isHard(surface);
}

QDebug operator<<(QDebug out, const Runway& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record)
                          << " Runway[length " << record.length
                          << ", width " << record.width
                          << ", hdg " << record.heading
                          << ", surface " << surface::surfaceToDbStr(record.surface) << endl
                          << ", primary " << record.primary << endl
                          << ", secondary " << record.secondary << endl
                          << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
