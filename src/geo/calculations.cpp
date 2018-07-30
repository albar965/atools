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

#include "geo/calculations.h"
#include "geo/line.h"
#include "geo/linestring.h"

#include <QDateTime>
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
    *rect = QRectF(center.x() - radius, center.y() - radius, 2. * radius, 2. * radius);

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

    if(startAngle != nullptr)
      *startAngle = static_cast<float>(start);

    if(spanAngle != nullptr)
      *spanAngle = static_cast<float>(span);
  }
}

void calcArcLength(const atools::geo::Line& line, const atools::geo::Pos& center, bool left,
                   float *distance, atools::geo::LineString *lines)
{
  float dist = center.distanceMeterTo(line.getPos1());
  float start = center.angleDegTo(line.getPos1());
  float end = center.angleDegTo(line.getPos2());
  float spanningAngle;

  if(distance != nullptr)
    *distance = 0.f;

  if(left)
  {
    if(start < end)
      // left = CCW 10 - 340
      spanningAngle = start + (360.f - end);
    else
      // left = CCW 260 - 100
      spanningAngle = start - end;
  }
  else
  {
    if(start < end)
      // right = CW 90 - 270
      spanningAngle = (end - start);
    else
      // right = CW 340 - 10
      spanningAngle = ((360.f - start) + end);
  }

  // Calculate number of steps for 20 degrees
  int numSteps = std::max(static_cast<int>(spanningAngle / 10.f), 1);

  float step = spanningAngle / static_cast<float>(numSteps);

  Pos last = line.getPos1();
  for(int i = 0; i <= numSteps; i++)
  {
    float angle;

    if(left)
      angle = start - static_cast<float>(i) * step;
    else
      angle = start + static_cast<float>(i) * step;

    angle = normalizeCourse(angle);
    Pos cur = center.endpoint(dist, angle).normalize();
    if(lines != nullptr)
      lines->append(cur);

    if(distance != nullptr)
      *distance += last.distanceMeterTo(cur);
    last = cur;
  }
}

void boundingRect(Rect& rect, const QVector<atools::geo::Pos>& positions)
{
  // If the line string is empty return an empty boundingbox
  if(positions.size() == 0)
  {
    rect = Rect();
    return;
  }

  float lonX = positions.begin()->getLonX(), latY = positions.begin()->getLatY();

  float north = latY;
  float south = latY;
  float west = lonX;
  float east = lonX;

  // If there's only a single node stored then the boundingbox only contains that point
  if(positions.size() == 1)
  {
    rect = Rect(west, north, east, south);
    return;
  }

  // Specifies whether the polygon crosses the IDL
  bool idlCrossed = false;

  // "idlCrossState" specifies the state concerning IDL crossage.
  // This is needed in order to create optimal bounding boxes in case of covering the IDL
  // Every time the IDL gets crossed from east to west the idlCrossState value gets
  // increased by one.
  // Every time the IDL gets crossed from west to east the idlCrossState value gets
  // decreased by one.

  int idlCrossState = 0;
  int idlMaxCrossState = 0;
  int idlMinCrossState = 0;

  // Holds values for east and west while idlCrossState != 0
  float otherWest = lonX;
  float otherEast = lonX;

  float previousLon = lonX;

  int currentSign = (lonX < 0) ? -1 : +1;
  int previousSign = currentSign;

  QVector<Pos>::ConstIterator it(positions.begin());
  QVector<Pos>::ConstIterator itEnd(positions.end());

  bool processingLastNode = false;

  while(it != itEnd)
  {
    // Get coordinates and normalize them to the desired range.
    lonX = it->getLonX();
    latY = it->getLatY();

    // Determining the maximum and minimum latitude
    if(latY > north)
      north = latY;
    if(latY < south)
      south = latY;

    currentSign = (lonX < 0.f) ? -1 : +1;

    // Once the polyline crosses the dateline the covered bounding box
    // would cover the whole [-M_PI; M_PI] range.
    // When looking separately at the longitude range that gets covered
    // east and west from the IDL we get two bounding boxes (we prefix
    // the resulting longitude range on the "other side" with "other").
    // By picking the "inner" range values we get a more appropriate
    // optimized single bounding box.

    // IDL check
    if(previousSign != currentSign && fabs(previousLon) + fabs(lonX) > 180.f)
    {

      // Initialize values for otherWest and otherEast
      if(idlCrossed == false)
      {
        otherWest = lonX;
        otherEast = lonX;
        idlCrossed = true;
      }

      // Determine the new IDL Cross State
      if(previousLon < 0.f)
      {
        idlCrossState++;
        if(idlCrossState > idlMaxCrossState)
          idlMaxCrossState = idlCrossState;
      }
      else
      {
        idlCrossState--;
        if(idlCrossState < idlMinCrossState)
          idlMinCrossState = idlCrossState;
      }
    }

    if(idlCrossState == 0)
    {
      if(lonX > east)
        east = lonX;
      if(lonX < west)
        west = lonX;
    }
    else
    {
      if(lonX > otherEast)
        otherEast = lonX;
      if(lonX < otherWest)
        otherWest = lonX;
    }

    previousLon = lonX;
    previousSign = currentSign;

    if(processingLastNode)
      break;

    ++it;

    if(positions.first() == positions.last() /* is closed */ && it == itEnd)
    {
      it = positions.begin();
      processingLastNode = true;
    }
  }

  if(idlCrossed)
  {
    if(idlMinCrossState < 0)
      east = otherEast;
    if(idlMaxCrossState > 0)
      west = otherWest;

    if((idlMinCrossState < 0 && idlMaxCrossState > 0) ||
       idlMinCrossState < -1 || idlMaxCrossState > 1 ||
       west <= east)
    {
      east = +180.f;
      west = -180.f;
      // if polygon fully in south hemisphere, contain south pole
      if(north < 0)
        south = -90.f;
      else
        north = 90.f;
    }
  }

  // explicit Rect(float leftLonX, float topLatY, float rightLonX, float bottomLatY);
  // return Rect(north, south, east, west);
  rect = Rect(west, north, east, south);
}

