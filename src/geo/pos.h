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

#ifndef ATOOLS_GEO_POSITION_H
#define ATOOLS_GEO_POSITION_H

#include "logging/loggingdefs.h"

namespace atools {
namespace geo {

/* Simple geographic position */
class Pos
{
public:
  Pos();
  Pos(float longitudeX, float latitudeY);
  ~Pos();

  float getLatY() const
  {
    return latY;
  }

  float getLonX() const
  {
    return lonX;
  }

protected:
  friend QDebug operator<<(QDebug out, const Pos& record);

  // Länge (x),Breite (y)
  float lonX, latY;
};

} // namespace geo
} // namespace atools

#endif /* ATOOLS_GEO_POSITION_H */
