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

Rect::Rect(const Pos& center, float radius)
{
  using namespace atools::geo;
  Pos north = center.endpoint(radius, 0.).normalize();
  Pos east = center.endpoint(radius, 90.).normalize();
  Pos south = center.endpoint(radius, 180.).normalize();
  Pos west = center.endpoint(radius, 270.).normalize();

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

bool Rect::isPoint()
{
  return topLeft.getLonX() == bottomRight.getLonX() &&
         topLeft.getLatY() == bottomRight.getLatY();
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
  return getEast() < getWest() || (getEast() == 180.f && getWest() == -180.f);
}

QList<Rect> Rect::splitAtAntiMeridian() const
{
  if(getEast() < getWest() || (getEast() == 180.f && getWest() == -180.f))
    return QList<Rect>({Rect(getWest(), getNorth(), 180.f, getSouth()),
                        Rect(-180.f, getNorth(), getEast(), getSouth())});

  return QList<Rect>({*this});
}

QDebug operator<<(QDebug out, const Rect& record)
{
  QDebugStateSaver saver(out);
  out.nospace().noquote() << "Rect[" << record.topLeft.toString() << record.bottomRight.toString() << "]";
  return out;
}

} // namespace geo
} // namespace atools
