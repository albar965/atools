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

#include "fs/perf/aircraftperfhandler.h"

#include "atools.h"
#include "fs/perf/aircraftperf.h"
#include "fs/sc/simconnectdata.h"
#include "fs/sc/simconnectuseraircraft.h"
#include "geo/calculations.h"
#include "settings/settings.h"

#include <QFile>

namespace atools {
namespace fs {
namespace perf {

using atools::fs::perf::AircraftPerf;
using atools::fs::sc::SimConnectData;
using atools::fs::sc::SimConnectUserAircraft;
using atools::roundToInt;

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
  *curSimAircraft = SimConnectUserAircraft();
}

void AircraftPerfHandler::reset()
{
  currentFlightSegment = NONE;
  startFuel = totalFuelConsumed = weightVolRatio = 0.f;

  lastSampleTimeMs = lastCruiseSampleTimeMs = lastClimbSampleTimeMs = lastDescentSampleTimeMs = 0L;

  aircraftClimb = aircraftDescent = aircraftFuelFlow = aircraftGround = aircraftFlying = false;
  aircraftCruise = 0;

  perf->setNull();
  *curSimAircraft = SimConnectUserAircraft();
}

void AircraftPerfHandler::restoreState(const QString& filename, const QString& settingsKeyPrefix)
{
  reset();

  if(atools::checkFile(Q_FUNC_INFO, filename))
    perf->loadXml(filename);

  atools::settings::Settings& settings = atools::settings::Settings::instance();
  totalFuelConsumed = settings.valueFloat(settingsKeyPrefix % QStringLiteral("FuelConsumed"));
  startFuel = settings.valueFloat(settingsKeyPrefix % QStringLiteral("StartFuel"));
  currentFlightSegment = settings.valueEnum<FlightSegment>(settingsKeyPrefix % QStringLiteral("CurrentFlightSegment"), NONE);
  lastClimbSampleTimeMs = settings.valueLongLong(settingsKeyPrefix % QStringLiteral("ClimbSampleTime"));
  lastCruiseSampleTimeMs = settings.valueLongLong(settingsKeyPrefix % QStringLiteral("CruiseSampleTime"));
  lastDescentSampleTimeMs = settings.valueLongLong(settingsKeyPrefix % QStringLiteral("DescentSampleTime"));
}

void AircraftPerfHandler::saveState(const QString& filename, const QString& settingsKeyPrefix) const
{
  perf->saveXml(filename);

  atools::settings::Settings& settings = atools::settings::Settings::instance();
  settings.setValue(settingsKeyPrefix % QStringLiteral("FuelConsumed"), totalFuelConsumed);
  settings.setValue(settingsKeyPrefix % QStringLiteral("StartFuel"), startFuel);
  settings.setValue(settingsKeyPrefix % QStringLiteral("CurrentFlightSegment"), currentFlightSegment);
  settings.setValue(settingsKeyPrefix % QStringLiteral("ClimbSampleTime"), lastClimbSampleTimeMs);
  settings.setValue(settingsKeyPrefix % QStringLiteral("CruiseSampleTime"), lastCruiseSampleTimeMs);
  settings.setValue(settingsKeyPrefix % QStringLiteral("DescentSampleTime"), lastDescentSampleTimeMs);
}

void AircraftPerfHandler::simDataChanged(const sc::SimConnectData& simulatorData, const QString& simulator)
{
  *curSimAircraft = simulatorData.getUserAircraftConst();

#ifdef DEBUG_INFORMATION_PERF_COLLECTION
  qDebug() << Q_FUNC_INFO
           << "curSimAircraft->isFullyValid()" << curSimAircraft->isFullyValid()
           << "curSimAircraft->isSimPaused()" << curSimAircraft->isSimPaused()
           << "curSimAircraft->isSimReplay()" << curSimAircraft->isSimReplay();

  qDebug() << Q_FUNC_INFO << "lastClimbSampleTimeMs" << lastClimbSampleTimeMs << "lastCruiseSampleTimeMs" << lastCruiseSampleTimeMs
           << "lastDescentSampleTimeMs" << lastDescentSampleTimeMs;
#endif

  // Bail out if aircraft is invalid or paused =====================
  if(!curSimAircraft->isFullyValid() || curSimAircraft->isSimPaused() || curSimAircraft->isSimReplay())
    return;

  aircraftClimb = isClimbing();
  aircraftDescent = isDescending();
  aircraftCruise = isAtCruise();
  aircraftFuelFlow = curSimAircraft->hasFuelFlow();
  aircraftGround = curSimAircraft->isOnGround();
  aircraftFlying = curSimAircraft->isFlying();

  // Stop calculation at touchdown if collection has finished ================
  if(!isFinished())
  {
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
  }

  // Determine current flight sement ================================================================
  // Continue even after touchdown for DESTINATION_PARKING
  FlightSegment flightSegment = currentFlightSegment;
  if(curSimAircraft->isFullyValid())
  {
#ifdef DEBUG_INFORMATION_PERF_COLLECTION
    qDebug() << Q_FUNC_INFO << "currentFlightSegment" << getFlightSegmentString(currentFlightSegment);
#endif
    switch(currentFlightSegment)
    {
      case INVALID:
        break;

      case NONE: // Nothing sampled yet - start from scratch ==============
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

      case DESTINATION_PARKING:
        // Finish on engine shutdown - stops collecting
        break;
    }
  }

  // Stop sampling at touchdown if collection has finished ================
  if(!isFinished())
  {
    // Remember segment dependent sample time to allow averaging =============
    qint64 aircraftZuluTimeMs = simulatorData.getUserAircraftConst().getZuluTime().toMSecsSinceEpoch();
    if(flightSegment != currentFlightSegment)
    {
#ifdef DEBUG_INFORMATION_PERF_COLLECTION
      qDebug() << Q_FUNC_INFO << "currentFlightSegment" << getFlightSegmentString(currentFlightSegment);
      qDebug() << Q_FUNC_INFO << "flightSegment" << getFlightSegmentString(flightSegment);
#endif

      if(flightSegment == CLIMB)
        lastClimbSampleTimeMs = aircraftZuluTimeMs;
      else if(flightSegment == CRUISE)
        lastCruiseSampleTimeMs = aircraftZuluTimeMs;
      else if(flightSegment == DESCENT)
        lastDescentSampleTimeMs = aircraftZuluTimeMs;
    }

    // Sum up taxi fuel  ========================================================
    if(currentFlightSegment == DEPARTURE_TAXI && aircraftFuelFlow)
      perf->setTaxiFuel(startFuel - curSimAircraft->getFuelTotalWeightLbs());

    // Sample every 500 ms ========================================
    if(aircraftZuluTimeMs > lastSampleTimeMs + SAMPLE_TIME_MS)
    {
#ifdef DEBUG_INFORMATION_PERF_COLLECTION
      qDebug() << Q_FUNC_INFO << "aircraftZuluTime - lastSampleTimeMs" << aircraftZuluTimeMs - lastSampleTimeMs;
      qDebug() << Q_FUNC_INFO << "aircraftZuluTime" << aircraftZuluTimeMs;
      qDebug() << Q_FUNC_INFO << "lastSampleTimeMs" << lastSampleTimeMs;
#endif

      samplePhase(flightSegment, aircraftZuluTimeMs, aircraftZuluTimeMs - lastSampleTimeMs);
      lastSampleTimeMs = aircraftZuluTimeMs;
    }
  }

  // Send message is flight segment has changed also for DESTINATION_PARKING ========================
  if(flightSegment != currentFlightSegment)
  {
#ifdef DEBUG_INFORMATION_PERF_COLLECTION
    qDebug() << Q_FUNC_INFO << "currentFlightSegment" << getFlightSegmentString(currentFlightSegment);
    qDebug() << Q_FUNC_INFO << "flightSegment" << getFlightSegmentString(flightSegment);
#endif

    currentFlightSegment = flightSegment;
    emit flightSegmentChanged(currentFlightSegment);
  }
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

  // Make first character upper case
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

  // Add dash for empty list to avoid list formatting
  if(retval.isEmpty())
    retval.append(tr("—"));

  return retval;
}

float AircraftPerfHandler::sampleValue(qint64 lastSampleDuration, qint64 curSampleDuration, float lastValue, float curValue)
{
#ifdef DEBUG_INFORMATION_PERF_COLLECTION
  qDebug() << Q_FUNC_INFO << getCurrentFlightSegmentString() << "lastSampleDuration" << lastSampleDuration << "curSampleDuration"
           << curSampleDuration << "lastValue" << lastValue << "curValue" << curValue;
#endif

  if(lastSampleDuration == 0 || curSampleDuration == 0)
    return lastValue;

  if(atools::almostEqual(lastValue, 0.f))
    return curValue;

  // Calculate weighted average
  double value = (lastValue * static_cast<double>(lastSampleDuration) + curValue * static_cast<double>(curSampleDuration)) /
                 static_cast<double>(lastSampleDuration + curSampleDuration);

#ifdef DEBUG_INFORMATION_PERF_COLLECTION
  qDebug() << Q_FUNC_INFO << "value" << value;
#endif

  return static_cast<float>(value);
}

void AircraftPerfHandler::samplePhase(FlightSegment flightSegment, qint64 aircraftZuluTime, qint64 curSampleDuration)
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
      break;

    case atools::fs::perf::CLIMB:
      {
        qint64 lastSampleDuration = aircraftZuluTime - lastClimbSampleTimeMs;
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
        qint64 lastSampleDuration = aircraftZuluTime - lastCruiseSampleTimeMs;
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
        qint64 lastSampleDuration = aircraftZuluTime - lastDescentSampleTimeMs;
        perf->setDescentSpeed(sampleValue(lastSampleDuration, curSampleDuration, perf->getDescentSpeed(),
                                          curSimAircraft->getTrueAirspeedKts()));
        perf->setDescentVertSpeed(sampleValue(lastSampleDuration, curSampleDuration, perf->getDescentVertSpeed(),
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
  }
  return tr("Unknown");
}

} // namespace perf
} // namespace fs
} // namespace atools
