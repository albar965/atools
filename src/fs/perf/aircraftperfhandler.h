/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel albar965@mailbox.org
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
class SimConnectUserAircraft;
}
}
}

namespace atools {
namespace fs {
namespace perf {

class AircraftPerf;
/*
 * Collects automatic performance information from a flight being fed by simulator events.
 *
 * Calculates taxi fuel, average speed, average vertical speed and fuel flow for all flight segments.
 * Detects flight segments (climb, cruise, descent) automatically.
 *
 * All fuel numbers collected are lbs.
 *
 */
class AircraftPerfHandler
  : public QObject
{
  Q_OBJECT

public:
  explicit AircraftPerfHandler(QObject *parent);
  virtual ~AircraftPerfHandler() override;

  /* Get active flight segment as detected */
  FlightSegment getCurrentFlightSegment() const
  {
    return currentFlightSegment;
  }

  QString getCurrentFlightSegmentString() const;
  static QString getFlightSegmentString(atools::fs::perf::FlightSegment currentFlightSegment);

  /* Must be set before sending sim data events to this class. Value in ft */
  void setCruiseAltitude(float value)
  {
    cruiseAltitude = value;
  }

  /* Performance object that will be filled with data.
   * Must be set before sending sim data events to this class.
   *  Fuel unit set in this object will determine collected units. */
  void setAircraftPerformance(AircraftPerf *value)
  {
    perf = value;
  }

  /* Fuel in lbs/gal before engine start */
  float getStartFuel() const
  {
    return startFuel;
  }

  /* Fuel in lbs/gal consumed since engine start */
  float getTotalFuelConsumed() const
  {
    return totalFuelConsumed;
  }

  /* Simulator event that trigger data collection */
  void simDataChanged(const atools::fs::sc::SimConnectData& simulatorData);

signals:
  void flightSegmentChanged(const atools::fs::perf::FlightSegment& flightSegment);
  void reportUpdated();

private:
  /* -1 if below, 0 if at and 1 if above flight plan cruise altitude. Uses a altitude dependent buffer to avoid jitters. */
  int isAtCruise(const sc::SimConnectUserAircraft& aircraft) const;

  /* True if speed below or above 200 ft/min */
  bool isClimbing(const sc::SimConnectUserAircraft& aircraft) const;
  bool isDescending(const sc::SimConnectUserAircraft& aircraft) const;

  /* Sample data for current flight phase, calculate averages */
  void samplePhase(FlightSegment flightSegment, const sc::SimConnectUserAircraft& aircraft, qint64 now,
                   qint64 curSampleDuration);
  float sampleValue(qint64 lastSampleDuration, qint64 curSampleDuration, float lastValue, float curValue);

  atools::fs::perf::AircraftPerf *perf = nullptr;

  atools::fs::perf::FlightSegment currentFlightSegment = NONE;
  float cruiseAltitude = 0.f, startFuel = 0.f, totalFuelConsumed = 0.f;

  /* Check lbs/gal ratio if jetfuel or avgas */
  float weightVolRatio = 0.f;
  /* Do not calculate values more often than this */
  const static qint64 SAMPLE_TIME_MS = 500L;

  /* Last time of sample to allow calculation of averages */
  qint64 lastSampleTimeMs = 0L;
  qint64 lastCruiseSampleTimeMs = 0L, lastClimbSampleTimeMs = 0L, lastDescentSampleTimeMs = 0L;
};

} // namespace perf
} // namespace fs
} // namespace atools

#endif // ATOOLS_AIRCRAFTPERFORMANCE_H
