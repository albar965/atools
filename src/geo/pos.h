/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_GEO_POSITION_H
#define ATOOLS_GEO_POSITION_H

#include <QDebug>
#include <QVariant>

namespace atools {
namespace geo {

class Point3D;
class LineString;

enum CrossTrackStatus
{
  INVALID, /* No distance found */
  ALONG_TRACK, /* Point is along track */
  BEFORE_START, /* Point is before start - distance is point to start */
  AFTER_END /* Point is after end - distance is point to end */
};

/* Result for point to line distance measurement. All distances in meter. */
struct LineDistance
{
  CrossTrackStatus status = INVALID;

  /* All distances in meter */
  float distance, /* Cross track distance - Positive means right of course,  negative means left of course.
                   * Can also contain distance to nearest point depending on status */
        distanceFrom1, /* Along track distance from point to start of line or linestring */
        distanceFrom2; /* Along track distance from point to end of line or linestring */
};

QDebug operator<<(QDebug out, const LineDistance& lineDist);

/*
 * Geographic position class. Calculations based on
 * http://williams.best.vwh.net/avform.htm
 */
class Pos
{
public:
  Pos() : lonX(INVALID_VALUE), latY(INVALID_VALUE), altitude(0.f)
  {
  }

  Pos(const Pos& other)
  {
    this->operator=(other);
  }

  explicit Pos(int lonXDeg, int lonXMin, float lonXSec, bool west,
               int latYDeg, int latYMin, float latYSec, bool south, float alt = 0.f);

  explicit Pos(float longitudeX, float latitudeY, float alt = 0.f)
    : lonX(longitudeX), latY(latitudeY), altitude(alt)
  {
  }

  explicit Pos(double longitudeX, double latitudeY, double alt = 0.)
  {
    lonX = static_cast<float>(longitudeX);
    latY = static_cast<float>(latitudeY);
    altitude = static_cast<float>(alt);
  }

  /* Will be invalid if variants are null or not valid */
  explicit Pos(const QVariant& longitudeX, const QVariant& latitudeY, const QVariant& alt = QVariant(0.f));

  /* @param str format like
   * N49° 26' 41.57",E9° 12' 5.49",+005500.00
   * N54* 16.82', W008* 35.95', +000011.00
   * 54.765892 , -130.647858,+5000 */
  explicit Pos(const QString& str, bool errorOnInvalid = true);

  atools::geo::Pos& operator=(const atools::geo::Pos& other)
  {
    lonX = other.lonX;
    latY = other.latY;
    altitude = other.altitude;
    return *this;
  }

  /* Does not compare altitude. Uses almostEqual for proper floating point comparison. */
  bool operator==(const atools::geo::Pos& other) const
  {
    return almostEqual(other, std::numeric_limits<float>::epsilon());
  }

  /* Does not compare altitude. Uses almostEqual for proper floating point comparison. */
  bool operator!=(const atools::geo::Pos& other) const
  {
    return !(*this == other);
  }

  /* Compare for equal with accuracy depending on epsilon. Does not compare altitude. */
  bool almostEqual(const atools::geo::Pos& other, float epsilon = POS_EPSILON_MIN) const;

  float getLatY() const
  {
    return latY;
  }

  float getLatYRad() const;

  int getLatYDeg() const
  {
    return deg(latY);
  }

  int getLatYMin() const
  {
    return min(latY);
  }

  float getLatYSec() const
  {
    return sec(latY);
  }

  float getLonX() const
  {
    return lonX;
  }

  float getLonXRad() const;

  int getLonXDeg() const
  {
    return deg(lonX);
  }

  int getLonXMin() const
  {
    return min(lonX);
  }

  float getLonXSec() const
  {
    return sec(lonX);
  }

  float getAltitude() const
  {
    return altitude;
  }

  atools::geo::Pos& swapLonXLatY()
  {
    std::swap(lonX, latY);
    return *this;
  }

  /* true if the position is close to one degree grid point or other factor defined by "spacing".
   *  Use 1 for spacing to detect one degree grid confluence points. */
  bool nearGridLonLat(float spacingLonX, float spacingLatY, float epsilon = POS_EPSILON_MIN) const;

  bool nearGrid(float spacing, float epsilon = POS_EPSILON_MIN) const
  {
    return nearGridLonLat(spacing, spacing, epsilon);
  }

  atools::geo::Pos& snapToGrid();

  /* Normalize this position to -180 < lonx < 180 and -90 < laty < 90 and return reference */
  atools::geo::Pos& normalize();

  /* Return a normalized copy of this */
  atools::geo::Pos normalized() const;

