/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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

#include "geo/rect.h"
#include "geo/calculations.h"
#include "atools.h"

namespace atools {
namespace geo {

Rect::Rect()
{
  valid = false;
}

Rect::Rect(const Rect& other)
{
  *this = other;
}

Rect::Rect(const Pos& singlePos)
{
  topLeft = singlePos;
  bottomRight = singlePos;
  valid = true;
}

Rect::Rect(const Pos& topLeftPos, const Pos& bottomRightPos)
{
  topLeft = topLeftPos;
  bottomRight = bottomRightPos;
  valid = true;
}

Rect::Rect(float leftLonX, float topLatY, float rightLonX, float bottomLatY)
{
  topLeft = Pos(leftLonX, topLatY);
  bottomRight = Pos(rightLonX, bottomLatY);
  valid = true;
}

Rect::Rect(float lonX, float latY)
{
  topLeft = Pos(lonX, latY);
  bottomRight = Pos(lonX, latY);
  valid = true;
}

Rect::Rect(const Pos& center, float radiusMeter)
{
  using namespace atools::geo;
  Pos north = center.endpoint(radiusMeter, 0.).normalize();
  Pos east = center.endpoint(radiusMeter, 90.).normalize();
  Pos south = center.endpoint(radiusMeter, 180.).normalize();
  Pos west = center.endpoint(radiusMeter, 270.).normalize();

  topLeft = Pos(west.getLonX(), north.getLatY());
  bottomRight = Pos(east.getLonX(), south.getLatY());

  valid = true;
}

Rect& Rect::operator=(const Rect& other)
{
  topLeft = other.topLeft;
  bottomRight = other.bottomRight;
  valid = other.valid;
  return *this;
}

bool Rect::operator==(const Rect& other) const
{
  return topLeft == other.topLeft && bottomRight == other.bottomRight && valid == other.valid;
}

bool Rect::contains(const Pos& pos) const
{
  for(const Rect& r : splitAtAntiMeridian())
  {
    if(r.getWest() <= pos.getLonX() && pos.getLonX() <= r.getEast() &&
       r.getNorth() >= pos.getLatY() && pos.getLatY() >= r.getSouth())
      return true;
  }
  return false;
}

bool Rect::overlaps(const Rect& other) const
{
  for(const Rect& r1 : splitAtAntiMeridian())
    for(const Rect &r2 : other.splitAtAntiMeridian())
      if(r1.overlapsInternal(r2))
        return true;

  return false;
}

void Rect::inflate(float degreesLon, float degreesLat)
{
  if(getWest() - degreesLon > -179.f)
    topLeft.setLonX(getWest() - degreesLon);
  else
    topLeft.setLonX(-179.f);

  if(getEast() + degreesLon < 179.f)
    bottomRight.setLonX(getEast() + degreesLon);
  else
    bottomRight.setLonX(179.f);

  if(getNorth() + degreesLat < 89.f)
    topLeft.setLatY(getNorth() + degreesLat);
  else
    topLeft.setLatY(89.f);

  if(getSouth() - degreesLat > -89.f)
    bottomRight.setLatY(getSouth() - degreesLat);
  else
    bottomRight.setLatY(-89.f);
}

bool Rect::overlapsInternal(const Rect& other) const
{
  // "not (right_lonx < :leftx or left_lonx > :rightx or bottom_laty > :topy or top_laty < :bottomy) ");
  return !(getEast() < other.getWest() || getWest() > other.getEast() ||
           getSouth() > other.getNorth() || getNorth() < other.getSouth());
}

Pos Rect::getTopRight() const
{
  return Pos(bottomRight.getLonX(), topLeft.getLatY());
}

Pos Rect::getBottomLeft() const
{
  return Pos(topLeft.getLonX(), bottomRight.getLatY());
}

Pos Rect::getBottomCenter() const
{
  return Pos((topLeft.getLonX() + bottomRight.getLonX()) / 2, bottomRight.getLatY());
}

Pos Rect::getTopCenter() const
{
  return Pos((topLeft.getLonX() + bottomRight.getLonX()) / 2, topLeft.getLatY());
}

bool Rect::isPoint() const
{
  return atools::almostEqual(topLeft.getLonX(), bottomRight.getLonX()) &&
         atools::almostEqual(topLeft.getLatY(), bottomRight.getLatY());
}

float Rect::getWidthDegree() const
{
  return bottomRight.getLonX() - topLeft.getLonX();
}

float Rect::getHeightDegree() const
{
  return topLeft.getLatY() - bottomRight.getLatY();
}

float Rect::getWidthMeter() const
{
  float centerY = topLeft.getLatY() - getHeightDegree() / 2.f;
  return Pos(topLeft.getLonX(), centerY).distanceMeterTo(Pos(bottomRight.getLonX(), centerY));
}

float Rect::getHeightMeter() const
{
  float centerX = bottomRight.getLonX() - getWidthDegree() / 2.f;
  return Pos(centerX, topLeft.getLatY()).distanceMeterTo(Pos(centerX, bottomRight.getLatY()));
}

void Rect::extend(const Pos& pos)
{
  float x = pos.getLonX(), y = pos.getLatY(),
        leftLonX = topLeft.getLonX(), topLatY = topLeft.getLatY(),
        rightLonX = bottomRight.getLonX(), bottomLatY = bottomRight.getLatY();

  if(x < leftLonX || leftLonX == 0.f)
    leftLonX = x;

  if(x > rightLonX || rightLonX == 0.f)
    rightLonX = x;

  if(y > topLatY || topLatY == 0.f)
    topLatY = y;

  if(y < bottomLatY || bottomLatY == 0.f)
    bottomLatY = y;

  topLeft = Pos(leftLonX, topLatY);
  bottomRight = Pos(rightLonX, bottomLatY);
}

Pos Rect::getCenter() const
{
  return Pos((topLeft.getLonX() + bottomRight.getLonX()) / 2.f,
             (topLeft.getLatY() + bottomRight.getLatY()) / 2.f);
}

bool Rect::crossesAntiMeridian() const
{
  return getEast() < getWest() ||
         (atools::almostEqual(getEast(), 180.f) && atools::almostEqual(getWest(), -180.f));
}

QList<Rect> Rect::splitAtAntiMeridian() const
{
  if(crossesAntiMeridian())
    return QList<Rect>({Rect(getWest(), getNorth(), 180.f, getSouth()),
                        Rect(-180.f, getNorth(), getEast(), getSouth())});

  return QList<Rect>({*this});
}

void Rect::swap(Rect& other)
{
  topLeft.swap(other.topLeft);
  bottomRight.swap(other.bottomRight);
  std::swap(valid, other.valid);
}

QDebug operator<<(QDebug out, const Rect& record)
{
  QDebugStateSaver saver(out);
  out.nospace().noquote() << "Rect[" << record.topLeft.toString() << record.bottomRight.toString() << "]";
  return out;
}

} // namespace geo
} // namespace atools
