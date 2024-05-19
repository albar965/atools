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

#ifndef ATOOLS_FS_SC_SIMCONNECTUSERAIRPLANE_H
#define ATOOLS_FS_SC_SIMCONNECTUSERAIRPLANE_H

#include "fs/sc/simconnectaircraft.h"

#include <QString>
#include <QDateTime>

class QIODevice;

namespace xpc {
class XpConnect;

}
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

  virtual void read(QDataStream & in) override;
  virtual void write(QDataStream& out) const override;

  // fs data ----------------------------------------------------

  float getWindSpeedKts() const
  {
    return windSpeedKts;
  }

  float getWindDirectionDegT() const
  {
    return windDirectionDegT;
  }

  float getAltitudeAboveGroundFt() const
  {
    return altitudeAboveGroundFt;
  }

  float getGroundAltitudeFt() const
  {
    return groundAltitudeFt;
  }

  float getAmbientTemperatureCelsius() const
  {
    return ambientTemperatureCelsius;
  }

  float getTotalAirTemperatureCelsius() const
  {
    return totalAirTemperatureCelsius;
  }

  float getSeaLevelPressureMbar() const
  {
    return seaLevelPressureMbar;
  }

  float getPitotIcePercent() const
  {
    return pitotIcePercent;
  }

  float getStructuralIcePercent() const
  {
    return structuralIcePercent;
  }

  float getAoaIcePercent() const
  {
    return aoaIcePercent;
  }

  float getInletIcePercent() const
  {
    return inletIcePercent;
  }

  float getPropIcePercent() const
  {
    return propIcePercent;
  }

  float getStatIcePercent() const
  {
    return statIcePercent;
  }

  float getWindowIcePercent() const
  {
    return windowIcePercent;
  }

  float getCarbIcePercent() const
  {
    return carbIcePercent;
  }

  float getAirplaneTotalWeightLbs() const
  {
    return airplaneTotalWeightLbs;
  }

  float getAirplaneMaxGrossWeightLbs() const
  {
    return airplaneMaxGrossWeightLbs;
  }

  float getAirplaneEmptyWeightLbs() const
  {
    return airplaneEmptyWeightLbs;
  }

  float getFuelTotalGalLbs(bool volume) const
  {
    return volume ? fuelTotalQuantityGallons : fuelTotalWeightLbs;
  }

  float getFuelTotalQuantityGallons() const
  {
    return fuelTotalQuantityGallons;
  }

  float getFuelTotalWeightLbs() const
  {
    return fuelTotalWeightLbs;
  }

  float getFuelFlowGalLbsPerHour(bool volume) const
  {
    return volume ? fuelFlowGPH : fuelFlowPPH;
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
    return ambientVisibilityMeter;
  }

  /* Accuracy one second max limited by simulators */
  const QDateTime& getLocalTime() const
  {
    return localDateTime;
  }

  /* Accuracy one second max limited by simulators */
  const QDateTime& getZuluTime() const
  {
    return zuluDateTime;
  }

  float getTrackDegMag() const
  {
    return trackMagDeg;
  }

  float getTrackDegTrue() const
  {
    return trackTrueDeg;
  }

  /* Uses several parameters since on-ground is unreliable in the first X-Plane packets */
  bool isFlying() const
  {
    if(isHelicopter())
      return !isOnGround() && getAltitudeAboveGroundFt() > 10.f;
    else
      return !isOnGround() && getGroundSpeedKts() > 10.f && getAltitudeAboveGroundFt() > 10.f;
  }

  bool hasFuelFlow() const
  {
    return fuelFlowGPH > 0.5f || fuelFlowPPH > 1.0f;
  }

  /* Altitude as selected in autopilot */
  float getAltitudeAutopilotFt() const
  {
    return altitudeAutopilotFt;
  }

  /* Calculate the weight/volume ratio and determine if it is jet fuel
   *  weightVolRatio is 0 if quantity/weight is not sufficient */
  bool isJetfuel(float& weightVolRatio) const;

  /* Calculate flight parameters that are based on time.
   * "This" is the current aircraft state where "past" is the older one at takeoff */
  float getConsumedFuelLbs(const SimConnectUserAircraft& past) const;
  float getConsumedFuelGallons(const SimConnectUserAircraft& past) const;
  float getAverageFuelFlowPPH(const SimConnectUserAircraft& past) const;
  float getAverageFuelFlowGPH(const SimConnectUserAircraft& past) const;
  int getTravelingTimeMinutes(const SimConnectUserAircraft& past) const;

private:
  friend class atools::fs::sc::SimConnectHandler;
  friend class atools::fs::sc::SimConnectData;
  friend class xpc::XpConnect;

  float
    altitudeAboveGroundFt = 0.f, groundAltitudeFt = 0.f, altitudeAutopilotFt = 0.f,
    windSpeedKts = 0.f, windDirectionDegT = 0.f,
    ambientTemperatureCelsius = 0.f, totalAirTemperatureCelsius = 0.f,
    seaLevelPressureMbar = 0.f,
    airplaneTotalWeightLbs = 0.f,
    airplaneMaxGrossWeightLbs = 0.f, airplaneEmptyWeightLbs = 0.f, fuelTotalQuantityGallons = 0.f,
    fuelTotalWeightLbs = 0.f, fuelFlowPPH = 0.f, fuelFlowGPH = 0.f, magVarDeg = 0.f, ambientVisibilityMeter = 0.f;
  float trackMagDeg = 0.f, trackTrueDeg = 0.f;

  quint8 pitotIcePercent = 0, structuralIcePercent = 0, aoaIcePercent = 0, inletIcePercent = 0,
         propIcePercent = 0, statIcePercent = 0, windowIcePercent = 0, carbIcePercent = 0;

  QDateTime localDateTime, zuluDateTime;
};

} // namespace sc
} // namespace fs
} // namespace atools

Q_DECLARE_METATYPE(atools::fs::sc::SimConnectUserAircraft)

#endif // ATOOLS_FS_SC_SIMCONNECTUSERAIRPLANE_H
