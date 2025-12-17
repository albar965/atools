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

#ifndef ROUNDEDPOLYGON_H
#define ROUNDEDPOLYGON_H

#include <QPolygonF>

class QPainterPath;

namespace atools {
namespace util {

/*
 * Draws a polyon with rounded corners.
 * Code/concept from https://www.toptal.com/c-plus-plus/rounded-corners-bezier-curves-qpainter
 */
class RoundedPolygon :
  public QPolygonF
{
public:
  RoundedPolygon(float cornerRadius)
    : radius(cornerRadius)
  {
  }

  RoundedPolygon(float cornerRadius, const QList<QPointF>& points)
    : QPolygonF(points), radius(cornerRadius)
  {
  }

  /* Builds a path of the polygon with rounded corners */
  QPainterPath getPainterPath();

private:
  QPointF lineStart(int i) const;
  QPointF lineEnd(int i) const;
  double distance(const QPointF& pt1, const QPointF& pt2) const;

  float radius;
};

} // namespace util
} // namespace atools
#endif // ROUNDEDPOLYGON_H
