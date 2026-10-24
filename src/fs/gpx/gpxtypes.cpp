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

#include "fs/gpx/gpxtypes.h"

namespace atools {
namespace fs {
namespace gpx {

const geo::Rect GpxData::getTrailRect(int numPoints) const
{
  atools::geo::Rect rect;

  int numPts = 0;
  for(int i = trails.size() - 1; i >= 0; i--)
  {
    const TrailPoints& points = trails.at(i);
    for(int j = points.size() - 1; j >= 0; j--)
    {
      rect.extend(points.at(j).getPos());

      if(++numPts > numPoints)
        return rect;
    }
  }

  return rect;
}

void GpxData::clear()
{
  trails.clear();
  flightplan.clearAll();
  flightplanRect = trailRect = atools::geo::Rect();
  minTrailAltitude = std::numeric_limits<float>::max();
  maxTrailAltitude = std::numeric_limits<float>::min();
  numPoints = 0;
}

void GpxData::appendFlightplanEntry(const pln::FlightplanEntry& entry)
{
  flightplan.append(entry);
  flightplanRect.extend(entry.getPosition());
}

void GpxData::appendTrailPoints(const TrailPoints& line)
{
  trails.append(line);

  for(int i = 0; i < line.size(); i++)
  {
    const atools::geo::Pos pos = line.at(i).pos.asPos();
    trailRect.extend(pos);
    minTrailAltitude = std::min(minTrailAltitude, pos.getAltitude());
    maxTrailAltitude = std::max(maxTrailAltitude, pos.getAltitude());
  }

  numPoints += line.size();
}

void GpxData::setFlightplan(const pln::Flightplan& value)
{
  flightplan = value;

  for(const atools::fs::pln::FlightplanEntry& entry : std::as_const(flightplan))
    flightplanRect.extend(entry.getPosition());
}

} // namespace gpx
} // namespace fs
} // namespace atools
