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

#ifndef ATOOLS_GEO_CALCULATIONS_H
#define ATOOLS_GEO_CALCULATIONS_H

#include "atools.h"

#include <cmath>
#include <limits>

#include <QString>

namespace atools {
namespace geo {

class Line;
class LineString;
class Pos;
class Rect;

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

/* Calculate angles and bouding rectangle (if not nullptr) for an arc going through line.p1 and line.p2 with
 * given center. */
void arcFromPoints(const QLineF& line, const QPointF& center, bool left, QRectF *rect, float *startAngle,
                   float *spanAngle);

/* Numerical method to calculate the distance in meter along the arc that has center and goes through the two points
* of the line. left = counter clockwise. Fills distance if not null and lines with the approximation if not null. */
void calcArcLength(const atools::geo::Line& line, const atools::geo::Pos& center, bool left,
                   float *length, atools::geo::LineString *lines = nullptr);

/* Calculate a bounding rectangle for a list of positions. Also around the anti meridian which can
 * mean that left > right */
void boundingRect(atools::geo::Rect& rect, const QVector<Pos>& positions);

template<typename TYPE>
bool angleInRange(TYPE angle, TYPE min, TYPE max)
{
  if(max - min < 180.)
    // min 100 max 260
    return angle > min && angle < max;
  else
    // min 260 max 100
    return (angle > max && angle <= 360.) || (angle < min && angle >= 0.);
}

/* Square distance */
template<typename TYPE>
Q_DECL_CONSTEXPR int manhattanDistance(TYPE x1, TYPE y1, TYPE x2, TYPE y2)
{
  return std::abs(x1 - x2) + std::abs(y1 - y2);
}

template<>
Q_DECL_CONSTEXPR int manhattanDistance<int>(int x1, int y1, int x2, int y2)
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
Q_DECL_CONSTEXPR TYPE nmToMeter(TYPE value)
{
  return (value > std::numeric_limits<TYPE>::max() / 2) ? value :
         static_cast<TYPE>(static_cast<double>(value) * 1852.216);
}

/* Distance from nautical miles to kilometers */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE nmToKm(TYPE value)
{
  return (value > std::numeric_limits<TYPE>::max() / 2) ? value :
         static_cast<TYPE>(static_cast<double>(value) * 1852.216 / 1000.);
}

/* Distance from nautical miles to statue miles */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE nmToMi(TYPE value)
{
  return (value > std::numeric_limits<TYPE>::max() / 2) ? value :
         static_cast<TYPE>(static_cast<double>(value) * 1852.216 / 1609.3426);
}

/* Distance from meter to statue miles */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE meterToMi(TYPE value)
{
  return (value > std::numeric_limits<TYPE>::max() / 2) ? value :
         static_cast<TYPE>(static_cast<double>(value) / 1609.3426);
}

/* Distance from meters to nautical miles */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE meterToNm(TYPE value)
{
  return (value > std::numeric_limits<TYPE>::max() / 2) ? value :
         static_cast<TYPE>(static_cast<double>(value) / 1852.216);
}

template<typename TYPE>
Q_DECL_CONSTEXPR TYPE meterToFeet(TYPE value)
{
  return (value > std::numeric_limits<TYPE>::max() / 2) ? value :
         static_cast<TYPE>(3.2808399 * static_cast<double>(value));
}

template<typename TYPE>
Q_DECL_CONSTEXPR TYPE feetToMeter(TYPE value)
{
  return (value > std::numeric_limits<TYPE>::max() / 2) ? value :
         static_cast<TYPE>(0.3048 * static_cast<double>(value));
}

template<typename TYPE>
Q_DECL_CONSTEXPR TYPE feetToNm(TYPE value)
{
  return (value > std::numeric_limits<TYPE>::max() / 2) ? value : meterToNm(feetToMeter(value));
}

template<typename TYPE>
Q_DECL_CONSTEXPR TYPE nmToFeet(TYPE value)
{
  return (value > std::numeric_limits<TYPE>::max() / 2) ? value : meterToFeet(nmToMeter(value));
}

template<typename TYPE>
Q_DECL_CONSTEXPR TYPE kgToLbs(TYPE value)
{
  return (value > std::numeric_limits<TYPE>::max() / 2) ?
         value : static_cast<TYPE>(static_cast<double>(value) * 2.204623);
}

template<typename TYPE>
Q_DECL_CONSTEXPR TYPE lbsToKg(TYPE value)
{
  return (value > std::numeric_limits<TYPE>::max() / 2) ?
         value : static_cast<TYPE>(static_cast<double>(value) / 2.204623);
}

/* NM to rad (longitude or latitude) */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE nmToRad(TYPE value)
{
  return (value > std::numeric_limits<TYPE>::max() / 2) ? value :
         static_cast<TYPE>(M_PI / (180. * 60.) * static_cast<double>(value));
}

/* meter to rad (longitude or latitude) */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE meterToRad(TYPE value)
{
  return (value > std::numeric_limits<TYPE>::max() / 2) ? value : nmToRad(meterToNm(value));
}

/* Degree to rad */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE toRadians(TYPE deg)
{
  return (deg > std::numeric_limits<TYPE>::max() / 2) ? deg :
         static_cast<TYPE>(static_cast<double>(deg) * 0.017453292519943295769236907684886);
}

/* Rad to degree */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE toDegree(TYPE rad)
{
  return (rad > std::numeric_limits<TYPE>::max() / 2) ? rad :
         static_cast<TYPE>(static_cast<double>(rad) / 0.017453292519943295769236907684886);
}

/* Normalize course to 0 < course < 360 */
template<typename TYPE>
TYPE normalizeCourse(TYPE courseDegree)
{
  if(courseDegree > std::numeric_limits<TYPE>::max() / 2)
    return courseDegree;

  double result = static_cast<double>(courseDegree);
  while(result > 360.)
    result = result - 360.;
  while(result < 0.)
    result = result + 360.;
  return static_cast<TYPE>(result);
}

/* Get opposed course */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE opposedCourseDeg(TYPE courseDegree)
{
  return (courseDegree > std::numeric_limits<TYPE>::max() / 2) ? courseDegree :
         static_cast<TYPE>(atools::geo::normalizeCourse(static_cast<double>(courseDegree) + 180.));
}

template<typename TYPE>
TYPE normalizeRad(TYPE rad)
{
  if(rad > std::numeric_limits<TYPE>::max() / 2)
    return rad;

  double result = static_cast<double>(rad);
  while(result > M_PI * 2.)
    result = result - M_PI * 2.;
  while(result < 0.)
    result = result + M_PI * 2.;
  return static_cast<TYPE>(result);
}

/* Normalize lonx to -180 < lonx < 180 */
template<typename TYPE>
TYPE normalizeLonXDeg(TYPE lonX)
{
  if(lonX > std::numeric_limits<TYPE>::max() / 2)
    return lonX;

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
  if(latY > std::numeric_limits<TYPE>::max() / 2)
    return latY;

  double result = static_cast<double>(latY);
  while(result > 90.)
    result = result - 180.;
  while(result < -90.)
    result = result + 180.;
  return static_cast<TYPE>(result);
}

/* Convert angle in degrees (0 = north, counting CW) to Qt for QLineF::setAngle */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE angleToQt(TYPE angle)
{
  return (angle > std::numeric_limits<TYPE>::max() / 2) ? angle : -(angle - 90.);
}

/* Convert angle to degrees (0 = north, counting CW) from QLineF::angle */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE angleFromQt(TYPE angle)
{
  return (angle > std::numeric_limits<TYPE>::max() / 2) ? angle :
         atools::geo::normalizeCourse(360.f - angle + 90.f);
}

/* ISA temperature in °C at altitude (https://en.wikipedia.org/wiki/International_Standard_Atmosphere) */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE isaTemperature(TYPE altFeet)
{
  return static_cast<TYPE>(static_cast<double>(altFeet) < 36000. ?
                           (15. - (1.98 * static_cast<double>(altFeet) / 1000.)) :
                           -56.5);
}

/* Mach number to TAS in knots https://en.wikipedia.org/wiki/True_airspeed */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE machToTasFromAlt(TYPE altFeet, TYPE machNumber)
{
  return static_cast<TYPE>(39. * static_cast<double>(machNumber) *
                           std::sqrt(isaTemperature(static_cast<double>(altFeet)) + 273.15));
}

/* Mach number to TAS in knots https://en.wikipedia.org/wiki/True_airspeed */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE machToTasFromSat(TYPE sat, TYPE machNumber)
{
  return static_cast<TYPE>(39. * static_cast<double>(machNumber) * std::sqrt(sat) + 273.15);
}

/* TAS to mach number https://en.wikipedia.org/wiki/True_airspeed */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE tasToMach(TYPE sat, TYPE tas)
{
  return static_cast<TYPE>(static_cast<double>(tas) / (std::sqrt(static_cast<double>(sat) + 273.15) * 39.));
}

} /* namespace geo */
} // namespace atools

#endif // ATOOLS_GEO_CALCULATIONS_H