  /* Set altitude and return a copy */
  atools::geo::Pos alt(float alt) const;

  /* Convert this position from rad to degree and return reference */
  atools::geo::Pos& toDeg();

  /* Convert this position from degree to rad and return reference */
  atools::geo::Pos& toRad();

  void swap(atools::geo::Pos& other);

  /* return endpoint at distance and angle */
  atools::geo::Pos endpoint(float distanceMeter, float angleDeg) const;
  atools::geo::Pos endpointDouble(double distanceMeter, double angleDeg) const;

  /* Distance to other point in simple units. Uses manhattan distance in degrees.
   * Only for short distances and does not consider anti-meridian. */
  float distanceSimpleTo(const atools::geo::Pos& otherPos) const;

  /* Distance to other point for great circle route */
  float distanceMeterTo(const atools::geo::Pos& otherPos) const
  {
    return static_cast<float>(distanceMeterToDouble(otherPos));
  }

  double distanceMeterToDouble(const atools::geo::Pos& otherPos) const;

  /* Also uses distance in altitude which has to be given in feet.
   * Vertical distance is added to GC distance using Pythagoras and hence only accurate for short distances.
   * altitudeWeight can be used to lower or raise altitude priority. */
  float distanceMeterTo3d(const atools::geo::Pos& otherPos, float altitudeWeight = 1.f) const
  {
    return static_cast<float>(distanceMeterTo3dDouble(otherPos, altitudeWeight));
  }

  double distanceMeterTo3dDouble(const atools::geo::Pos& otherPos, double altitudeWeight = 1.f) const;

  /* Distance to line formed by pos1 and pos2. Positive means right of course,
   * negative means left of course. valid if perpendicular point can be found on route. */
  void distanceMeterToLine(const atools::geo::Pos& pos1, const atools::geo::Pos& pos2,
                           atools::geo::LineDistance& result) const;

  /* Angle to other point (initial course) */
  float angleDegTo(const atools::geo::Pos& otherPos) const;

  /* Initial GC angle/course to the other point */
  float initialBearing(const atools::geo::Pos& otherPos) const;

  /* Final GC angle/course to the other point */
  float finalBearing(const atools::geo::Pos& otherPos) const;

  /* return endpoint at distance and angle for a rhumb line (constant course) */
  atools::geo::Pos endpointRhumb(float distanceMeter, float angleDeg) const;

  /* Distance to other point for rhumb line */
  float distanceMeterToRhumb(const atools::geo::Pos& otherPos) const;

  /* Angleto other point using a rhumb line */
  float angleDegToRhumb(const atools::geo::Pos& otherPos) const;

  /* @return Flight Simulator format like "N49° 26' 41.57",E9° 12' 5.49",+005500.00" or
   * "N49* 26' 41.57",E9* 12' 5.49",+005500.00" */
  QString toLongString(bool starDeg) const;

  /* @return format like "49.314,8.543,220" (lonX,latY,alt) */
  QString toString(int precision = 6, bool alt = true) const;

  /* @return Format like "9° 12' 5.49E", 49° 26' 41.57"N," */
  QString toHumanReadableString() const;

  /* false if position is not initialized */
  bool isValid() const
  {
    // Use half value to get around calculations
    return lonX < INVALID_VALUE / 2.f && latY < INVALID_VALUE / 2.f;
  }

  /* false if ordinates exceed range -180 < +180 and -90 < +90 */
  bool isValidRange() const
  {
    return lonX >= -180.f && lonX <= 180.f && latY >= -90.f && latY <= 90.f;
  }

  /* true if position is null */
  bool isNull() const;

  bool isNull(float epsilonDegree) const;

  /* Return true if close to any pole */
  bool isPole(float epsilonDegree = 1.f) const
  {
    return isValid() && (latY > (90.f - epsilonDegree) || (latY < -90.f + epsilonDegree));
  }

  /* Find point between start and end on GC route if distance between points is already known.
   *  fraction is 0 <= fraction <= 1 where 0 equals this and 1 equal other pos */
  atools::geo::Pos interpolate(const atools::geo::Pos& otherPos, float distanceMeter, float fraction) const;

  /* Find point between start and end on GC route if distance between points is not known */
  atools::geo::Pos interpolate(const atools::geo::Pos& otherPos, float fraction) const;

  /* positions includes this if numPoints > 0 but not otherPos  */
  void interpolatePoints(const atools::geo::Pos& otherPos, float distanceMeter, int numPoints, atools::geo::LineString& positions) const;
  void interpolatePointsRhumb(const atools::geo::Pos& otherPos, float distanceMeter, int numPoints,
                              atools::geo::LineString& positions) const;

