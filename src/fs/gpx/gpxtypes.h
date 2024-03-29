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

#ifndef ATOOLS_GPXTYPES_H
#define ATOOLS_GPXTYPES_H

#include "fs/pln/flightplan.h"
#include "geo/rect.h"

namespace atools {
namespace fs {

namespace gpx {

struct TrailPoint
{
  TrailPoint(const atools::geo::PosD& posParam, qint64 timestampParam)
    : pos(posParam), timestampMs(timestampParam)
  {
  }

  TrailPoint()
    : timestampMs(0L)
  {
  }

  atools::geo::PosD pos;
  qint64 timestampMs;
};

typedef QVector<TrailPoint> TrailPoints;
typedef QVector<TrailPoints> Trails;

/* Flight plan geometry, waypoint names and trail geometry for a logbook entry */
struct GpxData
{
  /* Saved to one of more <trk> elements */
  Trails trails;

  /* Saved to <rte> */
  atools::fs::pln::Flightplan flightplan;

  /* Bounding rectangles are filled after loading */
  atools::geo::Rect flightplanRect, trailRect;
  float maxTrailAltitude, minTrailAltitude;

  void updateBoundaries(const atools::geo::Pos& pos)
  {
    trailRect.extend(pos);
    minTrailAltitude = std::min(minTrailAltitude, pos.getAltitude());
    maxTrailAltitude = std::max(maxTrailAltitude, pos.getAltitude());
  }

  void clear();

};

} // namespace gpx
} // namespace fs
} // namespace atools

Q_DECLARE_TYPEINFO(atools::fs::gpx::TrailPoint, Q_PRIMITIVE_TYPE);

#endif // ATOOLS_GPXTYPES_H
