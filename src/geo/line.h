/*****************************************************************************
* Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
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
#include "geo/rect.h"

#include <QDebug>

namespace atools {
namespace geo {

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

  /* Create a line with length 0 */
  explicit Line(const atools::geo::Pos& p);

  explicit Line(float longitudeX1, float latitudeY1, float longitudeX2, float latitudeY2);
  explicit Line(double longitudeX1, double latitudeY1, double longitudeX2, double latitudeY2);

  ~Line();

  Line& operator=(const Line& other);

  /* Does not compare altitude */
  bool operator==(const atools::geo::Line& other) const;

  /* Does not compare altitude */
  bool operator!=(const atools::geo::Line& other) const
  {
    return !(*this == other);
  }

  /* Distance to other point in simple units */
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

  /* Find point between start and end on GC route if distance between points is not known. 0 < fraction <= 1 */
  atools::geo::Pos interpolate(float fraction) const;
  void interpolatePoints(float lengthMeter, int numPoints, QList<atools::geo::Pos>& positions) const;

  /* Find point between start and end on rhumb line */
  atools::geo::Pos interpolateRhumb(float lengthMeter, float fraction) const;
  atools::geo::Pos interpolateRhumb(float fraction) const;

  /* Find the intersection of the GC line with the circle - numerical method.
   * Will find the nearest intersection point. */
  atools::geo::Pos intersectionWithCircle(const atools::geo::Pos& center, float radiusMeter,
                                          float accuracyMeter) const;

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

  /*
   * @return true if line is a single point
   */
  bool isPoint(float epsilonDegree = 0.f) const;

  const atools::geo::Pos& getPos2() const
  {
    return pos2;
  }

  void setPos2(const atools::geo::Pos& value)
  {
    pos2 = value;
  }

private:
  friend QDataStream& operator<<(QDataStream& out, const atools::geo::Line& obj);

  friend QDataStream& operator>>(QDataStream& in, atools::geo::Line& obj);

  friend QDebug operator<<(QDebug out, const atools::geo::Line& record);

  atools::geo::Pos pos1, pos2;
};

uint qHash(const atools::geo::Line& line);

} // namespace geo
} // namespace atools

Q_DECLARE_TYPEINFO(atools::geo::Line, Q_PRIMITIVE_TYPE);
Q_DECLARE_METATYPE(atools::geo::Line);

#endif // ATOOLS_GEO_LINE_H
