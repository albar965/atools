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

#include "geo/line.h"

#include "geo/calculations.h"
#include "geo/rect.h"

namespace atools {
namespace geo {

uint qHash(const atools::geo::Line& line)
{
  return qHash(line.getPos1()) ^ qHash(line.getPos2());
}

Pos Line::intersectionWithCircle(const Pos& center, float radiusMeter, float accuracyMeter) const
{
  float dist = lengthMeter();
  float minFraction = std::max(accuracyMeter / dist, 0.00001f);

  float d1 = pos1.distanceMeterTo(center);
  float d2 = pos2.distanceMeterTo(center);

  if(d1 < radiusMeter && d2 < radiusMeter)
    // All inside
    return EMPTY_POS;

  Pos result;

  if(d1 < radiusMeter || d2 < radiusMeter)
  {
    Pos p1;
    Pos p2;
    if(d1 > radiusMeter && d2 < radiusMeter)
    {
      // end inside
      p1 = pos2;
      p2 = pos1;
    }
    else
    {
      // start inside
      p1 = pos1;
      p2 = pos2;
    }

    // Use a binary search to find the closest point with distance to the center
    // This part will always find a result
    float mfraction = 0.5f, fraction = 0.5f;
    while(mfraction > minFraction)
    {
      // Check middle point
      result = p1.interpolate(p2, dist, fraction);

      mfraction /= 2.f;
      if(result.distanceMeterTo(center) < radiusMeter)
        // inside go up
        fraction += mfraction;
      else
        // outside go down
        fraction -= mfraction;
    }
  }
  else
  {
    // All outside

    float step = 0.2f;

    Pos firstInside;
    bool found = false;
    QSet<float> tested;
    while(step > minFraction && !found)
    {
      for(float fraction = 0.f; fraction <= 1.f; fraction += step)
      {
        // Check if position is not tested yet
        if(!tested.contains(fraction))
        {
          firstInside = pos1.interpolate(pos2, dist, fraction);

          if(firstInside.distanceMeterTo(center) < radiusMeter)
          {
            // Found one point inside radius
            found = true;
            break;
          }
          tested.insert(fraction);
        }
      }

      if(step > minFraction)
        step /= 2.f;
      else
        break;
    }

    if(found)
    {
      float mfraction = 0.5f, fraction = 0.5f;
      while(mfraction > minFraction)
      {
        // Check middle point
        result = firstInside.interpolate(pos1, dist, fraction);

        mfraction /= 2.f;
        if(result.distanceMeterTo(center) < radiusMeter)
          // inside go up
          fraction += mfraction;
        else
          // outside go down
          fraction -= mfraction;
      }
    }
  }

  return result;
}

Line Line::parallel(float distanceMeter)
{
  if(almostEqual(distanceMeter, 0.f))
    return *this;

  // Positive distance: Parallel to the right (looking from pos1 to pos2)
  // Negative distance: Parallel to the left (looking from pos1 to pos2)
  float course = normalizeCourse(angleDeg() + (distanceMeter > 0.f ? 90.f : -90.f));
  return Line(pos1.endpoint(std::abs(distanceMeter), course),
              pos2.endpoint(std::abs(distanceMeter), course));
}

Line Line::extended(float distanceMeter1, float distanceMeter2)
{
  Line line(*this);
  if(std::abs(distanceMeter1) > 0.f)
    line.pos1 = pos1.endpoint(distanceMeter1, opposedCourseDeg(angleDeg()));
  if(std::abs(distanceMeter2) > 0.f)
    line.pos2 = pos2.endpoint(distanceMeter2, angleDeg());
  return line;
}

Rect Line::boundingRect() const
{
  if(!isValid())
    return Rect();

  Rect retval;
  atools::geo::boundingRect(retval, {pos1, pos2});
  return retval;
}

bool Line::isPoint(float epsilonDegree) const
{
  return isValid() && pos1.almostEqual(pos2, epsilonDegree);
}

bool Line::crossesAntiMeridian() const
{
  return atools::geo::crossesAntiMeridian(pos1.getLonX(), pos2.getLonX());
}

const QList<Line> Line::splitAtAntiMeridian(bool *crossed) const
{
  if(crossed != nullptr)
    *crossed = false;

  if(isValid())
  {
    if(crossesAntiMeridian())
    {
      if(crossed != nullptr)
        *crossed = true;

      // Check for intersection with anti-meridian
      // Radial (endless from pos1) is sufficient here since crossing is already confirmed
      Pos p = Pos::intersectingRadials(pos1, angleDeg(), Pos(180.f, 90.f), 180.f);

      if(p.isValid())
      {
        // Avoid 170 -> -180 and -170 -> 180 situation
        float boundary = pos1.getLonX() > 0.f && pos2.getLonX() < 0.f ? 180.f : -180.f;

        // Return split line
        return QList<Line>({Line(pos1.getLonX(), pos1.getLatY(), boundary, p.getLatY()),
                            Line(-boundary, p.getLatY(), pos2.getLonX(), pos2.getLatY())});
      }
      // Result is invalid most likely because of points being close to anti-meridian - build line pair manually
      else if(atools::almostEqual(pos1.getLonX(), 180.f, 0.01f) && atools::almostEqual(pos2.getLonX(), -180.f, 0.01f))
        // East to west
        return QList<Line>({Line(pos1.getLonX(), pos1.getLatY(), 180.f, pos1.getLatY()),
                            Line(-180.f, pos2.getLatY(), pos2.getLonX(), pos2.getLatY())});
      else if(atools::almostEqual(pos1.getLonX(), -180.f, 0.01f) && atools::almostEqual(pos2.getLonX(), 180.f, 0.01f))
        // West to easts
        return QList<Line>({Line(pos1.getLonX(), pos1.getLatY(), -180.f, pos1.getLatY()),
                            Line(180.f, pos2.getLatY(), pos2.getLonX(), pos2.getLatY())});
    }

    // Return a copy of this
    return QList<Line>({*this});
  }
  else
    // Invalid - return empty
    return QList<Line>();
}

bool Line::isWestCourse() const
{
  return atools::geo::isWestCourse(pos1.getLonX(), pos2.getLonX());
}

bool Line::isEastCourse() const
{
  return atools::geo::isEastCourse(pos1.getLonX(), pos2.getLonX());
}

Line& Line::normalize()
{
  pos1.normalize();
  pos2.normalize();
  return *this;
}

Line Line::normalized() const
{
  Line retval(*this);
  return retval.normalize();
}

QDebug operator<<(QDebug out, const Line& record)
{
  QDebugStateSaver saver(out);
  out.nospace().noquote() << "Line[" << record.pos1 << ", " << record.pos2 << "]";
  return out;
}

QDataStream& operator<<(QDataStream& out, const Line& obj)
{
  out << obj.pos1 << obj.pos2;
  return out;
}

QDataStream& operator>>(QDataStream& in, Line& obj)
{
  in >> obj.pos1 >> obj.pos2;
  return in;
}

} // namespace geo
} // namespace atools
