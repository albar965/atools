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

#ifndef ATOOLS_BGL_RUNWAY_H
#define ATOOLS_BGL_RUNWAY_H

#include "fs/bgl/record.h"
#include "fs/bgl/bglposition.h"

#include "runwayend.h"

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {

namespace rw {

/* Runway pavement markings */
enum RunwayMarkings
{
  EDGES = 1 << 0,
  THRESHOLD = 1 << 1,
  FIXED_DISTANCE = 1 << 2,
  TOUCHDOWN = 1 << 3,
  DASHES = 1 << 4,
  IDENT = 1 << 5,
  PRECISION = 1 << 6,
  EDGE_PAVEMENT = 1 << 7,
  SINGLE_END = 1 << 8,
  PRIMARY_CLOSED = 1 << 9,
  SECONDARY_CLOSED = 1 << 10,
  PRIMARY_STOL = 1 << 11,
  SECONDARY_STOL = 1 << 12,
  ALTERNATE_THRESHOLD = 1 << 13,
  ALTERNATE_FIXEDDISTANCE = 1 << 14,
  ALTERNATE_TOUCHDOWN = 1 << 15,
  ALTERNATE_PRECISION = 1 << 21,
  LEADING_ZERO_IDENT = 1 << 22,
  NO_THRESHOLD_END_ARROWS = 1 << 23
};

/* Light intensity */
enum Light
{
  NO_LIGHT = 0,
  LOW = 1,
  MEDIUM = 2,
  HIGH = 3
};

/* Surface - also used for aprons and taxiways */
enum Surface
{
  CONCRETE = 0x0000,
  GRASS = 0x0001,
  WATER = 0x0002,
  ASPHALT = 0x0004,
  CEMENT = 0x0005, // TODO wiki error report
  CLAY = 0x0007,
  SNOW = 0x0008,
  ICE = 0x0009,
  DIRT = 0x000C,
  CORAL = 0x000D,
  GRAVEL = 0x000E,
  OIL_TREATED = 0x000F,
  STEEL_MATS = 0x0010,
  BITUMINOUS = 0x0011,
  BRICK = 0x0012,
  MACADAM = 0x0013,
  PLANKS = 0x0014,
  SAND = 0x0015,
  SHALE = 0x0016,
  TARMAC = 0x0017,
  UNKNOWN = 0x00FE
};

enum PatternFlags
{
  PRIMARY_TAKEOFF = 1 << 0,
  PRIMARY_LANDING = 1 << 1,
  PRIMARY_PATTERN_RIGHT = 1 << 2,
  SECONDARY_TAKEOFF = 1 << 3,
  SECONDARY_LANDING = 1 << 4,
  SECONDARY_PATTERN_RIGHT = 1 << 5
};

enum LightFlags
{
  EDGE_MASK = 0x3,
  CENTER_MASK = 0xc,
  CENTER_RED = 0x20
};

} // namespace rw

/*
 * Runway. Has a primary and secondary end in a separate class and multiple subrecords like VASI, etc.
 */
class Runway :
  public atools::fs::bgl::Record
{
public:
  Runway(const atools::fs::BglReaderOptions *options, atools::io::BinaryStream *bs,
         const QString& airportIdent);
  virtual ~Runway();

  /*
   * @return Center light intensity/availability
   */
  atools::fs::bgl::rw::Light getCenterLight() const
  {
    return centerLight;
  }

  bool isWater() const;
  bool isSoft() const;
  bool isHard() const;

  /*
   * @return true if it has red center lights at the end
   */
  bool isCenterRed() const
  {
    return centerRed;
  }

  /*
   * @return Edge light intensity/availability
   */
  atools::fs::bgl::rw::Light getEdgeLight() const
  {
    return edgeLight;
  }

  /*
   * @return Runway heading degrees true
   */
  float getHeading() const
  {
    return heading;
  }

  /*
   * @return Runway length in meter
   */
  float getLength() const
  {
    return length;
  }

  atools::fs::bgl::rw::RunwayMarkings getMarkingFlags() const
  {
    return markingFlags;
  }

  /*
   * @return Pattern altitude in meter
   */
  float getPatternAltitude() const
  {
    return patternAltitude;
  }

  /*
   * @return Position of the runway center
   */
  const atools::fs::bgl::BglPosition& getPosition() const
  {
    return position;
  }

  /*
   * @return primary runway end record
   */
  const atools::fs::bgl::RunwayEnd& getPrimary() const
  {
    return primary;
  }

  /*
   * @return secondary runway end record
   */
  const atools::fs::bgl::RunwayEnd& getSecondary() const
  {
    return secondary;
  }

  atools::fs::bgl::rw::Surface getSurface() const
  {
    return surface;
  }

  /*
   * @return Runway width in meter
   */
  float getWidth() const
  {
    return width;
  }

  /*
   * @return Position of the primary runway end
   */
  const atools::geo::Pos& getPrimaryPosition() const
  {
    return primaryPos;
  }

  /*
   * @return Position of the secondary runway end
   */
  const atools::geo::Pos& getSecondaryPosition() const
  {
    return secondaryPos;
  }

  static QString runwayMarkingsToStr(atools::fs::bgl::rw::RunwayMarkings surface);
  static QString lightToStr(atools::fs::bgl::rw::Light surface);
  static QString surfaceToStr(atools::fs::bgl::rw::Surface value);

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Runway& record);

  int readRunwayExtLength();

  atools::fs::bgl::rw::Surface surface;

  atools::fs::bgl::BglPosition position;
  atools::geo::Pos primaryPos, secondaryPos;

  float heading, length, width, patternAltitude;

  atools::fs::bgl::rw::RunwayMarkings markingFlags;
  atools::fs::bgl::rw::LightFlags lightFlags;
  atools::fs::bgl::rw::PatternFlags patternFlags;
  atools::fs::bgl::rw::Light edgeLight, centerLight;
  bool centerRed;

  atools::fs::bgl::RunwayEnd primary, secondary;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_RUNWAY_H
