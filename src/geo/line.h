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

#ifndef ATOOLS_GEO_LINE_H
#define ATOOLS_GEO_LINE_H

#include "geo/pos.h"

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
  Line();
  Line(const atools::geo::Line& other);
  Line(const atools::geo::Pos& p1, const atools::geo::Pos& p2);
  Line(const atools::geo::Pos& pos, float distanceMeter, float course);

  /* Create a line with length 0 */
  explicit Line(const atools::geo::Pos& p);

  explicit Line(float longitudeX1, float latitudeY1, float longitudeX2, float latitudeY2);
  explicit Line(double longitudeX1, double latitudeY1, double longitudeX2, double latitudeY2);

  atools::geo::Line& operator=(const atools::geo::Line& other)
  {
    pos1 = other.pos1;
    pos2 = other.pos2;
    return *this;
  }

  /* Does not compare altitude */
  bool operator==(const atools::geo::Line& other) const;

  /* Does not compare altitude */
  bool operator!=(const atools::geo::Line& other) const
  {
    return !(*this == other);
  }

  /* Distance to other point in simple units. Uses manhattan distance in degrees. */
  float lengthSimple() const;

  /* Distance to other point for great circle route */
  float lengthMeter() const;

  /* Calculate status, cross track distance and more to this line. */
  void distanceMeterToLine(const atools::geo::Pos& pos, atools::geo::LineDistance& result) const;

  /* Angle of line (initial course) */
  float angleDeg() const;

  /* Distance to other point for rhumb line */
  float distanceMeterRhumb() const;

  /* Angleto other point using a rhumb line */
  float angleDegRhumb() const;

  /* Find point between start and end on GC route if distance between points is already known.
   *  fraction is 0 <= fraction <= 1 where 0 equals this and 1 equal other pos */
  atools::geo::Pos interpolate(float lengthMeter, float fraction) const;

  /* Find point between start and end on GC route if distance between points is not known. 0 < fraction <= 1.*/
  atools::geo::Pos interpolate(float fraction) const;

  /* Returns a list of points which includes pos1 and not pos2. */
  void interpolatePoints(float distanceMeter, int numPoints, atools::geo::LineString& positions) const;
  void interpolatePointsRhumb(float distanceMeter, int numPoints, atools::geo::LineString& positions) const;

  /* Find point between start and end on rhumb line */
  atools::geo::Pos interpolateRhumb(float lengthMeter, float fraction) const;
  atools::geo::Pos interpolateRhumb(float fraction) const;

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
  bool isPoint(float epsilonDegree = std::numeric_limits<float>::epsilon()) const;

  const atools::geo::Pos& getPos2() const
  {
    return pos2;
  }

  void setPos2(const atools::geo::Pos& value)
  {
    pos2 = value;
  }

  /* true if longitude values cross the anti-meridian independent of direction but unreliable for large rectangles. */
  bool crossesAntiMeridian() const;

  /* Returns two lines if it crosses. Otherwise a copy of this or empty list if invalid. */
  QList<Line> splitAtAntiMeridian(bool *crossed = nullptr) const;

  /* true if heading of "from" to "to" is towards west or east. */
  bool isWestCourse() const;

  bool isEastCourse() const;

private:
  friend QDataStream& operator<<(QDataStream& out, const atools::geo::Line& obj);

  friend QDataStream& operator>>(QDataStream& in, atools::geo::Line& obj);

  friend QDebug operator<<(QDebug out, const atools::geo::Line& record);

  atools::geo::Pos pos1, pos2;
};

/* Invalid line */
const atools::geo::Line EMPTY_LINE;

uint qHash(const atools::geo::Line& line);

} // namespace geo
} // namespace atools

Q_DECLARE_TYPEINFO(atools::geo::Line, Q_PRIMITIVE_TYPE);
Q_DECLARE_METATYPE(atools::geo::Line);

#endif // ATOOLS_GEO_LINE_H
