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

#include "fs/perf/aircraftperfhandler.h"

#include "fs/perf/aircraftperf.h"
#include "fs/sc/simconnectdata.h"
#include "atools.h"
#include "geo/calculations.h"
#include "fs/sc/simconnectuseraircraft.h"

#include <QFile>

namespace atools {
namespace fs {
namespace perf {

using atools::fs::sc::SimConnectUserAircraft;
using atools::fs::sc::SimConnectData;
using atools::roundToInt;
using atools::fs::perf::AircraftPerf;

AircraftPerfHandler::AircraftPerfHandler(QObject *parent)
  : QObject(parent)
{
  perf = new AircraftPerf;

  // Nullify all values and clear name and type
  perf->setNull();

  curSimAircraft = new SimConnectUserAircraft;
}

AircraftPerfHandler::~AircraftPerfHandler()
{
  ATOOLS_DELETE_LOG(perf);
  ATOOLS_DELETE_LOG(curSimAircraft);
}

void AircraftPerfHandler::start()
{
  currentFlightSegment = NONE;
  startFuel = totalFuelConsumed = weightVolRatio = 0.f;

  lastSampleTimeMs = lastCruiseSampleTimeMs = lastClimbSampleTimeMs = lastDescentSampleTimeMs = 0L;

  aircraftClimb = aircraftDescent = aircraftFuelFlow = aircraftGround = aircraftFlying = false;
  aircraftCruise = 0;

  perf->setNull();

  active = true;
  *curSimAircraft = SimConnectUserAircraft();
}

void AircraftPerfHandler::reset()
{
  start();
}

void AircraftPerfHandler::stop()
{
  active = false;
}

void AircraftPerfHandler::restoreCollected(const QString& filename)
{
  perf->setNull();
  perf->loadXml(filename);
  currentFlightSegment = LOADED;
}

void AircraftPerfHandler::saveCollected(const QString& filename) const
{
  perf->saveXml(filename);
}

void AircraftPerfHandler::simDataChanged(const sc::SimConnectData& simulatorData, const QString& simulator)
{
  *curSimAircraft = simulatorData.getUserAircraftConst();
  if(!active || !curSimAircraft->isFullyValid() || curSimAircraft->isSimPaused() || curSimAircraft->isSimReplay())
    return;

  aircraftClimb = isClimbing();
  aircraftDescent = isDescending();
  aircraftCruise = isAtCruise();
  aircraftFuelFlow = curSimAircraft->hasFuelFlow();
  aircraftGround = curSimAircraft->isOnGround();
  aircraftFlying = curSimAircraft->isFlying();

  // Fill metadata if still empty
  if(perf->getAircraftType().isEmpty())
    perf->setAircraftType(curSimAircraft->getAirplaneModel());

  if(perf->getName().isEmpty())
    perf->setName(curSimAircraft->getAirplaneTitle());

  if(perf->getSimulator().isEmpty())
    perf->setSimulator(simulator);

  // Determine fuel type ========================================================
  if(atools::almostEqual(weightVolRatio, 0.f))
  {
    bool jetfuel = atools::geo::isJetFuel(curSimAircraft->getFuelTotalWeightLbs(),
                                          curSimAircraft->getFuelTotalQuantityGallons(), weightVolRatio);

    if(weightVolRatio > 0.f)
    {
      perf->setJetFuel(jetfuel);
      qDebug() << Q_FUNC_INFO << "weightVolRatio" << weightVolRatio << "jetfuel" << perf->isJetFuel();
    }
    // else insufficient fuel amount
  }

  // Remember fuel in tanks if not done already ========================================================
  // Delay fuel calculation until there is fuel flow to avoid catching user changes
  // in fuel amount before flight
  if(startFuel < 0.1f && aircraftFuelFlow)
  {
    startFuel = curSimAircraft->getFuelTotalWeightLbs();
    qDebug() << Q_FUNC_INFO << "startFuel" << startFuel;
  }

  if(aircraftFuelFlow)
    totalFuelConsumed = startFuel - curSimAircraft->getFuelTotalWeightLbs();

  // Determine current flight sement ================================================================
  FlightSegment flightSegment = currentFlightSegment;
  if(curSimAircraft->isFullyValid())
  {
    switch(currentFlightSegment)
    {
      case INVALID:
        break;

      case NONE:
        // Nothing sampled yet - start from scratch ==============
        if(aircraftGround)
          flightSegment = aircraftFuelFlow ? DEPARTURE_TAXI : DEPARTURE_PARKING;
        else if(aircraftCruise >= 0)
          flightSegment = CRUISE;
        else if(isClimbing() && aircraftCruise == -1)
          flightSegment = CLIMB;
        else if(isDescending() && aircraftCruise == -1)
          flightSegment = DESCENT;
        break;

      case DEPARTURE_PARKING:
        if(aircraftFuelFlow)
          flightSegment = DEPARTURE_TAXI;
        if(aircraftFlying)
          // Skip directly to climb if in the air
          flightSegment = CLIMB;
        break;

      case DEPARTURE_TAXI:
        if(aircraftFlying)
          flightSegment = CLIMB;
        break;

      case CLIMB:
        if(aircraftCruise >= 0)
          // At cruise - 200 ft or above
          flightSegment = CRUISE;
        break;

      case CRUISE:
        if(aircraftCruise < 0)
          // Below cruise - start descent
          flightSegment = DESCENT;
        break;

      case DESCENT:
        if(!aircraftFlying)
          // Landed
          flightSegment = DESTINATION_TAXI;

        if(aircraftCruise >= 0)
          // Momentary deviation  go back to cruise
          flightSegment = CRUISE;
        break;

      case DESTINATION_TAXI:
        if(!aircraftFuelFlow)
          // Engine shutdown
          flightSegment = DESTINATION_PARKING;
        break;

      case LOADED:
      // Loaded from last session - no inactive
      case DESTINATION_PARKING:
        // Finish on engine shutdown - stop collecting
        active = false;
        break;
    }
  }

  // Remember segment dependent sample time to allow averaging =============
  qint64 aircraftZuluTime = simulatorData.getUserAircraftConst().getZuluTime().toMSecsSinceEpoch();
  if(flightSegment != currentFlightSegment)
  {
    if(flightSegment == CLIMB)
      lastClimbSampleTimeMs = aircraftZuluTime;
    else if(flightSegment == CRUISE)
      lastCruiseSampleTimeMs = aircraftZuluTime;
    else if(flightSegment == DESCENT)
      lastDescentSampleTimeMs = aircraftZuluTime;
  }

  // Sum up taxi fuel  ========================================================
  if(currentFlightSegment == DEPARTURE_TAXI && aircraftFuelFlow)
    perf->setTaxiFuel(startFuel - curSimAircraft->getFuelTotalWeightLbs());

  // Sample every 500 ms ========================================
  if(aircraftZuluTime > lastSampleTimeMs + SAMPLE_TIME_MS)
  {
    samplePhase(flightSegment, aircraftZuluTime, aircraftZuluTime - lastSampleTimeMs);
    lastSampleTimeMs = aircraftZuluTime;
  }

  // Send message is flight segment has changed  ========================
  if(flightSegment != currentFlightSegment)
  {
    currentFlightSegment = flightSegment;
    emit flightSegmentChanged(currentFlightSegment);
  }
}

bool AircraftPerfHandler::isFinished() const
{
  return currentFlightSegment == DESTINATION_TAXI || currentFlightSegment == DESTINATION_PARKING || currentFlightSegment == LOADED;
}

QStringList AircraftPerfHandler::getAircraftStatusTexts()
{
  QStringList retval;
  if(aircraftGround)
  {
    retval.append(tr("on ground"));
    if(aircraftFuelFlow)
      retval.append(tr("fuel flow"));
  }

  if(aircraftFlying)
  {
    // retval.append(tr("flight"));
    if(aircraftClimb)
      retval.append(tr("climbing"));
    else if(aircraftDescent)
      retval.append(tr("descending"));
    else
    {
      if(aircraftCruise == 0)
        retval.append(tr("at cruise altitude"));
      else if(aircraftCruise < 0)
        retval.append(tr("below cruise altitude"));
      else if(aircraftCruise > 0)
        retval.append(tr("above cruise altitude"));
    }
  }

  if(!retval.isEmpty())
  {
    QString& first = retval.first();
    if(!first.isEmpty())
    {
      QChar firstChar = first.at(0);
      first.remove(0, 1);
      first.prepend(firstChar.toUpper());
    }
  }

  return retval;
}

float AircraftPerfHandler::sampleValue(qint64 lastSampleDuration, qint64 curSampleDuration, float lastValue,
                                       float curValue)
{
  if(lastSampleDuration == 0 || curSampleDuration == 0)
    return lastValue;

  if(atools::almostEqual(lastValue, 0.f))
    return curValue;

  // Calculate weighted average
  return static_cast<float>((lastValue * static_cast<double>(lastSampleDuration) +
                             curValue * static_cast<double>(curSampleDuration)) /
                            static_cast<double>(lastSampleDuration + curSampleDuration));
}

void AircraftPerfHandler::samplePhase(FlightSegment flightSegment, qint64 now, qint64 curSampleDuration)
{
  // Calculate all average values for each flight phase
  switch(flightSegment)
  {
    case NONE:
    case DEPARTURE_PARKING:
    case INVALID:
    case DESTINATION_PARKING:
    case DESTINATION_TAXI:
    case DEPARTURE_TAXI:
    case LOADED:
      break;

    case atools::fs::perf::CLIMB:
      {
        qint64 lastSampleDuration = now - lastClimbSampleTimeMs;
        perf->setClimbSpeed(sampleValue(lastSampleDuration, curSampleDuration, perf->getClimbSpeed(),
                                        curSimAircraft->getTrueAirspeedKts()));
        perf->setClimbVertSpeed(sampleValue(lastSampleDuration, curSampleDuration, perf->getClimbVertSpeed(),
                                            curSimAircraft->getVerticalSpeedFeetPerMin()));
        perf->setClimbFuelFlow(sampleValue(lastSampleDuration, curSampleDuration, perf->getClimbFuelFlow(),
                                           curSimAircraft->getFuelFlowPPH()));
      }
      break;

    case atools::fs::perf::CRUISE:
      {
        qint64 lastSampleDuration = now - lastCruiseSampleTimeMs;
        perf->setCruiseSpeed(sampleValue(lastSampleDuration, curSampleDuration, perf->getCruiseSpeed(),
                                         curSimAircraft->getTrueAirspeedKts()));
        perf->setCruiseFuelFlow(sampleValue(lastSampleDuration, curSampleDuration, perf->getCruiseFuelFlow(),
                                            curSimAircraft->getFuelFlowPPH()));

        // Use cruise as default for alternate - user can adjust manually
        perf->setAlternateFuelFlow(perf->getCruiseFuelFlow());
        perf->setAlternateSpeed(perf->getCruiseSpeed());
      }
      break;

    case atools::fs::perf::DESCENT:
      {
        qint64 lastSampleDuration = now - lastDescentSampleTimeMs;
        perf->setDescentSpeed(sampleValue(lastSampleDuration, curSampleDuration, perf->getDescentSpeed(),
                                          curSimAircraft->getTrueAirspeedKts()));
        perf->setDescentVertSpeed(sampleValue(lastSampleDuration, curSampleDuration,
                                              perf->getDescentVertSpeed(),
                                              std::abs(curSimAircraft->getVerticalSpeedFeetPerMin())));
        perf->setDescentFuelFlow(sampleValue(lastSampleDuration, curSampleDuration, perf->getDescentFuelFlow(),
                                             curSimAircraft->getFuelFlowPPH()));
      }
      break;
  }
}

bool AircraftPerfHandler::isClimbing() const
{
  return curSimAircraft->getVerticalSpeedFeetPerMin() > 150.f;
}

bool AircraftPerfHandler::isDescending() const
{
  return curSimAircraft->getVerticalSpeedFeetPerMin() < -150.f;
}

int AircraftPerfHandler::isAtCruise() const
{
  float buffer = std::max(cruiseAltitude * 0.01f, 200.f);
  int result = !(curSimAircraft->getIndicatedAltitudeFt() > cruiseAltitude - buffer &&
                 curSimAircraft->getIndicatedAltitudeFt() < cruiseAltitude + buffer);

  if(result == 1)
  {
    // Use a larger buffer for deviations
    float buffer2 = std::max(cruiseAltitude * 0.02f, 200.f);
    if(curSimAircraft->getIndicatedAltitudeFt() < cruiseAltitude - buffer2)
      result = -1;

    if(curSimAircraft->getIndicatedAltitudeFt() > cruiseAltitude + buffer2)
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

    case DESTINATION_PARKING:
      return tr("Destination Parking");

    case LOADED:
      return tr("Loaded from last session");

  }
  return tr("Unknown");
}

} // namespace perf
} // namespace fs
} // namespace atools
