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

#ifndef ATOOLS_GEO_LINESTRING_H
#define ATOOLS_GEO_LINESTRING_H

#include "geo/pos.h"
#include "geo/rect.h"

namespace atools {
namespace geo {

/*
 * List of geographic positions
 */
class LineString :
  public QList<atools::geo::Pos>
{
public:
  LineString();
  LineString(std::initializer_list<atools::geo::Pos> list);

  void append(float longitudeX, float latitudeY, float alt = 0.f);
  void append(double longitudeX, double latitudeY, double alt = 0.f);

  /* Length of the line string in meter */
  float lengthMeter() const;

  /* Bounding rectangle of all positions */
  Rect boundingRect();

  bool isValid()
  {
    return !isEmpty();
  }

};

} // namespace geo
} // namespace atools

#endif // ATOOLS_GEO_LINESTRING_H
