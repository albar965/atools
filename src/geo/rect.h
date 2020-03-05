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

#ifndef ATOOLS_GEO_RECT_H
#define ATOOLS_GEO_RECT_H

#include "geo/pos.h"

#include <QDebug>

namespace atools {
namespace geo {

class LineString;

/*
 * Geographic rectangle class. Calculations based on
 *  http://williams.best.vwh.net/avform.htm
 */
class Rect
{
public:
  /* Create an invalid rectangle */
  Rect();
  Rect(const atools::geo::Rect& other);

  /* Create a single point rectangle */
  explicit Rect(const atools::geo::Pos& singlePos);
  explicit Rect(float lonX, float latY);

  explicit Rect(const atools::geo::Pos& topLeftPos, const atools::geo::Pos& bottomRightPos);
  explicit Rect(float leftLonX, float topLatY, float rightLonX, float bottomLatY);
  explicit Rect(double leftLonX, double topLatY, double rightLonX, double bottomLatY);
  explicit Rect(const LineString& linestring);

  /* Create rectangle that includes the given circle. Radius in meter. */
  Rect(const atools::geo::Pos& center, float radiusMeter);

  atools::geo::Rect& operator=(const atools::geo::Rect& other);

  bool operator==(const atools::geo::Rect& other) const;

  bool operator!=(const atools::geo::Rect& other) const
  {
    return !(*this == other);
  }

  /*
   * @return true if point is inside rectangle
   */
  bool contains(const atools::geo::Pos& pos) const;

  /*
   * @return true if rectangle overlaps this rectangle
   */
  bool overlaps(const atools::geo::Rect& other) const;

  /* add margins to this rectangle */
  atools::geo::Rect& inflate(float degreesLon, float degreesLat);

  /* Scales the rectangle keeping the center */
  atools::geo::Rect& scale(float horizontalFactor, float verticalFactor);

  const atools::geo::Pos& getTopLeft() const
  {
    return topLeft;
  }

  const atools::geo::Pos& getBottomRight() const
  {
    return bottomRight;
  }

  atools::geo::Pos getTopRight() const;
  atools::geo::Pos getBottomLeft() const;

  float getWidthDegree() const;
  float getHeightDegree() const;

  /* Get width and height of the rectangle in meter at the center coordinates.
   * This is a rought approximation at best. */
  float getWidthMeter() const;
  float getHeightMeter() const;

  float getNorth() const
  {
    return topLeft.getLatY();
  }

  float getSouth() const
  {
    return bottomRight.getLatY();
  }

  float getEast() const
  {
    return bottomRight.getLonX();
  }

  float getWest() const
  {
    return topLeft.getLonX();
  }

  void setNorth(float value)
  {
    topLeft.setLatY(value);
  }

  void setSouth(float value)
  {
    bottomRight.setLatY(value);
  }

  void  setEast(float value)
  {
    bottomRight.setLonX(value);
  }

  void  setWest(float value)
  {
    topLeft.setLonX(value);
  }

  /* Extend rectangle to include given point or rectangle.
   * Creates a singular rectangle for first call on invalid rect.
   *  Ignores invalid positions. */
  atools::geo::Rect& extend(const atools::geo::Pos& pos);
  atools::geo::Rect& extend(const atools::geo::Rect& rect);
  atools::geo::Rect& extend(const atools::geo::LineString& linestring);
  static atools::geo::Rect extended(const atools::geo::LineString& linestring);

  atools::geo::Pos getCenter() const;

  /* Returns two rectangles if this crosses the anti meridian otherwise *this. */
  QList<atools::geo::Rect> splitAtAntiMeridian() const;
  bool crossesAntiMeridian() const;

  /*
   * @return true if this rectangle was not default constructed */
  bool isValid() const
  {
    return topLeft.isValid() && bottomRight.isValid();
  }

  /*
   * @return true if rectangle is a single point
   */
  bool isPoint(float epsilonDegree = atools::geo::Pos::POS_EPSILON_MIN) const;

  /* Convert this position from rad to degree and return reference */
  atools::geo::Rect& toDeg();

  /* Convert this position from degree to rad and return reference */
  atools::geo::Rect& toRad();

  atools::geo::Pos getBottomCenter() const;
  atools::geo::Pos getTopCenter() const;
  atools::geo::Pos getLeftCenter() const;
  atools::geo::Pos getRightCenter() const;

  void swap(Rect& other);

  QString toString() const;

private:
  friend QDataStream& operator<<(QDataStream& out, const Rect& obj);

  friend QDataStream& operator>>(QDataStream& in, Rect& obj);

  friend QDebug operator<<(QDebug out, const atools::geo::Rect& record);

  bool overlapsInternal(const Rect& other) const;

  atools::geo::Pos topLeft, bottomRight;

};

const atools::geo::Rect EMPTY_RECT;

} // namespace geo
} // namespace atools

Q_DECLARE_TYPEINFO(atools::geo::Rect, Q_PRIMITIVE_TYPE);
Q_DECLARE_METATYPE(atools::geo::Rect);

#endif // ATOOLS_GEO_RECT_H
