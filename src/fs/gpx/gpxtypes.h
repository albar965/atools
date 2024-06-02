/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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

typedef QVector<atools::fs::gpx::TrailPoint> TrailPoints;
typedef QVector<atools::fs::gpx::TrailPoints> Trails;

/* Flight plan geometry, waypoint names and trail geometry for a logbook entry */
class GpxData
{
public:
  const atools::fs::pln::Flightplan& getFlightplan() const
  {
    return flightplan;
  }

  const atools::fs::gpx::Trails& getTrails() const
  {
    return trails;
  }

  bool hasTrails() const
  {
    return !trails.isEmpty();
  }

  const atools::geo::Rect& getFlightplanRect() const
  {
    return flightplanRect;
  }

  const atools::geo::Rect& getTrailRect() const
  {
    return trailRect;
  }

  float getMaxTrailAltitude() const
  {
    return maxTrailAltitude;
  }

  float getMinTrailAltitude() const
  {
    return minTrailAltitude;
  }

  int getNumPoints() const
  {
    return numPoints;
  }

  // Setters =========================================================
  void clear();

  /* Add entry and update bounding rect */
  void appendFlightplanEntry(const atools::fs::pln::FlightplanEntry& entry);

  void adjustDepartureAndDestinationFlightplan()
  {
    flightplan.adjustDepartureAndDestination(true /* force */);
  }

  /* Append point list, update trailRect, min/max altitude and numPoints */
  void appendTrailPoints(const TrailPoints& line);

  /* Set flightplan and update flightplanRect */
  void setFlightplan(const atools::fs::pln::Flightplan& value);

private:
  /* Saved to one of more <trk> elements */
  Trails trails;

  int numPoints = 0;

  /* Saved to <rte> */
  atools::fs::pln::Flightplan flightplan;

  /* Bounding rectangles are filled after loading */
  atools::geo::Rect flightplanRect, trailRect;
  float maxTrailAltitude, minTrailAltitude;

};

} // namespace gpx
} // namespace fs
} // namespace atools

Q_DECLARE_TYPEINFO(atools::fs::gpx::TrailPoint, Q_PRIMITIVE_TYPE);

#endif // ATOOLS_GPXTYPES_H
