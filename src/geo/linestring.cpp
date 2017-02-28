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

#include "geo/linestring.h"

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

LineString::LineString(const Pos& pos)
  : QVector({pos})
{

}

LineString::LineString(const Pos& pos1, const Pos& pos2)
  : QVector({pos1, pos2})
{

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

Rect LineString::boundingRect() const
{
  if(!isEmpty())
  {
    Rect bounding(first());

    for(const Pos& p : *this)
      bounding.extend(p);
    return bounding;
  }
  else
    return Rect();
}

float LineString::lengthMeter() const
{
  float length = 0.f;
  for(int i = 0; i < size() - 1; i++)
    length += at(i).distanceMeterTo(at(i + 1));
  return length;
}

} // namespace geo
} // namespace atools
