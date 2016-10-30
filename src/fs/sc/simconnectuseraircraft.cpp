/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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
  in >> windSpeed >> windDirection >> altitudeAboveGround >> groundAltitude
  >> ambientTemperature >> totalAirTemperature >> seaLevelPressure
  >> pitotIce >> structuralIce >> airplaneTotalWeight >> airplaneMaxGrossWeight >> airplaneEmptyWeight
  >> fuelTotalQuantity >> fuelTotalWeight >> fuelFlowPPH >> fuelFlowGPH >> magVar >> ambientVisibility
  >> localDateTime >> zuluDateTime;
}

void SimConnectUserAircraft::write(QDataStream& out) const
{
  SimConnectAircraft::write(out);
  out << windSpeed << windDirection << altitudeAboveGround << groundAltitude
      << ambientTemperature << totalAirTemperature << seaLevelPressure
      << pitotIce << structuralIce << airplaneTotalWeight << airplaneMaxGrossWeight << airplaneEmptyWeight
      << fuelTotalQuantity << fuelTotalWeight << fuelFlowPPH << fuelFlowGPH << magVar << ambientVisibility
      << localDateTime << zuluDateTime;
}

} // namespace sc
} // namespace fs
} // namespace atools
