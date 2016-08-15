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

#ifndef ATOOLS_GEO_CALCULATIONS_H
#define ATOOLS_GEO_CALCULATIONS_H

#include "atools.h"

#include <cmath>
#include <limits>

#include <QString>

namespace atools {
namespace geo {

enum LineDist
{
  DIST_TO_LINE, /* Distance is to the line */
  DIST_TO_START, /* Distance to start point */
  DIST_TO_END /* Distance to end point */
};

/*
 * Calculates the distance to a line in euclidian coordinate system
 *
 * @param x,y point which distance to the line should be calculated
 * @param x1,y1,x2,y2 start and end coordinates of the line
 * @param lineOnly if true will return only the distance to the line and not the distance to any endpoint
 * @param distType if not null will return the distance type
 * @return distance to line or to an endpoint depending on parameters
 */
float distanceToLine(float x, float y, float x1, float y1, float x2, float y2, bool lineOnly = false,
                     atools::geo::LineDist *distType = nullptr);

/* Square distance */
template<typename TYPE>
Q_DECL_CONSTEXPR int manhattanDistance(TYPE x1, TYPE y1, TYPE x2, TYPE y2)
{
  return std::abs(x1 - x2) + std::abs(y1 - y2);
}

template<>
inline Q_DECL_CONSTEXPR int manhattanDistance<int>(int x1, int y1, int x2, int y2)
{
  return absInt(x1 - x2) + absInt(y1 - y2);
}

inline float manhattanDistanceF(float x1, float y1, float x2, float y2)
{
  return std::abs(x1 - x2) + std::abs(y1 - y2);
}

/* Euclidian distance between points rounded to int */
template<typename TYPE>
Q_DECL_CONSTEXPR int simpleDistance(TYPE x1, TYPE y1, TYPE x2, TYPE y2)
{
  return static_cast<int>(std::round(sqrt(static_cast<double>((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2)))));
}

/* Euclidian distance between points */
template<typename TYPE>
Q_DECL_CONSTEXPR float simpleDistanceF(TYPE x1, TYPE y1, TYPE x2, TYPE y2)
{
  return static_cast<float>(sqrt(static_cast<double>((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2))));
}

/* Temperature from celsius to farenheit */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE degCToDegF(TYPE temp)
{
  return static_cast<TYPE>(1.8 * static_cast<double>(temp) + 32);
}

/* Temperature from farenheit to celsius */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE degFToDegC(TYPE temp)
{
  return static_cast<TYPE>((static_cast<double>(temp) - 32) / 1.8);
}

/* Pressure from millibar to inches Hg */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE mbarToInHg(TYPE press)
{
  return static_cast<TYPE>(static_cast<double>(press) * 0.02953);
}

/* Pressure from inches Hg to millibar */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE inHgToMbar(TYPE press)
{
  return static_cast<TYPE>(static_cast<double>(press) * 33.863753);
}

/* Distance from nautical miles to meters */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE nmToMeter(TYPE nm)
{
  return static_cast<TYPE>(static_cast<double>(nm) * 1852.216);
}

/* Distance from nautical miles to kilometers */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE nmToKm(TYPE nm)
{
  return static_cast<TYPE>(static_cast<double>(nm) * 1852.216 / 1000.);
}

/* Distance from meters to nautical miles */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE meterToNm(TYPE nm)
{
  return static_cast<TYPE>(static_cast<double>(nm) / 1852.216);
}

template<typename TYPE>
Q_DECL_CONSTEXPR TYPE meterToFeet(TYPE value)
{
  return static_cast<TYPE>(3.2808399 * static_cast<double>(value));
}

template<typename TYPE>
Q_DECL_CONSTEXPR TYPE feetToMeter(TYPE value)
{
  return static_cast<TYPE>(0.3048 * static_cast<double>(value));
}

template<typename TYPE>
Q_DECL_CONSTEXPR TYPE feetToNm(TYPE value)
{
  return meterToNm(feetToMeter(value));
}

/* NM to rad (longitude or latitude) */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE nmToRad(TYPE value)
{
  return static_cast<TYPE>(M_PI / (180. * 60.) * static_cast<double>(value));
}

/* meter to rad (longitude or latitude) */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE meterToRad(TYPE value)
{
  return nmToRad(meterToNm(value));
}

/* Degree to rad */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE toRadians(TYPE deg)
{
  return static_cast<TYPE>(static_cast<double>(deg) * 0.017453292519943295769236907684886);
}

/* Rad to degree */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE toDegree(TYPE rad)
{
  return static_cast<TYPE>(static_cast<double>(rad) / 0.017453292519943295769236907684886);
}

/* Get opposed course */
template<typename TYPE>
TYPE opposedCourseDeg(TYPE courseDegree)
{
  double result = static_cast<double>(courseDegree) + 180.;
  while(result > 360.)
    result -= 360.;
  while(result < 360.)
    result += 360.;

  return static_cast<TYPE>(result);
}

/* Normalize course to 0 < course < 360 */
template<typename TYPE>
TYPE normalizeCourse(TYPE courseDegree)
{
  double result = static_cast<double>(courseDegree);
  while(result > 360.)
    result = result - 360.;
  while(result < 0.)
    result = result + 360.;
  return static_cast<TYPE>(result);
}

/* Normalize lonx to -180 < lonx < 180 */
template<typename TYPE>
TYPE normalizeLonXDeg(TYPE lonX)
{
  double result = static_cast<double>(lonX);
  while(result > 180.)
    result = result - 360.;
  while(result < -180.)
    result = result + 360.;
  return static_cast<TYPE>(result);
}

/* Normalize laty to -90 < laty < 90 */
template<typename TYPE>
TYPE normalizeLatYDeg(TYPE latY)
{
  double result = static_cast<double>(latY);
  while(result > 90.)
    result = result - 180.;
  while(result < -90.)
    result = result + 180.;
  return static_cast<TYPE>(result);
}

} /* namespace geo */
} // namespace atools

#endif // ATOOLS_GEO_CALCULATIONS_H
