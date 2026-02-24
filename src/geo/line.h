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

#ifndef ATOOLS_GEO_LINE_H
#define ATOOLS_GEO_LINE_H

#include "geo/pos.h"
#include "geo/calculations.h"

#include <QDebug>

namespace atools {
namespace geo {

class Rect;
class LineString;

/*
 * Geographic line class. Calculations based on
 * http://williams.best.vwh.net/avform.htm
 */
class Line
{
public:
  Line()
  {

  }

  Line(const atools::geo::Line& other)
  {
    this->operator=(other);

  }

  Line(const atools::geo::Pos& p1, const atools::geo::Pos& p2)
  {
    pos1 = p1;
    pos2 = p2;
  }

  Line(const atools::geo::Pos& pos, float distanceMeter, float course)
  {
    pos1 = pos;
    pos2 = pos1.endpoint(distanceMeter, course);
  }

  /* Create a line with length 0 */
  explicit Line(const atools::geo::Pos& p)
  {
    pos1 = pos2 = p;
  }

  explicit Line(float longitudeX1, float latitudeY1, float longitudeX2, float latitudeY2)
    : pos1(longitudeX1, latitudeY1), pos2(longitudeX2, latitudeY2)
  {

  }

  explicit Line(double longitudeX1, double latitudeY1, double longitudeX2, double latitudeY2)
    : pos1(longitudeX1, latitudeY1), pos2(longitudeX2, latitudeY2)
  {

  }

  atools::geo::Line& operator=(const atools::geo::Line& other)
  {
    pos1 = other.pos1;
    pos2 = other.pos2;
    return *this;
  }

  /* Does not compare altitude */
  bool operator==(const atools::geo::Line& other) const
  {
    return pos1 == other.pos1 && pos2 == other.pos2;
  }

  /* Does not compare altitude */
  bool operator!=(const atools::geo::Line& other) const
  {
    return !(*this == other);
  }

  /* Distance to other point in simple units. Uses manhattan distance in degrees. */
  float lengthSimple() const
  {
    return pos1.distanceSimpleTo(pos2);
  }

  /* Distance to other point for great circle route */
  float lengthMeter() const
  {
    return pos1.distanceMeterTo(pos2);
  }

  double lengthMeterDouble() const
  {
    return pos1.distanceMeterToDouble(pos2);
  }

  /* Calculate status, cross track distance and more to this line. */
  void distanceMeterToLine(const atools::geo::Pos& pos, atools::geo::LineDistance& result) const
  {
    pos.distanceMeterToLine(pos1, pos2, result);
  }

  /* Angle of line (initial course) */
  float angleDeg() const
  {
    return pos1.angleDegTo(pos2);
  }

  /* Distance to other point for rhumb line */
  float distanceMeterRhumb() const
  {
    return pos1.distanceMeterToRhumb(pos2);
  }

  /* Angleto other point using a rhumb line */
  float angleDegRhumb() const
  {
    return pos1.angleDegToRhumb(pos2);
  }

  /* Find point between start and end on GC route if distance between points is already known.
   *  fraction is 0 <= fraction <= 1 where 0 equals this and 1 equal other pos */
  atools::geo::Pos interpolate(float distanceMeter, float fraction) const
  {
    return pos1.interpolate(pos2, distanceMeter, fraction);
  }

  /* Find point between start and end on GC route if distance between points is not known. 0 < fraction <= 1.*/
  atools::geo::Pos interpolate(float fraction) const
  {
    return pos1.interpolate(pos2, fraction);
  }

  /* Returns a list of points which includes pos1 and not pos2. */
  void interpolatePoints(float distanceMeter, int numPoints, atools::geo::LineString& positions) const
  {
    pos1.interpolatePoints(pos2, distanceMeter, numPoints, positions);
  }

  void interpolatePointsRhumb(float distanceMeter, int numPoints, atools::geo::LineString& positions) const
  {
    pos1.interpolatePointsRhumb(pos2, distanceMeter, numPoints, positions);
  }

  /* Find the intersection of the GC line with the circle - numerical method.
   * Will find the nearest intersection point. */
  atools::geo::Pos intersectionWithCircle(const atools::geo::Pos& center, float radiusMeter,
                                          float accuracyMeter) const;

