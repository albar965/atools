/*
 * Runway.h
 *
 *  Created on: 20.04.2015
 *      Author: alex
 */

#ifndef BGL_RUNWAY_H_
#define BGL_RUNWAY_H_

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

enum Light
{
  NO_LIGHT = 0,
  LOW = 1,
  MEDIUM = 2,
  HIGH = 3
};

enum Surface
{
  CONCRETE = 0x0000, // TODO report wiki error
  GRASS = 0x0001, // TODO report wiki error
  WATER = 0x0002,
  ASPHALT = 0x0004,
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

class Runway :
  public atools::fs::bgl::Record
{
public:
  Runway(atools::io::BinaryStream *bs, const QString& airportIdent);
  virtual ~Runway();

  atools::fs::bgl::rw::Light getCenterLight() const
  {
    return centerLight;
  }

  bool isCenterRed() const
  {
    return centerRed;
  }

  atools::fs::bgl::rw::Light getEdgeLight() const
  {
    return edgeLight;
  }

  float getHeading() const
  {
    return heading;
  }

  float getLength() const
  {
    return length;
  }

  unsigned int getLightFlags() const
  {
    return lightFlags;
  }

  unsigned int getMarkingFlags() const
  {
    return markingFlags;
  }

  float getPatternAltitude() const
  {
    return patternAltitude;
  }

  unsigned int getPatternFlags() const
  {
    return patternFlags;
  }

  const atools::fs::bgl::BglPosition& getPosition() const
  {
    return position;
  }

  const atools::fs::bgl::RunwayEnd& getPrimary() const
  {
    return primary;
  }

  const atools::fs::bgl::RunwayEnd& getSecondary() const
  {
    return secondary;
  }

  atools::fs::bgl::rw::Surface getSurface() const
  {
    return surface;
  }

  float getWidth() const
  {
    return width;
  }

  static QString runwayMarkingsToStr(atools::fs::bgl::rw::RunwayMarkings surface);
  static QString lightToStr(atools::fs::bgl::rw::Light surface);
  static QString surfaceToStr(atools::fs::bgl::rw::Surface value);

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Runway& record);

  int readRunwayExtLength();

  atools::fs::bgl::rw::Surface surface;

  atools::fs::bgl::BglPosition position;

  float heading, length, width, patternAltitude;

  unsigned int markingFlags, lightFlags, patternFlags;
  atools::fs::bgl::rw::Light edgeLight, centerLight;
  bool centerRed;

  atools::fs::bgl::RunwayEnd primary, secondary;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_RUNWAY_H_ */
