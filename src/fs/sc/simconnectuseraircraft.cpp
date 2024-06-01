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

#include "fs/sc/simconnectuseraircraft.h"

#include "geo/calculations.h"

#include <QDebug>
#include <QDataStream>

namespace atools {
namespace fs {
namespace sc {

SimConnectUserAircraft::SimConnectUserAircraft()
{

}

SimConnectUserAircraft::~SimConnectUserAircraft()
{

}

void SimConnectUserAircraft::read(QDataStream& in)
{
  SimConnectAircraft::read(in);
  in >> windSpeedKts >> windDirectionDegT >> altitudeAboveGroundFt >> groundAltitudeFt >> altitudeAutopilotFt
  >> ambientTemperatureCelsius >> totalAirTemperatureCelsius >> seaLevelPressureMbar
  >> pitotIcePercent >> structuralIcePercent
  >> aoaIcePercent >> inletIcePercent >> propIcePercent >> statIcePercent >> windowIcePercent >> carbIcePercent
  >> airplaneTotalWeightLbs >> airplaneMaxGrossWeightLbs >> airplaneEmptyWeightLbs >> fuelTotalQuantityGallons
  >> fuelTotalWeightLbs >> fuelFlowPPH >> fuelFlowGPH >> magVarDeg >> ambientVisibilityMeter >> trackMagDeg
  >> trackTrueDeg >> localDateTime >> zuluDateTime;
}

void SimConnectUserAircraft::write(QDataStream& out) const
{
  SimConnectAircraft::write(out);
  out << windSpeedKts << windDirectionDegT << altitudeAboveGroundFt << groundAltitudeFt << altitudeAutopilotFt
      << ambientTemperatureCelsius << totalAirTemperatureCelsius << seaLevelPressureMbar
      << pitotIcePercent << structuralIcePercent
      << aoaIcePercent << inletIcePercent << propIcePercent << statIcePercent << windowIcePercent << carbIcePercent
      << airplaneTotalWeightLbs << airplaneMaxGrossWeightLbs << airplaneEmptyWeightLbs << fuelTotalQuantityGallons
      << fuelTotalWeightLbs << fuelFlowPPH << fuelFlowGPH << magVarDeg << ambientVisibilityMeter << trackMagDeg
      << trackTrueDeg << localDateTime << zuluDateTime;
}

bool SimConnectUserAircraft::isJetfuel(float& weightVolRatio) const
{
  return atools::geo::isJetFuel(fuelTotalWeightLbs, fuelTotalQuantityGallons, weightVolRatio);
}

float SimConnectUserAircraft::getConsumedFuelLbs(const SimConnectUserAircraft& past) const
{
  return past.getFuelTotalWeightLbs() - getFuelTotalWeightLbs();
}

float SimConnectUserAircraft::getConsumedFuelGallons(const SimConnectUserAircraft& past) const
{
  return past.getFuelTotalQuantityGallons() - getFuelTotalQuantityGallons();
}

float SimConnectUserAircraft::getAverageFuelFlowPPH(const SimConnectUserAircraft& past) const
{
  float mins = getTravelingTimeMinutes(past);
  if(mins > 0)
    return (getConsumedFuelLbs(past) / mins) * 60.f;

  return 0.f;
}

float SimConnectUserAircraft::getAverageFuelFlowGPH(const SimConnectUserAircraft& past) const
{
  float mins = getTravelingTimeMinutes(past);
  if(mins > 0)
    return (getConsumedFuelGallons(past) / mins) * 60.f;

  return 0.f;
}

int SimConnectUserAircraft::getTravelingTimeMinutes(const SimConnectUserAircraft& past) const
{
  return static_cast<int>((getZuluTime().toSecsSinceEpoch() - past.getZuluTime().toSecsSinceEpoch()) / 60L);
}

} // namespace sc
} // namespace fs
} // namespace atools
