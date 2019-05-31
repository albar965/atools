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

#include "geo/linestring.h"
#include "geo/calculations.h"

#include "geo/line.h"

#include <cmath>

namespace atools {
namespace geo {

LineString::LineString()
{

}

LineString::LineString(const std::initializer_list<Pos>& list)
  : QVector(list)
{
}

LineString::LineString(const QVector<Pos>& list)
  : QVector(list)
{

}

LineString::LineString(const QList<Pos>& list)
  : QVector(list.toVector())
{

}

LineString::LineString(const std::initializer_list<float>& coordinatePairs)
{
  float lastVal = 0.f;
  int idx = 0;
  for(float val : coordinatePairs)
  {
    if((idx++ % 2) == 1)
      append(Pos(lastVal, val));

    lastVal = val;
  }
}

LineString::LineString(const Pos& pos)
  : QVector(
    {
      pos
    })
{

}

LineString::LineString(const Pos& pos1, const Pos& pos2)
  : QVector(
    {
      pos1, pos2
    })
{

}

LineString::LineString(const Pos& origin, float radiusMeter, int numSegments)
{
  int increment = 360 / numSegments;
  for(int j = 0; j < 360; j += increment)
    append(origin.endpoint(radiusMeter, j).normalize());
}

LineString::LineString(const Pos& origin, const Pos& start, const Pos& end, bool clockwise, int numSegments)
{
  float distance = origin.distanceMeterTo(start);

  float startAngle = atools::geo::normalizeCourse(origin.angleDegTo(start));
  float endAngle = atools::geo::normalizeCourse(origin.angleDegTo(end));

  if(atools::almostNotEqual(startAngle, endAngle))
  {
    float step = 360.f / static_cast<float>(numSegments);
    QVector<float> angles;
    if(clockwise)
    {
      // Clockwise - turn right
      if(startAngle < endAngle)
      {
        for(float angle = startAngle; angle < endAngle; angle += step)
          angles.append(angle);
      }
      else
      {
        for(float angle = startAngle; angle < 360.f; angle += step)
          angles.append(angle);
        for(float angle = 0.f; angle < endAngle; angle += step)
          angles.append(angle);
      }

      // Make sure to catch the end
      angles.append(endAngle);
    }
    else
    {
      // Counter clockwise - turn left
      if(startAngle > endAngle)
      {
        for(float angle = startAngle; angle > endAngle; angle -= step)
          angles.append(angle);
      }
      else
      {
        for(float angle = startAngle; angle > 0.f; angle -= step)
          angles.append(angle);
        for(float angle = 360.f; angle > endAngle; angle -= step)
          angles.append(angle);
      }

      // Make sure to catch the end
      angles.append(endAngle);
    }

    for(float angle : angles)
      append(origin.endpoint(distance, angle).normalize());
  }
}

LineString::LineString(const LineString& other)
  : QVector(other)
{
}

LineString& LineString::operator=(const LineString& other)
{
  clear();
  QVector::append(other);
  return *this;
}

void LineString::append(const Pos& pos)
{
  QVector::append(pos);
}

void LineString::append(const LineString& linestring)
{
  QVector::append(linestring);
}

void LineString::append(float longitudeX, float latitudeY, float alt)
{
  QVector::append(Pos(longitudeX, latitudeY, alt));
}

void LineString::append(double longitudeX, double latitudeY, double alt)
{
  QVector::append(Pos(longitudeX, latitudeY, alt));
}

void LineString::reverse()
{
  std::reverse(begin(), end());
}

LineString LineString::alt(float alt) const
{
  LineString retval(*this);
  retval.setAltitude(alt);
  return retval;
}

void LineString::setAltitude(float alt)
{
  for(Pos& p : *this)
    p.setAltitude(alt);
}

void LineString::removeInvalid()
{
  auto it = std::remove_if(begin(), end(),
                           [](const Pos& pos) -> bool
      {
        return !pos.isValid();
      });

  if(it != end())
    erase(it, end());
}

void LineString::removeDuplicates(float epsilon)
{
  resize(static_cast<int>(std::distance(begin(),
                                        std::unique(begin(), end(),
                                                    [ = ](atools::geo::Pos& p1, atools::geo::Pos& p2) -> bool
      {
        return p1.almostEqual(p2, epsilon);
      }))));
}

void LineString::removeDuplicates()
{
  removeDuplicates(std::numeric_limits<float>::epsilon());
}

void LineString::distanceMeterToLineString(const Pos& pos, LineDistance& result, int *index) const
{
  LineDistance lineResult, closestLineResult;
  int closestIndex = -1;

  lineResult.distance = std::numeric_limits<float>::max();
  closestLineResult.distance = std::numeric_limits<float>::max();

  for(int i = 0; i < size() - 1; i++)
  {
    pos.distanceMeterToLine(at(i), at(i + 1), lineResult);
    if(lineResult.status != INVALID &&
       std::abs(lineResult.distance) < std::abs(closestLineResult.distance))
    {
      closestLineResult = lineResult;
      closestIndex = i;
    }
  }

  if(closestIndex != -1)
  {
    result = closestLineResult;

    for(int i = 0; i < closestIndex; i++)
      result.distanceFrom1 += at(i).distanceMeterTo(at(i + 1));

    result.distanceFrom2 = lengthMeter() - result.distanceFrom1;

    if(closestIndex == 0)
    {
      if(result.status != BEFORE_START)
        result.status = ALONG_TRACK;
    }
    else if(closestIndex == size() - 2)
    {
      if(result.status != AFTER_END)
        result.status = ALONG_TRACK;
    }
    else
      result.status = ALONG_TRACK;

    if(index != nullptr)
      *index = closestIndex;
  }
  else
  {
    result.status = INVALID;
    result.distance = std::numeric_limits<float>::max();
    result.distance = std::numeric_limits<float>::max();
    if(index != nullptr)
      *index = std::numeric_limits<int>::max();
  }
}

atools::geo::Line LineString::toLine() const
{
  return isEmpty() ? Line() : Line(first(), last());
}

Pos LineString::interpolate(float fraction) const
{
  return interpolate(lengthMeter(), fraction);
}

atools::geo::Pos LineString::interpolate(float totalDistanceMeter, float fraction) const
{
  if(isEmpty() || fraction < 0.f || fraction > 1.0f)
    return atools::geo::EMPTY_POS;
  else if(fraction == 0.f)
    return first();
  else if(fraction == 1.f)
    return last();

  float distFromStartMeter = fraction * totalDistanceMeter;

  atools::geo::Pos retval;

  float total = 0.f;
  int foundIndex = -1;
  float foundDist = std::numeric_limits<float>::max();

  for(int i = 0; i < size() - 1; i++)
  {
    float dist = at(i).distanceMeterTo(at(i + 1));
    if(total + dist > distFromStartMeter)
    {
      // Distance already within leg
      foundIndex = i;
      foundDist = dist;
      break;
    }
    total += dist;
  }

  if(foundIndex != -1)
  {
    float fract = (distFromStartMeter - total) / foundDist;
    retval = at(foundIndex).interpolate(at(foundIndex + 1), fract);
  }
  return retval;
}

float LineString::lengthMeter() const
{
  float length = 0.f;
  for(int i = 0; i < size() - 1; i++)
    length += at(i).distanceMeterTo(at(i + 1));
  return length;
}

QDebug operator<<(QDebug out, const LineString& record)
{
  QDebugStateSaver saver(out);
  out.nospace().noquote() << "LineString[";

  for(const Pos& p:record)
    out.nospace().noquote() << p << ", ";

  return out;
}

Rect LineString::boundingRect() const
{
  if(!isValid())
    return Rect();

  Rect retval;
  atools::geo::boundingRect(retval, *this);
  return retval;
}

bool LineString::hasAllValidPoints() const
{
  for(const Pos& p : *this)
  {
    if(!p.isValid())
      return false;
  }
  return true;
}

} // namespace geo
} // namespace atools
