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

#include "geo/calculations.h"

#include <QLineF>
#include <QPointF>
#include <QRect>

namespace atools {

namespace geo {

float distanceToLine(float x, float y, float x1, float y1, float x2, float y2, bool lineOnly,
                     LineDist *distType)
{
  float vx = x2 - x1, vy = y2 - y1;
  float wx = x - x1, wy = y - y1;
  float dist = std::numeric_limits<float>::max();
  float c1 = vx * wx + vy * wy; // dot product
  if(c1 <= 0)
  {
    if(!lineOnly)
      dist = atools::geo::simpleDistanceF(x, y, x1, y1);
    if(distType != nullptr)
      *distType = DIST_TO_START;
  }
  else
  {
    float c2 = vx * vx + vy * vy; // dot product
    if(c2 <= c1)
    {
      if(!lineOnly)
        dist = atools::geo::simpleDistanceF(x, y, x2, y2);
      if(distType != nullptr)
        *distType = DIST_TO_END;
    }
    else
    {
      // get line in parameter form
      float a = y1 - y2, b = x2 - x1, c = x1 * y2 - x2 * y1;
      dist = std::abs(a * x + b * y + c) / std::sqrt(a * a + b * b);
      if(distType != nullptr)
        *distType = DIST_TO_LINE;
    }
  }
  return dist;
}

void arcFromPoints(const QLineF& line, const QPointF& center, bool left, QRectF *rect, float *startAngle,
                   float *spanAngle)
{
  // center of the circle is (x0, y0) and that the arc contains your two points (x1, y1) and (x2, y2).
  // Then the radius is: r=sqrt((x1-x0)(x1-x0) + (y1-y0)(y1-y0)).
  double radius = QLineF(center, line.p1()).length();

  if(rect != nullptr)
    *rect = QRectF(center.x() - radius, center.y() - radius, 2.f * radius, 2.f * radius);

  if(startAngle != nullptr || spanAngle != nullptr)
  {
    double start = normalizeCourse(toDegree(std::atan2(line.y1() - center.y(), line.x1() - center.x())));
    double end = normalizeCourse(toDegree(std::atan2(line.y2() - center.y(), line.x2() - center.x())));

    double span = 0.;
    if(left)
    {
      if(start > end)
        span = start - end;
      else
        span = 360. - end + start;
    }
    else
    {
      // negative values mean clockwise
      if(end > start)
        span = end - start;
      else
        span = 360. - start + end;
      span = -span;
    }

    if(spanAngle != nullptr)
      *startAngle = static_cast<float>(start);

    if(spanAngle != nullptr)
      *spanAngle = static_cast<float>(span);
  }
}

} // namespace geo
} // namespace atools
