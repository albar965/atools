/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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

#include <QDataStream>
#include <cmath>

namespace atools {
namespace geo {

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

LineString::LineString(const Pos& origin, float radiusMeter, int numSegments)
{
  int increment = 360 / numSegments;
  for(int j = 0; j < 360; j += increment)
    append(origin.endpoint(radiusMeter, j));
}

LineString::LineString(const Pos& origin, const Pos& start, const Pos& end, bool clockwise, int numSegments)
{
  float distance = origin.distanceMeterTo(start);

  float startAngle = atools::geo::normalizeCourse(origin.angleDegTo(start));
  float endAngle = atools::geo::normalizeCourse(origin.angleDegTo(end));

  if(atools::almostNotEqual(startAngle, endAngle))
  {
    float step = 360.f / static_cast<float>(numSegments);
    QList<float> angles;
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
      append(origin.endpoint(distance, angle));
  }
}

LineString LineString::reversed()
{
  LineString linestring(*this);
  linestring.reverse();
  return linestring;
}

const LineString LineString::alt(float alt) const
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
  erase(std::remove_if(begin(), end(), [](const Pos& pos) -> bool {
        return !pos.isValid();
      }), end());
}

void LineString::removeDuplicates(float epsilon)
{
  // Remove all consecutive duplicate elements from the range
  erase(std::unique(begin(), end(), [epsilon](atools::geo::Pos& p1, atools::geo::Pos& p2) -> bool {
        return p1.almostEqual(p2, epsilon);
      }), end());
}

void LineString::removeDuplicates()
{
  removeDuplicates(std::numeric_limits<float>::epsilon());
}

void LineString::distanceMeterToLineString(const Pos& pos, LineDistance& result,
                                           LineDistance *closestLineResult, int *index, const atools::geo::Rect *screenRect) const
{
  LineDistance lineResult, closestResult;
  int closestIndex = -1;

  lineResult.distance = std::numeric_limits<float>::max();
  closestResult.distance = std::numeric_limits<float>::max();

  for(int i = 0; i < size() - 1; i++)
  {
    Line line(at(i), at(i + 1));

    if(screenRect != nullptr && !line.boundingRect().overlaps(*screenRect))
      continue;

    pos.distanceMeterToLine(line.getPos1(), line.getPos2(), lineResult);
    if(lineResult.status != INVALID && std::abs(lineResult.distance) < std::abs(closestResult.distance))
    {
      closestResult = lineResult;
      closestIndex = i;
    }
  }

  if(closestIndex != -1)
  {
    result = closestResult;

    if(closestLineResult != nullptr)
      *closestLineResult = closestResult;

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
  return isEmpty() ? Line() : Line(constFirst(), constLast());
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
    return constFirst();
  else if(fraction == 1.f)
    return constLast();

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
  return static_cast<float>(lengthMeterDouble());
}

double LineString::lengthMeterDouble() const
{
  double length = 0.;
  for(int i = 0; i < size() - 1; i++)
    length += at(i).distanceMeterToDouble(at(i + 1));
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

  return atools::geo::bounding(*this);
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

bool LineString::crossesAntiMeridian() const
{
  for(int i = 1; i < size(); i++)
  {
    // True if any segment crosses
    if(atools::geo::crossesAntiMeridian(at(i - 1).getLonX(), at(i).getLonX()))
      return true;
  }
  return false;
}

const LineString LineString::splitAtAntiMeridian(bool *crossed) const
{
  if(crossed != nullptr)
    *crossed = false;

  LineString linestring;

  int correctedSize = size();
  if(isClosed())
    correctedSize--;

  if(correctedSize > 1)
  {
    for(int i = 0; i < correctedSize; i++)
    {
      const QList<Line> splitLines = atools::geo::splitAtAntiMeridian(at(atools::wrapIndex(i, correctedSize)),
                                                                      at(atools::wrapIndex(i + 1, correctedSize)));
      if(splitLines.size() == 2)
      {
        // Crossing confirmed
        if(crossed != nullptr)
          *crossed = true;

        // Add split line segments
        linestring.append(splitLines.at(0).getPos1());
        linestring.append(splitLines.at(0).getPos2());
        linestring.append(splitLines.at(1).getPos1());
      }
      else if(!splitLines.isEmpty())
        linestring.append(splitLines.constFirst().getPos1());
    }

    linestring.append(constLast());
  }
  else if(correctedSize == 1)
    linestring.append(constFirst());

  return linestring;
}

const QList<LineString> LineString::splitAtAntiMeridianList() const
{
  QList<LineString> splits;
  int correctedSize = size();
  if(isClosed())
    correctedSize--;

  if(correctedSize > 1)
  {
    LineString split;
    for(int i = 0; i < correctedSize; i++)
    {
      const QList<Line> splitLines = atools::geo::splitAtAntiMeridian(at(atools::wrapIndex(i, correctedSize)),
                                                                      at(atools::wrapIndex(i + 1, correctedSize)));
      if(splitLines.size() == 2)
      {
        // Add split line segments
        split.append(splitLines.at(0).getPos1());
        split.append(splitLines.at(0).getPos2()); // On anti-meridian

        // Add segment and start a new one
        splits.append(split);
        split.clear();

        split.append(splitLines.at(1).getPos1()); // On anti-meridian
      }
      else if(!splitLines.isEmpty())
        split.append(splitLines.constFirst().getPos1());
    }
    if(!splits.isEmpty())
      splits.first().append(split);
    else
      splits.append(split);
  }

  return splits;
}

LineString& LineString::normalize()
{
  for(Pos& pos : *this)
    pos.normalize();
  return *this;
}

const LineString LineString::normalized() const
{
  LineString retval(*this);
  return retval.normalize();
}

float LineString::getStartCourse() const
{
  if(size() >= 2)
    return at(0).angleDegTo(at(1));
  else
    return Pos::INVALID_VALUE;
}

float LineString::getEndCourse() const
{
  if(size() >= 2)
    return at(size() - 1).angleDegTo(at(size() - 2));
  else
    return Pos::INVALID_VALUE;
}

QDataStream& operator<<(QDataStream& out, const LineString& obj)
{
  out << static_cast<quint32>(obj.size());
  for(const Pos& pos : obj)
    out << pos;
  return out;
}

QDataStream& operator>>(QDataStream& in, LineString& obj)
{
  quint32 size;
  in >> size;
  for(quint32 i = 0; i < size; i++)
  {
    Pos p;
    in >> p;
    obj.append(p);
  }
  return in;
}

} // namespace geo
} // namespace atools
