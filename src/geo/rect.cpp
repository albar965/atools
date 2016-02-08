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

namespace atools {
namespace geo {

Rect::Rect()
{

}

Rect::Rect(const Pos& topLeftPos, const Pos& bottomRightPos)
{
  topLeft = topLeftPos;
  bottomRight = bottomRightPos;
}

Rect::Rect(float leftLonX, float topLatY, float rightLonX, float bottomLatY)
{
  topLeft = Pos(leftLonX, topLatY);
  bottomRight = Pos(rightLonX, bottomLatY);
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

} // namespace geo
} // namespace atools
