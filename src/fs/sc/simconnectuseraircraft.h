/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_FS_SC_SIMCONNECTUSERAIRPLANE_H
#define ATOOLS_FS_SC_SIMCONNECTUSERAIRPLANE_H

#include "fs/sc/simconnectaircraft.h"

#include <QString>
#include <QDateTime>

class QIODevice;

namespace atools {
namespace fs {
namespace sc {
class SimConnectHandler;
class SimConnectData;

/*
 * User aircraft that is used to transfer across network links.
 */
class SimConnectUserAircraft :
  public SimConnectAircraft
{
public:
  SimConnectUserAircraft();
  SimConnectUserAircraft(const SimConnectUserAircraft& other);
  virtual ~SimConnectUserAircraft();

  virtual void read(QDataStream& in) override;
  virtual void write(QDataStream& out) const override;

  // fs data ----------------------------------------------------

  float getWindSpeedKts() const
  {
    return windSpeed;
  }

  float getWindDirectionDegT() const
  {
    return windDirection;
  }

  float getAltitudeAboveGroundFt() const
  {
    return altitudeAboveGround;
  }

  float getGroundAltitudeFt() const
  {
    return groundAltitude;
  }

  float getAmbientTemperatureCelsius() const
  {
    return ambientTemperature;
  }

  float getTotalAirTemperatureCelsius() const
  {
    return totalAirTemperature;
  }

  float getSeaLevelPressureMbar() const
  {
    return seaLevelPressure;
  }

  float getPitotIcePercent() const
  {
    return pitotIce;
  }

  float getStructuralIcePercent() const
  {
    return structuralIce;
  }

  float getAirplaneTotalWeightLbs() const
  {
    return airplaneTotalWeight;
  }

  float getAirplaneMaxGrossWeightLbs() const
  {
    return airplaneMaxGrossWeight;
  }

  float getAirplaneEmptyWeightLbs() const
  {
    return airplaneEmptyWeight;
  }

  float getFuelTotalQuantityGallons() const
  {
    return fuelTotalQuantity;
  }

  float getFuelTotalWeightLbs() const
  {
    return fuelTotalWeight;
  }

  float getFuelFlowPPH() const
  {
    return fuelFlowPPH;
  }

  float getFuelFlowGPH() const
  {
    return fuelFlowGPH;
  }

  float getMagVarDeg() const
  {
    return magVarDeg;
  }

  float getAmbientVisibilityMeter() const
  {
    return ambientVisibility;
  }

  const QDateTime& getLocalTime() const
  {
    return localDateTime;
  }

  const QDateTime& getZuluTime() const
  {
    return zuluDateTime;
  }

  float getTrackDegMag() const
  {
    return trackMag;
  }

  float getTrackDegTrue() const
  {
    return trackTrue;
  }

private:
  friend class atools::fs::sc::SimConnectHandler;
  friend class atools::fs::sc::SimConnectData;

  float
    altitudeAboveGround = 0.f, groundAltitude = 0.f, windSpeed = 0.f, windDirection = 0.f,
    ambientTemperature = 0.f, totalAirTemperature = 0.f,
    seaLevelPressure = 0.f, pitotIce = 0.f, structuralIce = 0.f, airplaneTotalWeight = 0.f,
    airplaneMaxGrossWeight = 0.f, airplaneEmptyWeight = 0.f, fuelTotalQuantity = 0.f,
    fuelTotalWeight = 0.f, fuelFlowPPH = 0.f, fuelFlowGPH = 0.f, magVarDeg = 0.f, ambientVisibility = 0.f;
  float trackMag = 0.f, trackTrue = 0.f;
  QDateTime localDateTime, zuluDateTime;
};

} // namespace sc
} // namespace fs
} // namespace atools

Q_DECLARE_METATYPE(atools::fs::sc::SimConnectUserAircraft);

Q_DECLARE_TYPEINFO(atools::fs::sc::SimConnectUserAircraft, Q_MOVABLE_TYPE);

#endif // ATOOLS_FS_SC_SIMCONNECTUSERAIRPLANE_H