  /* interpolate altitude value in positins too */
  void interpolatePointsAlt(const atools::geo::Pos& otherPos, float distanceMeter, int numPoints,
                            atools::geo::LineString& positions) const;

  /* Find point between start and end on rhumb line */
  atools::geo::Pos interpolateRhumb(const atools::geo::Pos& otherPos, float distanceMeter, float fraction) const;
  atools::geo::Pos interpolateRhumb(const atools::geo::Pos& otherPos, float fraction) const;

  /* Returns the point of intersection of two paths defined by point and bearing */
  static atools::geo::Pos intersectingRadials(const atools::geo::Pos& p1, float brng1,
                                              const atools::geo::Pos& p2, float brng2);

  void setLonX(float value)
  {
    lonX = value;
  }

  void setLatY(float value)
  {
    latY = value;
  }

  void setAltitude(float value)
  {
    altitude = value;
  }

  /*
   * Methods to convert the coordinate points into a 3D cartesian system.
   * Earth center is zero-origin in cartesian and units are meter.
   */
  /*  *INDENT-OFF* */
  // Coordinate axes:
  //   | Z
  //   |
  //   |___ Y
  //  /
  // / X
  /* *INDENT-ON* */
  atools::geo::Point3D toCartesian() const;
  void toCartesian(atools::geo::Point3D& point) const;
  void toCartesian(double& x, double& y, double& z) const;
  void toCartesian(float& x, float& y, float& z) const;

  // 1 deg / minutes / nm to meter / to 10 cm
  Q_DECL_CONSTEXPR static float POS_EPSILON_MIN = std::numeric_limits<float>::epsilon();
  Q_DECL_CONSTEXPR static float POS_EPSILON_10CM = 1.f / 60.f / 1852.216f / 10.f; /* ca 10 cm for lat and lon nearby equator */
  Q_DECL_CONSTEXPR static float POS_EPSILON_1M = 1.f / 60.f / 1852.216f; /* ca 1 m for lat and lon nearby equator */
  Q_DECL_CONSTEXPR static float POS_EPSILON_5M = 1.f / 60.f / 1852.216f * 5.f; /* ca 5 m for lat and lon nearby equator */
  Q_DECL_CONSTEXPR static float POS_EPSILON_10M = 1.f / 60.f / 1852.216f * 10.f; /* ca 10 m for lat and lon nearby equator */
  Q_DECL_CONSTEXPR static float POS_EPSILON_100M = 1.f / 60.f / 1852.216f * 100.f; /* ca 100 m for lat and lon nearby equator */
  Q_DECL_CONSTEXPR static float POS_EPSILON_500M = 1.f / 60.f / 1852.216f * 500.f; /* ca 500 m for lat and lon nearby equator */
  Q_DECL_CONSTEXPR static float POS_EPSILON_1000M = 1.f / 60.f / 1852.216f * 1000.f; /* ca 1 km for lat and lon nearby equator */

  Q_DECL_CONSTEXPR static float POS_EPSILON_1NM = 1.f / 60.f; /* 1 minute = 1 NM lat and lon nearby equator */
  Q_DECL_CONSTEXPR static float POS_EPSILON_10NM = 1.f / 60.f * 10.f; /* 10 NM  */
  Q_DECL_CONSTEXPR static float POS_EPSILON_100NM = 1.f / 60.f * 100.f; /* 100 NM */

  Q_DECL_CONSTEXPR static float INVALID_VALUE = std::numeric_limits<float>::max();

  /* Basic funtions if more accuracy than float is needed */
  static double distanceRad(double lonX1, double latY1, double lonX2, double latY2);
  static double courseRad(double lonX1, double latY1, double lonX2, double latY2);

  /* Quick and rough estimate using table based approach meter per degree longitude depending on latitude.
   * Rounds down to have slightly larger boundaries in degree than desired. */
  static float meterForDegreeLonx(float latY);

  float meterForDegreeLonx() const
  {
    return meterForDegreeLonx(latY);
  }

  Q_DECL_CONSTEXPR static double EARTH_RADIUS_METER_DOUBLE = 6371. * 1000.;

private:
  // Länge (x),Breite (y)
  float lonX, latY, altitude;

  friend QDataStream& operator<<(QDataStream& out, const atools::geo::Pos& obj);

  friend QDataStream& operator>>(QDataStream& in, atools::geo::Pos& obj);

  friend QDebug operator<<(QDebug out, const atools::geo::Pos& record);

