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

#ifndef ATOOLS_GEO_CALCULATIONS_H
#define ATOOLS_GEO_CALCULATIONS_H

#include "atools.h"

#include <cmath>
#include <limits>

#include <QLineF>
#include <QString>

namespace atools {
namespace geo {

const float EARTH_RADIUS_METER = 6371.f * 1000.f;
const float EARTH_CIRCUMFERENCE_METER = EARTH_RADIUS_METER * 2.f * 3.14159265358979323846f;

/* Use maximum numbers to indicate invalid values */
const float INVALID_FLOAT = std::numeric_limits<float>::max();
const double INVALID_DOUBLE = std::numeric_limits<double>::max();
const int INVALID_INT = std::numeric_limits<int>::max();

/* Forward declarations */
class Line;
class LineString;
class Pos;
class Rect;

void registerMetaTypes();

template<typename TYPE>
inline TYPE atan2Deg(TYPE y, TYPE x);

template<typename TYPE>
TYPE normalizeCourse(TYPE courseDegree);

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
void boundingRect(atools::geo::Rect& rect, QVector<Pos> positions);
atools::geo::Rect boundingRect(const QVector<Pos>& positions);

/* true if longitude values cross the anti-meridian independent of direction but unreliable for large rectangles. */
bool crossesAntiMeridian(float lonx1, float lonx2);

bool isWestCourse(float lonx1, float lonx2);
bool isEastCourse(float lonx1, float lonx2);

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

/* Returns time with milliseconds set to 0
 *  zenith:      Sun's zenith for sunrise/sunset
 *  offical      = 90 degrees 50'
 *  civil        = 96 degrees (relevant for aviation)
 *  nautical     = 102 degrees
 *  astronomical = 108 degrees
 *
 * Negative zenith for sunset and positive for sunrise
 *
 * Returns null time if sun never rises or sets at this position and date
 *
 * http://edwilliams.org/sunrise_sunset_algorithm.htm
 */
const float SUNRISE_OFFICIAL = 90.f + (50.f / 60.f);
const float SUNSET_OFFICIAL = 90.f + (50.f / 60.f);
const float SUNRISE_CIVIL = 96.f;
const float SUNSET_CIVIL = -96.f;
const float SUNRISE_NAUTICAL = 102.f;
const float SUNSET_NAUTICAL = -102.f;
const float SUNRISE_ASTRONOMICAL = 108.f;
const float SUNSET_ASTRONOMICAL = -108.f;

QTime calculateSunriseSunset(bool& neverRises, bool& neverSets, const atools::geo::Pos& position, const QDate& date,
                             float zenith);

/* Get desired heading to fly course in wind conditions.
 * returns INVALID_FLOAT is impossible due to tailwind > TAS
 */
float windCorrectedHeading(float& groundSpeed, float windSpeed, float windDirectionDeg, float courseDeg,
                           float trueAirspeed);
float windCorrectedHeading(float windSpeed, float windDirectionDeg, float courseDeg, float trueAirspeed);
float windCorrectedGroundSpeed(float windSpeed, float windDirectionDeg, float courseDeg, float trueAirspeed);

/* Calculate head and cross wind for a given course and wind direction.
 *  If head wind is < 0 it is a tail wind.
 *  If cross wind is < 0 wind is from left */
void windForCourse(float& headWind, float& crossWind, float windSpeed, float windDirectionDeg, float courseDeg);
float headWindForCourse(float windSpeed, float windDirectionDeg, float courseDeg);

/* Calculate wind speed from u and v components
 * V component of wind; northward_wind;
 * U component of wind; eastward_wind;
 *  A positive Ugeo component represents wind blowing to the East (confusingly known as a "westerly").
 * +Vgeo is wind to the North (a "southerly" ). This is right handed with respect to an upward +Wgeo.*/
inline float windSpeedFromUV(float u, float v)
{
  return std::sqrt(u * u + v * v);
}

inline double windSpeedFromUV(double u, double v)
{
  return std::sqrt(u * u + v * v);
}

/* Calculate wind true heading from u and v components
 * V component of wind; northward_wind;
 * U component of wind; eastward_wind; */
inline float windDirectionFromUV(float u, float v)
{
  if(atools::almostEqual(u, 0.f) && atools::almostEqual(v, 0.f))
    return 0.f; // Otherwise 180
  else
    return atools::geo::normalizeCourse(atan2Deg(-u, -v));
}

inline double windDirectionFromUV(double u, double v)
{
  if(atools::almostEqual(u, 0.) && atools::almostEqual(v, 0.))
    return 0.; // Otherwise 180
  else
    return atools::geo::normalizeCourse(atan2Deg(-u, -v));
}

/* Calculate wind eastward component from speed and direction in degrees
 * U component of wind; eastward_wind; */
inline float windUComponent(float speed, float dir)
{
  return static_cast<float>(-speed * sin(atools::geo::toRadians(dir)));
}

inline double windUComponent(double speed, double dir)
{
  return -speed *sin(atools::geo::toRadians(dir));
}

/* Calculate wind eastward component from speed and direction in degrees
 * V component of wind; northward_wind; */
inline float windVComponent(float speed, float dir)
{
  return static_cast<float>(-speed * cos(atools::geo::toRadians(dir)));
}

inline double windVComponent(double speed, double dir)
{
  return -speed *cos(atools::geo::toRadians(dir));
}

/* Check for invalid coordinates if they are not exceeding bounds and are not NaN or INF if floating point */
inline bool ordinateValid(int ord)
{
  return ord > std::numeric_limits<int>::lowest() / 2 && ord < INVALID_INT / 2;
}

inline bool ordinateValidF(float ord)
{
  return std::isfinite(ord) &&
         ord > std::numeric_limits<float>::lowest() / 2.f && ord < INVALID_FLOAT / 2.f;
}

inline bool ordinateValidD(double ord)
{
  return std::isfinite(ord) &&
         ord > std::numeric_limits<double>::lowest() / 2. && ord < INVALID_DOUBLE / 2.;
}

inline bool pointValid(const QPointF& point)
{
  return ordinateValidD(point.x()) && ordinateValidD(point.y());
}

inline bool pointValid(const QPoint& point)
{
  return ordinateValid(point.x()) && ordinateValid(point.y());
}

inline bool lineValid(const QLineF& line)
{
  return pointValid(line.p1()) && pointValid(line.p2());
}

inline bool lineValid(const QLine& line)
{
  return pointValid(line.p1()) && pointValid(line.p2());
}

/* Converts rectangles to square rectangles so that width == height */
QRect rectToSquare(const QRect& rect);
QRectF rectToSquare(const QRectF& rect);

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
  return static_cast<int>(std::round(std::sqrt(static_cast<double>((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2)))));
}

/* Euclidian distance between points */
template<typename TYPE>
Q_DECL_CONSTEXPR float simpleDistanceF(TYPE x1, TYPE y1, TYPE x2, TYPE y2)
{
  return static_cast<float>(std::sqrt(static_cast<double>((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2))));
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
         static_cast<TYPE>(static_cast<double>(value) * 1.852216);
}

/* Distance from nautical miles to statue miles */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE nmToMi(TYPE value)
{
  return (value > std::numeric_limits<TYPE>::max() / 2) ? value :
         static_cast<TYPE>(static_cast<double>(value) * 1852.216 / 1609.3426);
}

/* Distance from statue miles to nautical miles */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE miToNm(TYPE value)
{
  return (value > std::numeric_limits<TYPE>::max() / 2) ? value :
         static_cast<TYPE>(static_cast<double>(value) * 1609.3426 / 1852.216);
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

/* Distance from km to nautical miles */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE kmToNm(TYPE value)
{
  return (value > std::numeric_limits<TYPE>::max() / 2) ? value :
         static_cast<TYPE>(static_cast<double>(value) / 1.852216);
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
Q_DECL_CONSTEXPR TYPE meterPerSecToKnots(TYPE value)
{
  return static_cast<TYPE>((value > std::numeric_limits<TYPE>::max() / 2) ? value :
                           static_cast<double>(value) * 1.943844);
}

template<typename TYPE>
Q_DECL_CONSTEXPR TYPE knotsToMeterPerSec(TYPE value)
{
  return static_cast<TYPE>((value > std::numeric_limits<TYPE>::max() / 2) ? value :
                           static_cast<double>(value) / 1.943844);
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

/* Litre to US Gallon */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE literToGallon(TYPE value)
{
  return (value > std::numeric_limits<TYPE>::max() / 2) ?
         value : static_cast<TYPE>(static_cast<double>(value) / 3.785411784);
}

/* US Gallon to Litre */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE gallonToLiter(TYPE value)
{
  return (value > std::numeric_limits<TYPE>::max() / 2) ?
         value : static_cast<TYPE>(static_cast<double>(value) * 3.785411784);
}

/* Calculate the weight/volume ratio and determine if it is jet fuel
 *  weightVolRatio is 0 if quantity/weight is not sufficient */
bool isJetFuel(float fuelWeightLbs, float fuelQuantityGal, float& weightVolRatio);

/* Avgas 1 gal = 6 lbs, Jetfuel 1 gal = 6,7 lbs */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE  fromGalToLbs(bool jetFuel, TYPE value)
{
  return (value > std::numeric_limits<TYPE>::max() / 2) ?
         value : static_cast<TYPE>(static_cast<double>(value) * (jetFuel ? 6.7 : 6.));
}

template<typename TYPE>
Q_DECL_CONSTEXPR TYPE  fromLbsToGal(bool jetFuel, TYPE value)
{
  return (value > std::numeric_limits<TYPE>::max() / 2) ?
         value : static_cast<TYPE>(static_cast<double>(value) / (jetFuel ? 6.7 : 6.));
}

template<typename TYPE>
Q_DECL_CONSTEXPR TYPE  fromLiterToKg(bool jetFuel, TYPE value)
{
  return (value > std::numeric_limits<TYPE>::max() / 2) ?
         value : static_cast<TYPE>(lbsToKg(fromGalToLbs(jetFuel, literToGallon(static_cast<double>(value)))));
}

template<typename TYPE>
Q_DECL_CONSTEXPR TYPE  fromKgToLiter(bool jetFuel, TYPE value)
{
  return (value > std::numeric_limits<TYPE>::max() / 2) ?
         value : static_cast<TYPE>(gallonToLiter(fromLbsToGal(jetFuel, kgToLbs(static_cast<double>(value)))));
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

/* Normalizes a number to an arbitrary range by assuming the range wraps around when going below min or above max */
template<typename TYPE>
TYPE normalizeRange(TYPE value, TYPE start, TYPE end)
{
  TYPE width = end - start;
  TYPE offsetValue = value - start; // Make value relative to 0

  // Reset back to start of original range by adding start
  return (offsetValue - (floor(offsetValue / width) * width)) + start;
}

/* Normalizes a number to an arbitrary range by assuming the range wraps around when going below min or above max */
template<>
inline int normalizeRange<int>(int value, int start, int end)
{
  int width = end - start;
  int offsetValue = value - start; // Make value relative to 0

  // Reset back to start of original range by adding start
  return (offsetValue - ((offsetValue / width) * width)) + start;
}

/* Normalize course to 0 < course < 360 */
template<typename TYPE>
TYPE normalizeCourse(TYPE courseDegree)
{
  return static_cast<TYPE>(normalizeRange(static_cast<double>(courseDegree), 0., 360.));
}

/* Get opposed course */
template<typename TYPE>
Q_DECL_CONSTEXPR TYPE opposedCourseDeg(TYPE courseDegree)
{
  return (courseDegree > std::numeric_limits<TYPE>::max() / 2) ? courseDegree :
         static_cast<TYPE>(atools::geo::normalizeCourse(static_cast<double>(courseDegree) + 180.));
}

/* Calculates difference between courses (0-360 Deg).
 * Result is always positive and never > 180 Deg */
template<typename TYPE>
TYPE angleAbsDiff(TYPE angle1, TYPE angle2)
{
  return angle2 > angle1 ?
         // 100 to 260 : 10 to 350
         (angle2 - angle1 <= 180. ? angle2 - angle1 : angle1 + 360. - angle2) :
         // 260 to 100 : 350 to 10
         (angle1 - angle2 <= 180. ? angle1 - angle2 : angle2 + 360. - angle1);
}

/* Calculates difference between courses (0-360 Deg).
 * Result is positive if angle1 < angle2 and negative if angle1 > angle2 and never > 180 Deg.
 * Clockwise is positive and counter-clockwise is negative */
template<typename TYPE>
TYPE angleAbsDiffSign(TYPE angle1, TYPE angle2)
{
  return angle2 > angle1 ?
         // 100 to 260 : 10 to 350
         (angle2 - angle1 <= 180. ? angle2 - angle1 : -(angle1 + 360. - angle2)) :
         // 260 to 100 : 350 to 10
         (angle1 - angle2 <= 180. ? -(angle1 - angle2) : angle2 + 360. - angle1);
}

/* Calculates difference between courses (0-360 Deg) which are given clockwise from angle1 to angle2.
 * Result can be 0 <= res <= 360 */
template<typename TYPE>
TYPE angleAbsDiff2(TYPE angle1, TYPE angle2)
{
  if(atools::almostEqual(angle1, angle2))
    return 0.f;
  else if(angle2 > angle1)
    return angle2 - angle1;
  else
    return (angle2 + 360.) - angle1;
}

/* Normalize lonx to -180 < lonx < 180 */
template<typename TYPE>
TYPE normalizeLonXDeg(TYPE lonX)
{
  return static_cast<TYPE>(normalizeRange(static_cast<double>(lonX), -180., 180.));
}

/* Normalize laty to -90 < laty < 90 */
template<typename TYPE>
TYPE normalizeLatYDeg(TYPE latY)
{
  return static_cast<TYPE>(normalizeRange(static_cast<double>(latY), -90., 90.));
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

template<typename TYPE>
Q_DECL_CONSTEXPR TYPE tasToMachFromAlt(TYPE altFeet, TYPE tas)
{
  return static_cast<TYPE>(static_cast<double>(tas) /
                           (std::sqrt(isaTemperature(static_cast<double>(altFeet)) + 273.15) * 39.));
}

template<typename TYPE>
Q_DECL_CONSTEXPR TYPE pressureMbarForAltMeter(TYPE altMeter)
{
  return static_cast<TYPE>(1013.25 * std::pow(1.0 - ((0.0065 * static_cast<double>(altMeter)) / 288.15), 5.255));
}

template<typename TYPE>
Q_DECL_CONSTEXPR TYPE altMeterForPressureMbar(TYPE pressureMbar)
{
  return static_cast<TYPE>(288.15 / 0.0065 *
                           (1 - std::pow((static_cast<double>(pressureMbar) / 1013.25), (1. / 5.255))));
}

/* Collection of trigonometric functions that accept or return degree */
template<typename TYPE>
inline TYPE sinDeg(TYPE value)
{
  return static_cast<TYPE>(sin(atools::geo::toRadians(static_cast<double>(value))));
}

template<typename TYPE>
inline TYPE asinDeg(TYPE value)
{
  return static_cast<TYPE>(atools::geo::toDegree(asin(static_cast<double>(value))));
}

template<typename TYPE>
inline TYPE cosDeg(TYPE value)
{
  return static_cast<TYPE>(cos(atools::geo::toRadians(static_cast<double>(value))));
}

template<typename TYPE>
inline TYPE acosDeg(TYPE value)
{
  return static_cast<TYPE>(atools::geo::toDegree(acos(static_cast<double>(value))));
}

template<typename TYPE>
inline TYPE tanDeg(TYPE value)
{
  return static_cast<TYPE>(tan(atools::geo::toRadians(static_cast<double>(value))));
}

template<typename TYPE>
inline TYPE atanDeg(TYPE value)
{
  return static_cast<TYPE>(atools::geo::toDegree(atan(static_cast<double>(value))));
}

template<typename TYPE>
inline TYPE atan2Deg(TYPE y, TYPE x)
{
  return static_cast<TYPE>(atools::geo::toDegree(atan2(static_cast<double>(y), static_cast<double>(x))));
}

/* Get descent speed in feet/minute for groundspeed and vertical angle. Returns positive values. */
template<typename TYPE>
inline TYPE descentSpeedForPathAngle(TYPE groundspeedKts, TYPE vertAngleDeg)
{
  return atools::geo::tanDeg(std::abs(vertAngleDeg)) * (atools::geo::nmToFeet(groundspeedKts) / 60.f);
}

} /* namespace geo */
} // namespace atools

#endif // ATOOLS_GEO_CALCULATIONS_H
