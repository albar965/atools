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

#include "fs/perf/aircraftperfhandler.h"

#include "fs/perf/aircraftperf.h"
#include "fs/sc/simconnectdata.h"
#include "atools.h"
#include "fs/sc/simconnectuseraircraft.h"

namespace atools {
namespace fs {
namespace perf {

using atools::fs::sc::SimConnectUserAircraft;
using atools::fs::sc::SimConnectData;
using atools::roundToInt;

AircraftPerfHandler::AircraftPerfHandler(QObject *parent)
  : QObject(parent)
{
}

AircraftPerfHandler::~AircraftPerfHandler()
{
}

void AircraftPerfHandler::simDataChanged(const sc::SimConnectData& simulatorData)
{
  if(perf == nullptr)
    return;

  const SimConnectUserAircraft& aircraft = simulatorData.getUserAircraftConst();

  // Fill metadata if still empty
  if(perf->getAircraftType().isEmpty())
    perf->setAircraftType(aircraft.getAirplaneModel());

  if(perf->getName().isEmpty())
    perf->setName(aircraft.getAirplaneTitle());

  // Determine fuel type ========================================================
  if(atools::almostEqual(weightVolRatio, 0.f) &&
     aircraft.getFuelTotalWeightLbs() > 5.f && aircraft.getFuelTotalQuantityGallons() > 1.f)
  {
    weightVolRatio = aircraft.getFuelTotalWeightLbs() / aircraft.getFuelTotalQuantityGallons();

    if(atools::almostEqual(weightVolRatio, 6.f, 0.2f))
      perf->setAvgas();
    else if(atools::almostEqual(weightVolRatio, 6.7f, 0.3f))
      perf->setJetFuel();

    qDebug() << Q_FUNC_INFO << "weightVolRatio" << weightVolRatio << "jetfuel" << perf->isJetFuel();
  }

  // Remember fuel in tanks if not done already ========================================================
  if(startFuel < 0.1f)
  {
    startFuel = aircraft.getFuelTotalWeightLbs();
    qDebug() << Q_FUNC_INFO << "startFuel" << startFuel;
  }

  totalFuelConsumed = startFuel - aircraft.getFuelTotalWeightLbs();

  // Determine current flight sement ================================================================
  FlightSegment flightSegment = currentFlightSegment;
  if(aircraft.isValid())
  {
    switch(currentFlightSegment)
    {
      case INVALID:
        break;

      case NONE:
        // Nothing sampled yet - start from scratch ==============
        if(aircraft.isOnGround())
          flightSegment = aircraft.hasFuelFlow() ? DEPARTURE_TAXI : DEPARTURE_PARKING;
        else if(isAtCruise(aircraft) == 0)
          flightSegment = CRUISE;
        else if(isClimbing(aircraft) && isAtCruise(aircraft) == -1)
          flightSegment = CLIMB;
        else if(isDescending(aircraft) && isAtCruise(aircraft) == -1)
          flightSegment = DESCENT;
        break;

      case DEPARTURE_PARKING:
        if(aircraft.hasFuelFlow())
          flightSegment = DEPARTURE_TAXI;
        if(aircraft.isFlying())
          // Skip directly to climb if in the air
          flightSegment = CLIMB;
        break;

      case DEPARTURE_TAXI:
        if(aircraft.isFlying())
          flightSegment = CLIMB;
        break;

      case CLIMB:
        if(isAtCruise(aircraft) >= 0)
          // At cruise - 200 ft or above
          flightSegment = CRUISE;
        break;

      case CRUISE:
        if(isAtCruise(aircraft) < 0)
          // Below cruise - start descent
          flightSegment = DESCENT;
        break;

      case DESCENT:
        if(!aircraft.isFlying())
          // Landed
          flightSegment = DESTINATION_TAXI;

        if(isAtCruise(aircraft) >= 0)
          // Momentary deviation  go back to cruise
          flightSegment = CRUISE;
        break;

      case DESTINATION_TAXI:
        if(!aircraft.hasFuelFlow())
          // Engine shutdown
          flightSegment = DESTINTATION_PARKING;
        break;

      case DESTINTATION_PARKING:
        break;
    }
  }

  // Remember segment dependet sample time to allow averaging =============
  qint64 now = QDateTime::currentMSecsSinceEpoch();
  if(flightSegment != currentFlightSegment)
  {
    if(flightSegment == CLIMB)
      lastClimbSampleTimeMs = now;
    else if(flightSegment == CRUISE)
      lastCruiseSampleTimeMs = now;
    else if(flightSegment == DESCENT)
      lastDescentSampleTimeMs = now;
  }

  // Sum up taxi fuel  ========================================================
  if(currentFlightSegment == DEPARTURE_TAXI)
    perf->setTaxiFuel(startFuel - aircraft.getFuelTotalWeightLbs());

  // Sample every 500 ms ========================================
  if(now > lastSampleTimeMs + SAMPLE_TIME_MS)
  {
    samplePhase(flightSegment, aircraft, now, now - lastSampleTimeMs);
    lastSampleTimeMs = now;
  }

  // Send message is flight segment has changed  ========================
  if(flightSegment != currentFlightSegment)
  {
    currentFlightSegment = flightSegment;
    emit flightSegmentChanged(currentFlightSegment);
  }
}

float AircraftPerfHandler::sampleValue(qint64 lastSampleDuration, qint64 curSampleDuration, float lastValue,
                                       float curValue)
{
  if(lastSampleDuration == 0 || curSampleDuration == 0)
    return lastValue;

  // Calculate weighted average
  return static_cast<float>((lastValue * static_cast<double>(lastSampleDuration) +
                             curValue * static_cast<double>(curSampleDuration)) /
                            static_cast<double>(lastSampleDuration + curSampleDuration));
}

void AircraftPerfHandler::samplePhase(FlightSegment flightSegment, const SimConnectUserAircraft& aircraft,
                                      qint64 now, qint64 curSampleDuration)
{
  // Calculate all average values for each flight phase
  switch(flightSegment)
  {
    case atools::fs::perf::NONE:
    case atools::fs::perf::DEPARTURE_PARKING:
    case atools::fs::perf::INVALID:
    case atools::fs::perf::DESTINTATION_PARKING:
    case atools::fs::perf::DESTINATION_TAXI:
    case atools::fs::perf::DEPARTURE_TAXI:
      break;

    case atools::fs::perf::CLIMB:
      {
        qint64 lastSampleDuration = now - lastClimbSampleTimeMs;
        perf->setClimbSpeed(sampleValue(lastSampleDuration, curSampleDuration, perf->getClimbSpeed(),
                                        aircraft.getTrueAirspeedKts()));
        perf->setClimbVertSpeed(sampleValue(lastSampleDuration, curSampleDuration, perf->getClimbVertSpeed(),
                                            aircraft.getVerticalSpeedFeetPerMin()));
        perf->setClimbFuelFlow(sampleValue(lastSampleDuration, curSampleDuration, perf->getClimbFuelFlow(),
                                           aircraft.getFuelFlowPPH()));
      }
      break;

    case atools::fs::perf::CRUISE:
      {
        qint64 lastSampleDuration = now - lastCruiseSampleTimeMs;
        perf->setCruiseSpeed(sampleValue(lastSampleDuration, curSampleDuration, perf->getCruiseSpeed(),
                                         aircraft.getTrueAirspeedKts()));
        perf->setCruiseFuelFlow(sampleValue(lastSampleDuration, curSampleDuration, perf->getCruiseFuelFlow(),
                                            aircraft.getFuelFlowPPH()));
      }
      break;

    case atools::fs::perf::DESCENT:
      {
        qint64 lastSampleDuration = now - lastDescentSampleTimeMs;
        perf->setDescentSpeed(sampleValue(lastSampleDuration, curSampleDuration, perf->getDescentSpeed(),
                                          aircraft.getTrueAirspeedKts()));
        perf->setDescentVertSpeed(sampleValue(lastSampleDuration, curSampleDuration,
                                              perf->getDescentVertSpeed(),
                                              std::abs(aircraft.getVerticalSpeedFeetPerMin())));
        perf->setDescentFuelFlow(sampleValue(lastSampleDuration, curSampleDuration, perf->getDescentFuelFlow(),
                                             aircraft.getFuelFlowPPH()));
      }
      break;
  }
}

bool AircraftPerfHandler::isClimbing(const SimConnectUserAircraft& aircraft) const
{
  return aircraft.getVerticalSpeedFeetPerMin() > 200.f;
}

bool AircraftPerfHandler::isDescending(const SimConnectUserAircraft& aircraft) const
{
  return aircraft.getVerticalSpeedFeetPerMin() < 200.f;
}

int AircraftPerfHandler::isAtCruise(const SimConnectUserAircraft& aircraft) const
{
  float buffer = std::max(cruiseAltitude * 0.01f, 200.f);
  int result = !(aircraft.getIndicatedAltitudeFt() > cruiseAltitude - buffer &&
                 aircraft.getIndicatedAltitudeFt() < cruiseAltitude + buffer);

  if(result == 1)
  {
    // Use a larger buffer for deviations
    float buffer2 = std::max(cruiseAltitude * 0.02f, 200.f);
    if(aircraft.getIndicatedAltitudeFt() < cruiseAltitude - buffer2)
      result = -1;

    if(aircraft.getIndicatedAltitudeFt() > cruiseAltitude + buffer2)
      result = 1;
  }
  return result;
}

QString AircraftPerfHandler::getCurrentFlightSegmentString() const
{
  return getFlightSegmentString(currentFlightSegment);
}

QString AircraftPerfHandler::getFlightSegmentString(atools::fs::perf::FlightSegment currentFlightSegment)
{
  switch(currentFlightSegment)
  {
    case atools::fs::perf::INVALID:
      return tr("Invalid");

    case NONE:
      return tr("None");

    case DEPARTURE_PARKING:
      return tr("Departure Parking");

    case DEPARTURE_TAXI:
      return tr("Departure Taxi and Takeoff");

    case CLIMB:
      return tr("Climb");

    case CRUISE:
      return tr("Cruise");

    case DESCENT:
      return tr("Descent");

    case DESTINATION_TAXI:
      return tr("Destination Taxi");

    case DESTINTATION_PARKING:
      return tr("Destination Parking");

  }
  return tr("Unknown");
}

} // namespace perf
} // namespace fs
} // namespace atools
