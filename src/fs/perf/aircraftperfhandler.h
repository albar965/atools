/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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

  AircraftPerfHandler(const AircraftPerfHandler& other) = delete;
  AircraftPerfHandler& operator=(const AircraftPerfHandler& other) = delete;

  /* Cruise altitude must be set before sending sim data events to this class. Cruise altitude in ft.
   * Performance object that will be filled with data once started.
   * Fuel unit set in this object will determine collected units (weight/volume). */
  void start();
  void reset();

  /* Stops collection process */
  void stop();

  /* Flight plan cruise altitude. Value in ft */
  void setCruiseAltitude(float value)
  {
    cruiseAltitude = value;
  }

  /* Get active flight segment/phase as detected */
  atools::fs::perf::FlightSegment getCurrentFlightSegment() const
  {
    return currentFlightSegment;
  }

  /* Started collection after detecting engine startup / TAXI phase */
  bool hasFlightSegment() const
  {
    return currentFlightSegment != atools::fs::perf::NONE;
  }

  QString getCurrentFlightSegmentString() const;
  static QString getFlightSegmentString(atools::fs::perf::FlightSegment currentFlightSegment);

  /* Fuel in lbs consumed since engine start. */
  float getTotalFuelConsumedLbs() const
  {
    return totalFuelConsumed;
  }

  /* Simulator event that trigger data collection */
  void simDataChanged(const atools::fs::sc::SimConnectData& simulatorData);

  /* Done after landing or engine shutdown */
  bool isFinished() const;

  /* Currently collecting */
  bool isActive() const
  {
    return active;
  }

  /* Get latest updated performance. Fuel unit is always lbs. */
  const atools::fs::perf::AircraftPerf& getAircraftPerformanceLbs() const
  {
    return *perf;
  }

  /* Get a list describing aircraft status, like cruise, fuel flow, etc */
  QStringList getAircraftStatusTexts();

  const atools::fs::sc::SimConnectUserAircraft& getCurSimAircraft() const
  {
    return *curSimAircraft;
  }

signals:
  void flightSegmentChanged(const atools::fs::perf::FlightSegment& flightSegment);

private:
  /* -1 if below, 0 if at and 1 if above flight plan cruise altitude. Uses a altitude dependent buffer to avoid jitters. */
  int isAtCruise() const;

  /* True if speed below or above 200 ft/min */
  bool isClimbing() const;
  bool isDescending() const;

  /* Sample data for current flight phase, calculate averages */
  void samplePhase(FlightSegment flightSegment, qint64 now, qint64 curSampleDuration);

  /* Samples a datum for current flight phase, calculate averages */
  float sampleValue(qint64 lastSampleDuration, qint64 curSampleDuration, float lastValue, float curValue);

  atools::fs::perf::AircraftPerf *perf = nullptr;

  atools::fs::perf::FlightSegment currentFlightSegment = NONE;
  float cruiseAltitude = 0.f, startFuel = 0.f, totalFuelConsumed = 0.f;

  /* Check lbs/gal ratio if jetfuel or avgas */
  float weightVolRatio = 0.f;

  /* Do not calculate values more often than this */
  const static qint64 SAMPLE_TIME_MS = 500L;

  /* Last detected aircraft status - aggregated and therefore never null */
  atools::fs::sc::SimConnectUserAircraft *curSimAircraft;

  /* Collecting data if true. Set to false after landing. */
  bool active = false;

  bool aircraftClimb = false, aircraftDescent = false, aircraftFuelFlow = false, aircraftGround = false,
       aircraftFlying = false;

  /* -1 if below, 0 if at and 1 if above flight plan cruise altitude. Use below as default. */
  int aircraftCruise = -1;

  /* Last time of sample to allow calculation of averages */
  qint64 lastSampleTimeMs = 0L;
  qint64 lastCruiseSampleTimeMs = 0L, lastClimbSampleTimeMs = 0L, lastDescentSampleTimeMs = 0L;
};

} // namespace perf
} // namespace fs
} // namespace atools

#endif // ATOOLS_AIRCRAFTPERFORMANCE_H
