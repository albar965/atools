/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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

#include "util/roundedpolygon.h"

#include <QPainterPath>
#include <cmath>

namespace atools {
namespace util {

double RoundedPolygon::distance(QPointF pt1, QPointF pt2) const
{
  return std::sqrt((pt1.x() - pt2.x()) * (pt1.x() - pt2.x()) +
                   (pt1.y() - pt2.y()) * (pt1.y() - pt2.y()));
}

QPointF RoundedPolygon::lineStart(int i) const
{
  QPointF pt1 = at(i);
  QPointF pt2 = at((i + 1) % count());
  double ratio = radius / distance(pt1, pt2);
  if(ratio > 0.5)
    ratio = 0.5;

  return QPointF((1.0 - ratio) * pt1.x() + ratio * pt2.x(),
                 (1.0 - ratio) * pt1.y() + ratio * pt2.y());
}

QPointF RoundedPolygon::lineEnd(int i) const
{
  QPointF pt1 = at(i);
  QPointF pt2 = at((i + 1) % count());
  double ratio = radius / distance(pt1, pt2);
  if(ratio > 0.5)
    ratio = 0.5;
  return QPointF(ratio * pt1.x() + (1.0 - ratio) * pt2.x(),
                 ratio * pt1.y() + (1.0 - ratio) * pt2.y());
}

QPainterPath RoundedPolygon::getPainterPath()
{
  QPainterPath path;

  if(count() < 3)
    return path;

  QPointF pt1;
  QPointF pt2;
  for(int i = 0; i < count(); i++)
  {
    pt1 = lineStart(i);

    if(i == 0)
      path.moveTo(pt1);
    else
      path.quadTo(at(i), pt1);

    pt2 = lineEnd(i);
    path.lineTo(pt2);
  }

  pt1 = lineStart(0);
  path.quadTo(at(0), pt1);

  return path;
}

} // namespace util
} // namespace atools