  /* Create a parallel line to this one.
   * Positive distance: Parallel to the right (looking from pos1 to pos2)
   * Negative distance: Parallel to the left (looking from pos1 to pos2) */
  atools::geo::Line parallel(float distanceMeter);

  /* Make line longer or shorted
   * Positive distance: Move pos outward - line will be longer
   * Negative distance: Move pos inward - line will be shorted */
  atools::geo::Line extended(float distanceMeter1, float distanceMeter2);

  /* Calculate and return bounding rectangle for this line */
  atools::geo::Rect boundingRect() const;

  /* false if position is not initialized */
  bool isValid() const
  {
    return pos1.isValid() && pos2.isValid();
  }

  /* false if position is null */
  bool isNull() const
  {
    return pos1.isNull() && pos2.isNull();
  }

  const atools::geo::Pos& getPos1() const
  {
    return pos1;
  }

  void setPos1(const atools::geo::Pos& value)
  {
    pos1 = value;
  }

  /* Creates a line with same start and end position */
  void setPosAll(const atools::geo::Pos& value)
  {
    pos1 = pos2 = value;
  }

  /*
   * @return true if line is a single point
   */
  bool isPoint(float epsilonDegree = std::numeric_limits<float>::epsilon()) const
  {
    return isValid() && pos1.almostEqual(pos2, epsilonDegree);
  }

  const atools::geo::Pos& getPos2() const
  {
    return pos2;
  }

  void setPos2(const atools::geo::Pos& value)
  {
    pos2 = value;
  }

  /* true if longitude values cross the anti-meridian independent of direction but unreliable for large rectangles. */
  bool crossesAntiMeridian() const
  {
    return atools::geo::crossesAntiMeridian(pos1, pos2);
  }

  /* Returns two lines if it crosses. Otherwise a copy of this or empty list if invalid. */
  const QList<Line> splitAtAntiMeridian(bool *crossed = nullptr) const
  {
    return atools::geo::splitAtAntiMeridian(pos1, pos2, crossed);
  }

  /* true if heading of "from" to "to" is towards west or east. */
  bool isWestCourse() const
  {
    return atools::geo::isWestCourse(pos1.getLonX(), pos2.getLonX());
  }

  bool isEastCourse() const
  {
    return atools::geo::isEastCourse(pos1.getLonX(), pos2.getLonX());
  }

  float getInitialBearing()
  {
    return pos1.initialBearing(pos2);
  }

  float getFinalBearing()
  {
    return pos1.finalBearing(pos2);
  }

  /* Normalize all positions to -180 < lonx < 180 and -90 < laty < 90 and return reference */
  atools::geo::Line& normalize();

  /* Return a normalized copy of this */
  atools::geo::Line normalized() const;

  /* Find point between start and end on rhumb line */
  atools::geo::Pos interpolateRhumb(float distanceMeter, float fraction) const
  {
    return pos1.interpolateRhumb(pos2, distanceMeter, fraction);
  }

  atools::geo::Pos interpolateRhumb(float fraction) const
  {
    return pos1.interpolateRhumb(pos2, fraction);
  }

private:
  friend QDataStream& operator<<(QDataStream& out, const atools::geo::Line& obj);

  friend QDataStream& operator>>(QDataStream& in, atools::geo::Line& obj);

  friend QDebug operator<<(QDebug out, const atools::geo::Line& record);

  atools::geo::Pos pos1, pos2;
};

/* Invalid line */
const atools::geo::Line EMPTY_LINE;

inline size_t qHash(const atools::geo::Line& line, size_t seed)
{
  return qHashMulti(seed, line.getPos1().getLonX(), line.getPos1().getLatY(), line.getPos2().getLonX(), line.getPos2().getLatY());
}

} // namespace geo
} // namespace atools

Q_DECLARE_TYPEINFO(atools::geo::Line, Q_PRIMITIVE_TYPE);
Q_DECLARE_METATYPE(atools::geo::Line)

#endif // ATOOLS_GEO_LINE_H
