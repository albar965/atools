/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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

#include "logging/loggingdefs.h"
#include "geo/pos.h"

namespace atools {
namespace geo {

class Rect
{
public:
  /* Create an invalid rectangle */
  Rect();

  Rect(const atools::geo::Pos& singlePos);
  Rect(const atools::geo::Pos& topLeftPos, const atools::geo::Pos& bottomRightPos);
  Rect(float leftLonX, float topLatY, float rightLonX, float bottomLatY);

  /* Create rectangle that includes the given circle. Radius in meter. */
  Rect(const atools::geo::Pos& center, float radius);

  const atools::geo::Pos& getTopLeft() const
  {
    return topLeft;
  }

  const atools::geo::Pos& getBottomRight() const
  {
    return bottomRight;
  }

  float getWidthDegree() const;
  float getHeightDegree() const;

  /* Get width and height of the rectangle in meter at the center coordinates.
   * This is a rought approximation at best. */
  float getWidthMeter() const;
  float getHeightMeter() const;

  /* Extend rectangle to include given point */
  void extend(const atools::geo::Pos& pos);

  Pos getCenter() const;

  /* Returns two rectangles if this crosses the anti meridian otherwise *this. */
  QList<Rect> crossesAntiMeridian() const;

  bool isValid() const
  {
    return valid;
  }

private:
  atools::geo::Pos topLeft, bottomRight;
  bool valid = false;
};

} // namespace geo
} // namespace atools

#endif // ATOOLS_GEO_RECT_H
