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

#ifndef ATOOLS_POINT3D_H
#define ATOOLS_POINT3D_H

#include <QTypeInfo>
#include <cmath>

namespace atools {
namespace geo {

/*
 * Point representing earth coordinates in a 3D cartesian system.
 * Earth center is zero-origin and units are meter.
 * Max values are earth radius in meter.
 *
 * Note that distance calculations are more efficient in this system
 * instead of using the havesince formula.
 */
/*  *INDENT-OFF* */
  // Coordinate axes:
  //   | Z
  //   |
  //   |___ Y
  //  /
  // / X
/* *INDENT-ON* */
class Point3D
{
public:
  Point3D() : x(0.f), y(0.f), z(0.f)
  {
  }

  explicit Point3D(float xParam, float yParam, float zParam)  : x(xParam), y(yParam), z(zParam)
  {
  }

  /* Initialize point from an array which must have three elements */
  explicit Point3D(float *coords)  : x(coords[0]), y(coords[1]), z(coords[2])
  {
  }

  /* true if initialized, i.e. coordinates are not null (earth center) */
  bool isValid() const
  {
    // Not in earth center which is default invalid value
    return std::abs(x) + std::abs(y) + std::abs(z) > 0.f;
  }

  /* Square comparable (manhattan) distance for coordinates. Fastest calculation resulting in squared units. */
  float comparableDistance(const Point3D& p2)const
  {
    float xdiff = p2.x - this->x, ydiff = p2.y - this->y, zdiff = p2.z - this->z;
    return xdiff * xdiff + ydiff * ydiff + zdiff * zdiff;
  }

  static float comparableDistance(float *p1, float *p2)
  {
    float xdiff = p2[0] - p1[0], ydiff = p2[1] - p1[1], zdiff = p2[2] - p1[2];
    return xdiff * xdiff + ydiff * ydiff + zdiff * zdiff;
  }

  /* Direct tunnel-though distance in meters. Always lower than GC distance and only accurate for small distances.
   * Second fastest calculation. */
  float directDistanceMeter(const Point3D& p2)const
  {
    return std::sqrt(comparableDistance(p2));
  }

  static float directDistanceMeter(float *p1, float *p2)
  {
    return std::sqrt(comparableDistance(p1, p2));
  }

  /* Precise GC distance in meters. Faster calculation than haversine formula but slowest of all for Point3D. */
  float gcDistanceMeter(const Point3D& p2)const
  {
    return RADIUS2 * std::asin(std::min(1.f, std::sqrt(comparableDistance(p2)) * INV_RADIUS2));
  }

  static float gcDistanceMeter(float *p1, float *p2)
  {
    return RADIUS2 * std::asin(std::min(1.f, std::sqrt(comparableDistance(p1, p2)) * INV_RADIUS2));
  }

  float getX() const
  {
    return x;
  }

  float getY() const
  {
    return y;
  }

  float getZ() const
  {
    return z;
  }

  void set(float xParam, float yParam, float zParam)
  {
    x = xParam;
    y = yParam;
    z = zParam;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::geo::Point3D& pt);

  float x, y, z;
  static constexpr float RADIUS2 = 6371.f * 1000.f * 2.f; // 2∗average  radius  of  earth in km
  static constexpr float INV_RADIUS2 = 1.0f / RADIUS2;
};

} // namespace geo
} // namespace atools

Q_DECLARE_TYPEINFO(atools::geo::Point3D, Q_PRIMITIVE_TYPE);

#endif // ATOOLS_POINT3D_H
