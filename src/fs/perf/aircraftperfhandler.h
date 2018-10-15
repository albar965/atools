/*****************************************************************************
* Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_AIRCRAFTPERFORMANCE_H
#define ATOOLS_AIRCRAFTPERFORMANCE_H

#include <QObject>

#include "fs/perf/aircraftperfconstants.h"

namespace atools {
namespace fs {
namespace sc {
class SimConnectData;
}
}
}

namespace atools {
namespace fs {
namespace perf {

/* Flight segment as detected by the AircraftPerfHandler for simulator events */
enum FlightSegment
{
  NONE,
  ALL,
  DEPARTURE_PARKING,
  DEPARTURE_TAXI,
  CLIMB,
  CRUISE,
  DESCENT,
  DESTINTATION_TAXI,
  DESTINTATION_PARKING
};

/*
 * Collects automatic performance information from a flight
 */
class AircraftPerfHandler
  : public QObject
{
  Q_OBJECT

public:
  AircraftPerfHandler();
  virtual ~AircraftPerfHandler() override;

  void simDataChanged(const atools::fs::sc::SimConnectData& simulatorData);

  FlightSegment getCurrentFlightSegment() const
  {
    return currentFlightSegment;
  }

signals:
  void flightSegmentChanged();
  void reportUpdated();

private:
  atools::fs::sc::SimConnectData *currentSimData = nullptr, *lastSimData = nullptr;

  FlightSegment currentFlightSegment = NONE;
};

} // namespace perf
} // namespace fs
} // namespace atools

#endif // ATOOLS_AIRCRAFTPERFORMANCE_H
