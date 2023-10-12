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

#ifndef ATOOLS_UTIL_POLYGONTOOLS_H
#define ATOOLS_UTIL_POLYGONTOOLS_H

#include <QLineF>
#include <QVector>

class QPolygonF;
class QPointF;
class QRectF;

class QRect;
namespace atools {
namespace util {

/* Polyon orientation */
enum Orientation
{
  INVALID_TOO_SMALL, /* Not a polygon. Point or line. */
  CLOCKWISE, /* Points in clockwise order */
  COUNTERCLOCKWISE /* Points in counter-clockwise order */
};

/* Calculates the polygon orientation also for non convex polygons.
 * https://en.wikipedia.org/wiki/Curve_orientation#Orientation_of_a_simple_polygon */
Orientation getPolygonOrientation(const QPolygonF& polygon);

/* List of polygon lines sorted by length */
class PolygonLineDistance;
typedef QVector<PolygonLineDistance> PolygonLineDistances;

/*
 * Class defining one or more polygon segments in 2D screen space which were found visible.
 * May combine more than one polygon segment.
 */
class PolygonLineDistance
{
public:
  /* Factory method to create a list of visible polygon segments ordered by length descending.
   * screenRect: Tries to ensure full visibility. Otherwise tries to find overlapping polygons.
   * limit: returns only the longest number of polygons.
   * maxAngle: Maximum angle difference for combining segments.
   * circle Function checks if this is a circular polygon by looking at the standard deviation of the relative angles. Omitted it nullptr.*/
  static PolygonLineDistances getLongPolygonLines(const QPolygonF& polygon, const QRectF& screenRect, int limit, float maxAngle,
                                                  bool *circle = nullptr);

  /* Length of this line segment or line segments in pixel. */
  float getLength() const
  {
    return static_cast<float>(length);
  }

  /* Index of the first polygon point this segment refers too */
  int getIndexFrom() const
  {
    return indexFrom;
  }

  /* Index of the last polygon point this segment refers too */
  int getIndexTo() const
  {
    return indexTo;
  }

  /* true if not default constructed */
  bool isValid() const
  {
    return length > 0.;
  }

  /* Angle in degrees of this (combined) segment */
  float getAngle() const
  {
    return static_cast<float>(angle);
  }

  /* Straight line of segment or combined segments */
  const QLineF& getLine() const
  {
    return line;
  }

  /* Center point of this segment or combined segments */
  QPointF getCenter() const
  {
    return line.center();
  }

private:
  /* true if line is fully inside the rectangle */
  static bool isLineInsideRect(const QLineF& line, const QRectF& rect);

  /* true if line intersects the rectangle */
  static bool isLineIntersectingRect(const QLineF& line, const QRectF& rect);

  /* Create a list of polygon lines without combining them. Not visible lines are invalid */
  static PolygonLineDistances createPolyLines(const QVector<QLineF>& lines, const QRectF& screenRect, int size, bool checkIntersect,
                                              double *anglesStdDev);

  PolygonLineDistance(const QLineF& line, double distanceParam, double angleParam, int indexFromParam, int indexToParam);

  PolygonLineDistance()
    : length(0.)
  {
  }

  /* Check if this line has the same orientation up to maxAngle */
  bool hasSameAngle(const PolygonLineDistance& other, double maxAngle) const;

  double length, /* Distance in pixel */
         angle; /* Line angle - not Qt angle */
  int indexFrom, indexTo; /* Index of first and last point in segment i -> i+1 */
  QLineF line;
};

} // namespace util
} // namespace atools

Q_DECLARE_TYPEINFO(atools::util::PolygonLineDistance, Q_PRIMITIVE_TYPE);

#endif // ATOOLS_UTIL_POLYGONTOOLS_H
