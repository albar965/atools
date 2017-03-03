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

#ifndef ATOOLS_GEO_POSITION_H
#define ATOOLS_GEO_POSITION_H

#include <QDebug>

class QRegularExpression;

namespace atools {
namespace geo {

enum CrossTrackStatus
{
  INVALID, /* No distance found */
  ALONG_TRACK, /* Point is along track */
  BEFORE_START, /* Point is before start - distance is point to start */
  AFTER_END /* Point is after end - distance is point to end */
};

/* Result for point to line distance measurments. All distances in meter. */
struct LineDistance
{
  CrossTrackStatus status;
  /* All distances in meter */
  float distance, /* Positive means right of course,  negative means left of course.
                   *  Can also contain distance to nearest point depending on status */
        distanceFrom1, /* Distance from point to start of line or linestring */
        distanceFrom2; /* Distance from point to end of line or linestring */
};

QDebug operator<<(QDebug out, const LineDistance& lineDist);

/*
 * Geographic position class. Calculations based on
 * http://williams.best.vwh.net/avform.htm
 */
class Pos
{
public:
  Pos();
  Pos(const Pos& other);

  explicit Pos(int lonXDeg, int lonXMin, float lonXSec, bool west,
               int latYDeg, int latYMin, float latYSec, bool south, float alt = 0.f);

  explicit Pos(float longitudeX, float latitudeY, float alt = 0.f);
  explicit Pos(double longitudeX, double latitudeY, double alt = 0.);

  /* @param str format like "N49° 26' 41.57",E9° 12' 5.49",+005500.00" */
  explicit Pos(const QString& str);

  atools::geo::Pos& operator=(const atools::geo::Pos& other);

  ~Pos()
  {

  }

  /* Does not compare altitude. Uses almostEqual for proper floating point comparison. */
  bool operator==(const atools::geo::Pos& other) const;

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

  /* Normalize this position to -180 < lonx < 180 and -90 < laty < 90 and return reference */
  atools::geo::Pos& normalize();

  /* Convert this position from rad to degree and return reference */
  atools::geo::Pos& toDeg();

  /* Convert this position from degree to rad and return reference */
  atools::geo::Pos& toRad();

  void swap(atools::geo::Pos& other);

  /* return endpoint at distance and angle */
  atools::geo::Pos endpoint(float distanceMeter, float angleDeg) const;

  /* Distance to other point in simple units */
  float distanceSimpleTo(const atools::geo::Pos& otherPos) const;

  /* Distance to other point for great circle route */
  float distanceMeterTo(const atools::geo::Pos& otherPos) const;

  /* Distance to line formed by pos1 and pos2. Positive means right of course,
   * negative means left of course. valid if perpendicular point can be found on route. */
  void distanceMeterToLine(const atools::geo::Pos& pos1, const atools::geo::Pos& pos2,
                           atools::geo::LineDistance& result) const;

  /* Angle to other point (initial course) */
  float angleDegTo(const atools::geo::Pos& otherPos) const;

  /* return endpoint at distance and angle for a rhumb line (constant course) */
  atools::geo::Pos endpointRhumb(float distanceMeter, float angleDeg) const;

  /* Distance to other point for rhumb line */
  float distanceMeterToRhumb(const atools::geo::Pos& otherPos) const;

  /* Angleto other point using a rhumb line */
  float angleDegToRhumb(const atools::geo::Pos& otherPos) const;

  /* @return Flight Simulator format like "N49° 26' 41.57",E9° 12' 5.49",+005500.00" */
  QString toLongString() const;

  /* @return format like "49.314,8.543,220" (lonX,latY,alt) */
  QString toString() const;

  /* @return Format like "9° 12' 5.49E", 49° 26' 41.57"N," */
  QString toHumanReadableString() const;

  /* false if position is not initialized */
  bool isValid() const
  {
    // Use half value to get around calculations
    return lonX < INVALID_VALUE / 2 && latY < INVALID_VALUE / 2;
  }

  /* false if position is null */
  bool isNull() const
  {
    return lonX == 0.f && latY == 0.f;
  }

  /* Return true if close to any pole */
  bool isPole() const
  {
    return isValid() && (latY > 89.f || latY < -89.f);
  }

  /* Find point between start and end on GC route if distance between points is already known.
   *  fraction is 0 <= fraction <= 1 where 0 equals this and 1 equal other pos */
  atools::geo::Pos interpolate(const atools::geo::Pos& otherPos, float distanceMeter, float fraction) const;

  /* Find point between start and end on GC route if distance between points is not known */
  atools::geo::Pos interpolate(const atools::geo::Pos& otherPos, float fraction) const;
  void interpolatePoints(const atools::geo::Pos& otherPos, float distanceMeter, int numPoints,
                         QList<atools::geo::Pos>& positions) const;

  /* Find point between start and end on rhumb line */
  atools::geo::Pos interpolateRhumb(const atools::geo::Pos& otherPos, float distanceMeter,
                                    float fraction) const;
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

  // 1 deg / minutes / nm to meter / to 10 cm
  Q_DECL_CONSTEXPR static float POS_EPSILON_MIN = std::numeric_limits<float>::epsilon();
  Q_DECL_CONSTEXPR static float POS_EPSILON_10CM = 1.f / 60.f / 1852.216f / 10.f; /* ca 10 cm for lat and lon nearby equator */
  Q_DECL_CONSTEXPR static float POS_EPSILON_1M = 1.f / 60.f / 1852.216f; /* ca 1 m for lat and lon nearby equator */
  Q_DECL_CONSTEXPR static float POS_EPSILON_5M = 1.f / 60.f / 1852.216f * 5.f; /* ca 5 m for lat and lon nearby equator */
  Q_DECL_CONSTEXPR static float POS_EPSILON_10M = 1.f / 60.f / 1852.216f * 10.f; /* ca 10 m for lat and lon nearby equator */
  Q_DECL_CONSTEXPR static float POS_EPSILON_100M = 1.f / 60.f / 1852.216f * 100.f; /* ca 100 m for lat and lon nearby equator */
  Q_DECL_CONSTEXPR static float POS_EPSILON_1000M = 1.f / 60.f / 1852.216f * 1000.f; /* ca 1 km for lat and lon nearby equator */

  Q_DECL_CONSTEXPR static float INVALID_VALUE = std::numeric_limits<float>::max();

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

  double distanceRad(double lonX1, double latY1, double lonX2, double latY2) const;
  double courseRad(double lonX1, double latY1, double lonX2, double latY2) const;

};

/* Invalid postion */
const atools::geo::Pos EMPTY_POS;

uint qHash(const atools::geo::Pos& pos);

} // namespace geo
} // namespace atools

Q_DECLARE_TYPEINFO(atools::geo::Pos, Q_PRIMITIVE_TYPE);
Q_DECLARE_METATYPE(atools::geo::Pos);

#endif // ATOOLS_GEO_POSITION_H
