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

#include "fs/sc/simconnectuseraircraft.h"

#include <QDebug>
#include <QDataStream>

namespace atools {
namespace fs {
namespace sc {

SimConnectUserAircraft::SimConnectUserAircraft()
{

}

SimConnectUserAircraft::SimConnectUserAircraft(const SimConnectUserAircraft& other)
  : SimConnectAircraft(other)
{
  *this = other;
}

SimConnectUserAircraft::~SimConnectUserAircraft()
{

}

void SimConnectUserAircraft::read(QDataStream& in)
{
  SimConnectAircraft::read(in);
  in >> windSpeedKts >> windDirectionDegT >> altitudeAboveGroundFt >> groundAltitudeFt
  >> ambientTemperatureCelsius >> totalAirTemperatureCelsius >> seaLevelPressureMbar
  >> pitotIcePercent >> structuralIcePercent >> airplaneTotalWeightLbs >> airplaneMaxGrossWeightLbs >> airplaneEmptyWeightLbs
  >> fuelTotalQuantityGallons >> fuelTotalWeightLbs >> fuelFlowPPH >> fuelFlowGPH >> magVarDeg >> ambientVisibilityMeter
  >> trackMagDeg >> trackTrueDeg>> localDateTime >> zuluDateTime;
}

void SimConnectUserAircraft::write(QDataStream& out) const
{
  SimConnectAircraft::write(out);
  out << windSpeedKts << windDirectionDegT << altitudeAboveGroundFt << groundAltitudeFt
      << ambientTemperatureCelsius << totalAirTemperatureCelsius << seaLevelPressureMbar
      << pitotIcePercent << structuralIcePercent << airplaneTotalWeightLbs << airplaneMaxGrossWeightLbs << airplaneEmptyWeightLbs
      << fuelTotalQuantityGallons << fuelTotalWeightLbs << fuelFlowPPH << fuelFlowGPH << magVarDeg << ambientVisibilityMeter
      << trackMagDeg << trackTrueDeg << localDateTime << zuluDateTime;
}

} // namespace sc
} // namespace fs
} // namespace atools
