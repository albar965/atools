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

#ifndef ATOOLS_GEO_RECT_H
#define ATOOLS_GEO_RECT_H

#include "geo/pos.h"

#include <QDebug>

namespace atools {
namespace geo {

/*
 * Geographic rectangle class. Calculations based on
 *  http://williams.best.vwh.net/avform.htm
 */
class Rect
{
public:
  /* Create an invalid rectangle */
  Rect();
  Rect(const Rect& other);
  /* Create a single point rectangle */
  Rect(const atools::geo::Pos& singlePos);
  Rect(const atools::geo::Pos& topLeftPos, const atools::geo::Pos& bottomRightPos);
  Rect(float leftLonX, float topLatY, float rightLonX, float bottomLatY);
  Rect(double leftLonX, double topLatY, double rightLonX, double bottomLatY);
  Rect(float lonX, float latY);

  /* Create rectangle that includes the given circle. Radius in meter. */
  Rect(const atools::geo::Pos& center, float radiusMeter);

  Rect& operator=(const Rect& other);

  bool operator==(const Rect& other) const;

  bool operator!=(const Rect& other) const
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
  bool overlaps(const Rect& other) const;

  /* add margins to this rectangle */
  void inflate(float degreesLon, float degreesLat);

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

  /* Extend rectangle to include given point */
  void extend(const atools::geo::Pos& pos);

  Pos getCenter() const;

  /* Returns two rectangles if this crosses the anti meridian otherwise *this. */
  QList<Rect> splitAtAntiMeridian() const;
  bool crossesAntiMeridian() const;

  /*
   * @return true if this rectangle was not default constructed */
  bool isValid() const
  {
    return valid;
  }

  /*
   * @return true if rectangle is a single point
   */
  bool isPoint(float epsilonDegree = 0.f) const;

  /* Convert this position from rad to degree and return reference */
  Rect& toDeg();

  /* Convert this position from degree to rad and return reference */
  Rect& toRad();

  Pos getBottomCenter() const;
  Pos getTopCenter() const;

  void swap(Rect& other);

private:
  friend QDataStream& operator<<(QDataStream& out, const Rect& obj);

  friend QDataStream& operator>>(QDataStream& in, Rect& obj);

  friend QDebug operator<<(QDebug out, const atools::geo::Rect& record);

  bool overlapsInternal(const Rect& other) const;

  atools::geo::Pos topLeft, bottomRight;
  bool valid = false;

};

const atools::geo::Rect EMPTY_RECT;

} // namespace geo
} // namespace atools

Q_DECLARE_TYPEINFO(atools::geo::Rect, Q_PRIMITIVE_TYPE);
Q_DECLARE_METATYPE(atools::geo::Rect);

#endif // ATOOLS_GEO_RECT_H
