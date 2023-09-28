/*****************************************************************************
* Copyright 2015-2023 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_GEO_LINESTRING_H
#define ATOOLS_GEO_LINESTRING_H

#include "geo/pos.h"
#include "geo/rect.h"

#include <QVector>

namespace atools {
namespace geo {

class Line;

/*
 * List of geographic positions
 */
class LineString :
  public QList<atools::geo::Pos>
{
public:
  LineString()
  {

  }

  explicit LineString(const std::initializer_list<float>& coordinatePairs);

  explicit LineString(const std::initializer_list<atools::geo::Pos>& list)
    : QList(list)
  {
  }

  explicit LineString(const QList<atools::geo::Pos>& list)
    : QList(list)
  {

  }

  explicit LineString(const atools::geo::Pos& pos)
    : QList({pos})
  {

  }

  explicit LineString(const atools::geo::Pos& pos1, const atools::geo::Pos& pos2)
    : QList({pos1, pos2})
  {

  }

  /* Build a circle */
  explicit LineString(const atools::geo::Pos& origin, float radiusMeter, int numSegments);

  /* Build an arc - numSegments is related to full circle */
  explicit LineString(const atools::geo::Pos& origin, const atools::geo::Pos& start,
                      const atools::geo::Pos& end, bool clockwise, int numSegments);

  LineString(const atools::geo::LineString& other)
    : QList(other)
  {
  }

  atools::geo::LineString& operator=(const atools::geo::LineString& other)
  {
    QList::operator=(other);
    return *this;
  }

  void append(const atools::geo::Pos& pos)
  {
    QList::append(pos);
  }

  void append(const atools::geo::LineString& linestring)
  {
    QList::append(linestring);
  }

  void append(float longitudeX, float latitudeY, float alt = 0.f)
  {
    QList::append(Pos(longitudeX, latitudeY, alt));
  }

  void append(double longitudeX, double latitudeY, double alt = 0.f)
  {
    QList::append(Pos(longitudeX, latitudeY, alt));
  }

  LineString reversed();

  void reverse()
  {
    std::reverse(begin(), end());
  }

  /* Set altitude to all points and return a copy */
  atools::geo::LineString alt(float alt) const;

  /* Set altitude to all points */
  void setAltitude(float alt);

  /* Remove all invalid points */
  void removeInvalid();

  /* Remove consecutive duplicates optionally closer than epsilon (degrees) */
  void removeDuplicates(float epsilon);
  void removeDuplicates();

  /* Calculate status, cross track distance and more to this line. */
  void distanceMeterToLineString(const atools::geo::Pos& pos, atools::geo::LineDistance& result,
                                 int *index = nullptr) const;

  /* Line with first and last point */
  atools::geo::Line toLine() const;

  /* Find point between start and end on GC route if distance between points is already known.
   *  fraction is 0 <= fraction <= 1 where 0 equals first and 1 equal last pos */
  atools::geo::Pos interpolate(float fraction) const;
  atools::geo::Pos interpolate(float totalDistanceMeter, float fraction) const;

  const atools::geo::Pos& getPos1() const
  {
    return isEmpty() ? EMPTY_POS : constFirst();
  }

  const atools::geo::Pos& getPos2() const
  {
    return isEmpty() ? EMPTY_POS : constLast();
  }

  /* Typed version of mid.
   * Returns a sub-vector which contains elements from this vector, starting at position pos.
   * If length is -1 (the default), all elements after pos are included; otherwise length elements
   * (or all remaining elements if there are less than length elements) are included.*/
  atools::geo::LineString mid(int pos, int len = -1) const
  {
    return atools::geo::LineString(QList::mid(pos, len));
  }

  /* Returns a string with len number of coordinates from the beginning of the list */
  atools::geo::LineString left(int len) const
  {
    return atools::geo::LineString(QList::mid(0, len));
  }

  /* Returns a string with len number of coordinates from the end of the list */
  atools::geo::LineString right(int len) const
  {
    return atools::geo::LineString(QList::mid(size() - len));
  }

  /* Calculate Length of the line string in meter */
  float lengthMeter() const;
  double lengthMeterDouble() const;

  /* Calculate bounding rectangle of all positions considering date boundary. Expensive. */
  Rect boundingRect() const;

  bool isValid() const
  {
    return !isEmpty();
  }

  /* true if all points are valid. O(n) runtime. */
  bool hasAllValidPoints() const;

  bool isPoint() const
  {
    return size() == 1 || (size() == 2 && constFirst() == constLast());
  }

  bool isClosed() const
  {
    return isValid() && constFirst() == constLast();
  }

  /* true if longitude values cross the anti-meridian independent of direction but unreliable for large rectangles. */
  bool crossesAntiMeridian() const;

  /* Returns two lines if it crosses. Otherwise a copy of this or empty list if invalid. */
  LineString splitAtAntiMeridian(bool *crossed = nullptr) const;

  /* Normalize all positions to -180 < lonx < 180 and -90 < laty < 90 and return reference */
  atools::geo::LineString& normalize();

  /* Return a normalized copy of this */
  atools::geo::LineString normalized() const;

  /* Course from first to second point or INVALID_VALUE if isPoint() == true */
  float getStartCourse() const;

  /* Course from last to second last point or INVALID_VALUE if isPoint() == true */
  float getEndCourse() const;

private:
  friend QDebug operator<<(QDebug out, const atools::geo::LineString& record);
  friend QDataStream& operator<<(QDataStream& out, const atools::geo::LineString& obj);
  friend QDataStream& operator>>(QDataStream& in, atools::geo::LineString& obj);

};

/* Invalid lines */
const atools::geo::LineString EMPTY_LINESTRING;

} // namespace geo
} // namespace atools

Q_DECLARE_TYPEINFO(atools::geo::LineString, Q_MOVABLE_TYPE);
Q_DECLARE_METATYPE(atools::geo::LineString);

#endif // ATOOLS_GEO_LINESTRING_H
