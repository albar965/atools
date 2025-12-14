/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

#ifndef WINDTYPES_H
#define WINDTYPES_H

#include "geo/pos.h"
#include "atools.h"

namespace atools {
namespace grib {

Q_DECL_CONSTEXPR static float INVALID_WIND_DIR_VALUE = std::numeric_limits<float>::max();
Q_DECL_CONSTEXPR static float INVALID_WIND_SPEED_VALUE = std::numeric_limits<float>::max();

/* Combines wind speed and direction */
struct Wind
{
  Wind(float windDir, float windSpeed)
  {
    dir = windDir;
    speed = windSpeed;
  }

  Wind()
  {
    dir = speed = 0.f;
  }

  /* Degrees true and knots */
  float dir, speed;

  bool isValid() const
  {
    return speed >= 0.f && speed < 1000.f && dir >= 0.f && dir <= 360.f;
  }

  bool isNull() const
  {
    return speed < 1.f;
  }

  bool operator==(const Wind& other) const
  {
    return atools::almostEqual(speed, other.speed) && atools::almostEqual(dir, other.dir);
  }

  bool operator!=(const Wind& other) const
  {
    return !operator==(other);
  }

};

/* Invalid wind */
const atools::grib::Wind EMPTY_WIND;

/* Combines wind speed and direction at a position */
struct WindPos
{
  atools::geo::Pos pos;
  Wind wind;

  bool isValid() const
  {
    return pos.isValid() && wind.isValid();
  }

  bool operator==(const WindPos& other) const
  {
    return pos == other.pos && wind == other.wind;
  }

  bool operator!=(const WindPos& other) const
  {
    return !operator==(other);
  }

};

/* Invalid wind pos */
const atools::grib::WindPos EMPTY_WIND_POS;

typedef QList<WindPos> WindPosList;

} // namespace grib
} // namespace atools

#endif // WINDTYPES_H