QRect rectToSquare(const QRect& rect)
{
  QRect retval = rect.normalized();
  if(retval.width() > retval.height())
    retval.setRect(retval.x(), retval.y() - (retval.width() - retval.height()) / 2,
                   retval.width(), retval.width());
  else if(retval.width() < retval.height())
    retval.setRect(retval.x() - (retval.height() - retval.width()) / 2, retval.y(),
                   retval.height(), retval.height());
  return retval;
}

QRectF rectToSquare(const QRectF& rect)
{
  QRectF retval = rect.normalized();
  if(retval.width() > retval.height())
    retval.setRect(retval.x(), retval.y() - (retval.width() - retval.height()) / 2.,
                   retval.width(), retval.width());
  else if(retval.width() < retval.height())
    retval.setRect(retval.x() - (retval.height() - retval.width()) / 2., retval.y(),
                   retval.height(), retval.height());
  return retval;
}

QTime calculateSunriseSunset(const atools::geo::Pos& position, const QDate& date, float zenith)
{
  // 1. first calculate the day of the year
  int dayOfYear = date.dayOfYear();

  // 2. convert the longitude to hour value and calculate an approximate time
  double longitudeHour = position.getLonX() / 15.;
  double t;
  if(zenith > 0.)
    t = dayOfYear + ((6. - longitudeHour) / 24.);
  else
    t = dayOfYear + ((18. - longitudeHour) / 24.);

  // 3. calculate the Sun's mean anomaly
  double sunMeanAnomaly = (0.9856 * t) - 3.289;

  // 4. calculate the Sun's true longitude
  double sunLongitude = sunMeanAnomaly +
                        (1.916 * sinDeg(sunMeanAnomaly)) +
                        (0.020 * sinDeg(2. * sunMeanAnomaly)) + 282.634;
  if(sunLongitude > 360.)
    sunLongitude -= 360.;
  if(sunLongitude < 0.)
    sunLongitude += 360.;

  // 5a. calculate the Sun's right ascension
  double rightAcension = atanDeg(0.91764 * tanDeg(sunLongitude));

  // 5b. right ascension value needs to be in the same quadrant as L
  double longitudeQuadrant = (floor(sunLongitude / 90.)) * 90.;
  double raQuadrant = (floor(rightAcension / 90.)) * 90.;
  rightAcension = rightAcension + (longitudeQuadrant - raQuadrant);

  // 5c. right ascension value needs to be converted into hours
  rightAcension = rightAcension / 15.;

  // 6. calculate the Sun's declination
  double sinDeclination = 0.39782 * sinDeg(sunLongitude);
  double cosDeclination = cosDeg(asinDeg(sinDeclination));

  // 7a. calculate the Sun's local hour angle
  double cosHourAngle = (cosDeg(std::abs(zenith)) - (sinDeclination * sinDeg(position.getLatY()))) /
                        (cosDeclination * cosDeg(position.getLatY()));

  if(cosHourAngle > 1.)
    // the sun never rises on this location (on the specified date)
    return QTime();

  if(cosHourAngle < -1.)
    // the sun never sets on this location (on the specified date)
    return QTime();

  // 7b. finish calculating H and convert into hours
  double hourAngle;
  if(zenith > 0.)
    hourAngle = 360. - acosDeg(cosHourAngle);
  else
    hourAngle = acosDeg(cosHourAngle);

  // H = H / 15
  hourAngle = hourAngle / 15.;

  // 8. calculate local mean time of rising/setting
  double localMeanTime = hourAngle + rightAcension - (0.06571 * t) - 6.622;

  // 9. adjust back to UTC
  double utcTime = localMeanTime - longitudeHour;

  if(utcTime > 24.)
    utcTime -= 24.;
  if(utcTime < 0.)
    utcTime += 24.;

  // Erase milliseconds
  QTime time = QTime::fromMSecsSinceStartOfDay(atools::roundToInt(utcTime * 3600) * 1000);
  return time;
}

} // namespace geo
} // namespace atools