  float sec(float value) const;
  int min(float value) const;
  int deg(float value) const;
  inline bool doesOverflow60(float value) const;

};

/* Invalid position */
const atools::geo::Pos EMPTY_POS;

inline uint qHash(const atools::geo::Pos& pos)
{
  return static_cast<unsigned int>(pos.getLonX()) ^ static_cast<unsigned int>(pos.getLatY());
}

/* Primitive class to transport more accurate values */
class PosD
{
public:
  PosD()
    : lonX(INVALID_VALUE), latY(INVALID_VALUE), altitude(0.)
  {

  }

  explicit PosD(const atools::geo::Pos& pos)
  {
    if(pos.isValid())
    {
      lonX = pos.getLonX();
      latY = pos.getLatY();
      altitude = pos.getAltitude();
    }
    else
    {
      lonX = latY = INVALID_VALUE;
      altitude = 0.;
    }
  }

  explicit PosD(double longitudeX, double latitudeY, double alt = 0.)
    : lonX(longitudeX), latY(latitudeY), altitude(alt)
  {

  }

  PosD(const atools::geo::PosD& other)
  {
    this->operator=(other);
  }

  atools::geo::PosD& operator=(const atools::geo::PosD& other)
  {
    if(other.isValid())
    {
      lonX = other.lonX;
      latY = other.latY;
      altitude = other.altitude;
    }
    else
    {
      lonX = latY = INVALID_VALUE;
      altitude = 0.;
    }
    return *this;
  }

  /* Does not compare altitude. Uses almostEqual for proper floating point comparison. */
  bool operator==(const atools::geo::PosD& other) const
  {
    return almostEqual(other, std::numeric_limits<double>::epsilon());
  }

  /* Does not compare altitude. Uses almostEqual for proper floating point comparison. */
  bool operator!=(const atools::geo::PosD& other) const
  {
    return !(*this == other);
  }

  /* Distance to other point for great circle route */
  double distanceMeterTo(const PosD& otherPos) const;

  /* Angle to other point (initial course) */
  double angleDegTo(const atools::geo::PosD& otherPos) const;

  atools::geo::PosD endpoint(double distanceMeter, double angleDeg) const;

  /* Compare for equal with accuracy depending on epsilon. Does not compare altitude. */
  bool almostEqual(const atools::geo::PosD& other, double epsilon = DPOS_EPSILON_MIN) const;

  atools::geo::Pos asPos() const
  {
    return isValid() ? atools::geo::Pos(lonX, latY, altitude) : atools::geo::Pos();
  }

  double getLonX() const
  {
    return lonX;
  }

  double getLatY() const
  {
    return latY;
  }

  /* false if position is not initialized */
  bool isValid() const
  {
    // Use half value to get around calculations
    return lonX < INVALID_VALUE / 2. && latY < INVALID_VALUE / 2.;
  }

  /* false if ordinates exceed range -180 < +180 and -90 < +90 */
  bool isValidRange() const
  {
    return lonX >= -180. && lonX <= 180. && latY >= -90. && latY <= 90.;
  }

  /* true if position is null */
  bool isNull() const;

  double getAltitude() const
  {
    return altitude;
  }

  void setLonX(double value)
  {
    lonX = value;
  }

  void setLatY(double value)
  {
    latY = value;
  }

  void setAltitude(double value)
  {
    altitude = value;
  }

  PosD& normalize();

private:
  friend QDebug operator<<(QDebug out, const atools::geo::PosD& record);
  friend QDataStream& operator<<(QDataStream& out, const atools::geo::PosD& obj);
  friend QDataStream& operator>>(QDataStream& in, atools::geo::PosD& obj);

  Q_DECL_CONSTEXPR static double INVALID_VALUE = std::numeric_limits<double>::max();
  Q_DECL_CONSTEXPR static float DPOS_EPSILON_MIN = std::numeric_limits<float>::epsilon();

  double lonX, latY, altitude;
};

/* Invalid position */
const atools::geo::PosD EMPTY_POSD;

} // namespace geo
} // namespace atools

Q_DECLARE_TYPEINFO(atools::geo::Pos, Q_PRIMITIVE_TYPE);
Q_DECLARE_METATYPE(atools::geo::Pos)

Q_DECLARE_TYPEINFO(atools::geo::PosD, Q_PRIMITIVE_TYPE);
Q_DECLARE_METATYPE(atools::geo::PosD)

Q_DECLARE_TYPEINFO(atools::geo::LineDistance, Q_PRIMITIVE_TYPE);

#endif // ATOOLS_GEO_POSITION_H
