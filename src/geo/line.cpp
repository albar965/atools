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

#include "geo/line.h"

namespace atools {
namespace geo {

uint qHash(const atools::geo::Line& line)
{
  return qHash(line.getPos1()) ^ qHash(line.getPos2());
}

Line::Line()
{

}

Line::Line(const Line& other)
{
  this->operator=(other);

}

Line::Line(const Pos& p1, const Pos& p2)
{
  pos1 = p1;
  pos2 = p2;
}

Line::Line(float longitudeX1, float latitudeY1, float longitudeX2, float latitudeY2)
  : pos1(longitudeX1, latitudeY1), pos2(longitudeX2, latitudeY2)
{

}

Line::Line(double longitudeX1, double latitudeY1, double longitudeX2, double latitudeY2)
  : pos1(longitudeX1, latitudeY1), pos2(longitudeX2, latitudeY2)
{

}

Line::~Line()
{

}

Line& Line::operator=(const Line& other)
{
  pos1 = other.pos1;
  pos2 = other.pos2;
  return *this;
}

bool Line::operator==(const Line& other) const
{
  return pos1 == other.pos1 && pos2 == other.pos2;
}

float Line::distanceSimple() const
{
  return pos1.distanceSimpleTo(pos2);
}

float Line::distanceMeter() const
{
  return pos1.distanceMeterTo(pos2);
}

float Line::distanceMeterToLine(const Pos& pos, bool& validPos) const
{
  return pos.distanceMeterToLine(pos1, pos2, validPos);
}

float Line::angleDeg() const
{
  return pos1.angleDegTo(pos2);
}

float Line::distanceMeterRhumb() const
{
  return pos1.distanceMeterToRhumb(pos2);
}

float Line::angleDegRhumb() const
{
  return pos1.angleDegToRhumb(pos2);
}

Pos Line::interpolate(float distanceMeter, float fraction) const
{
  return pos1.interpolate(pos2, distanceMeter, fraction);
}

Pos Line::interpolate(float fraction) const
{
  return pos1.interpolate(pos2, fraction);
}

void Line::interpolatePoints(float distanceMeter, int numPoints, QList<Pos>& positions) const
{
  return pos1.interpolatePoints(pos2, distanceMeter, numPoints, positions);
}

Pos Line::interpolateRhumb(float distanceMeter, float fraction) const
{
  return pos1.interpolateRhumb(pos2, distanceMeter, fraction);
}

Pos Line::interpolateRhumb(float fraction) const
{
  return pos1.interpolateRhumb(pos2, fraction);
}

Pos Line::intersectionWithCircle(const Pos& center, float radiusMeter, float accuracyMeter) const
{
  float dist = distanceMeter();
  float minFraction = std::min(accuracyMeter / dist, 0.00001f);

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

    float step = 0.2;

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
      step /= 2.f;
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

QDebug operator<<(QDebug out, const Line& record)
{
  QDebugStateSaver saver(out);
  out.nospace().noquote() << "Line[" << record.pos1 << record.pos2 << "]";
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
