/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_AIRCRAFTPERF_H
#define ATOOLS_AIRCRAFTPERF_H

#include "fs/perf/aircraftperfconstants.h"

#include <QString>
#include <QCoreApplication>

class QSettings;

namespace atools {

namespace fs {
namespace perf {

/*
 * Aircraft performance data which can be loaded and saved from or to an ini-file.
 *
 * All speeds are TAS knots, fuel is gallons/lbs and vertical speeds are feet/minute.
 * Fuel flow is gallons/lbs per hour.
 */
class AircraftPerf
{
  Q_DECLARE_TR_FUNCTIONS(AircraftPerf)

public:
  /* Load and save throw Exception in case of error */
  void load(const QString& filepath);
  void save(const QString& filepath);

  /* Set all speed, fuel flow and fuel values to 0 */
  void setNull();

  /* Reset all back to default values */
  void resetToDefault();

  void fromGalToLbs();
  void fromLbsToGal();

  /* Climb and descent rates to calculate profile slope.
   *  Negative head wind is tailwind */
  float getClimbRateFtPerNm(float headWind) const;
  float getDescentRateFtPerNm(float headWind) const;

  /* Flight path angles in degree */
  float getClimbFlightPathAngle(float headWind) const;
  float getDescentFlightPathAngle(float headWind) const;

  /* True if speed and vertical speed are valid */
  bool isClimbValid() const;
  bool isDescentValid() const;

  /* Time to TOC and time to TOD in decimal hours. Unaffected by wind.  */
  float getTimeToClimb(float departureAltFt, float cruiseAltFt) const;
  float getTimeToDescent(float destinationAltFt, float cruiseAltFt) const;

  /* true if all fuel flow is > 0 */
  bool isFuelFlowValid() const;

  /* Does not compare version numbers and other metadata */
  bool operator==(const AircraftPerf& other) const;

  bool operator!=(const AircraftPerf& other) const
  {
    return !operator==(other);
  }

  /* Program version as saved in the file*/
  const QString& getProgramVersion() const
  {
    return programVersion;
  }

  /* File format version as loaded from the file */
  const QString& getFormatVersion() const
  {
    return formatVersion;
  }

  bool useFuelAsVolume() const
  {
    return volume;
  }

  void setFuelAsVolume(bool fuelAsVol)
  {
    volume = fuelAsVol;
  }

  /* lbs or gallons - not part of trip fuel */
  float getTaxiFuel() const
  {
    return taxiFuel;
  }

  float getTaxiFuelLbs() const;
  float getTaxiFuelGal() const;

  void setTaxiFuel(float value)
  {
    taxiFuel = value;
  }

  /* lbs or gallons - not part of trip fuel */
  float getReserveFuel() const
  {
    return reserveFuel;
  }

  float getReserveFuelLbs() const;
  float getReserveFuelGal() const;

  void setReserveFuel(float value)
  {
    reserveFuel = value;
  }

  /* lbs or gallons - not part of trip fuel */
  float getExtraFuel() const
  {
    return extraFuel;
  }

  float getExtraFuelLbs() const;
  float getExtraFuelGal() const;

  void setExtraFuel(float value)
  {
    extraFuel = value;
  }

  /* Should be added to trip fuel. Factor (1.0 - 5.0) */
  float getContingencyFuelFactor() const
  {
    return (getContingencyFuel() + 100.f) / 100.f;
  }

  /* Should be added to trip fuel. Value in percent (0 - 500) */
  float getContingencyFuel() const
  {
    return contingencyFuel;
  }

  void setContingencyFuel(float value)
  {
    contingencyFuel = value;
  }

  /* Average climb speed in feet per minute */
  float getClimbVertSpeed() const
  {
    return climbVertSpeed;
  }

  void setClimbVertSpeed(float value)
  {
    climbVertSpeed = value;
  }

  /* Speed in climb phase in knots TAS */
  float getClimbSpeed() const
  {
    return climbSpeed;
  }

  void setClimbSpeed(float value)
  {
    climbSpeed = value;
  }

  /* Average fuel flow in climb phase in gallons/lbs per hour */
  float getClimbFuelFlow() const
  {
    return climbFuelFlow;
  }

  float getClimbFuelFlowLbs() const;
  float getClimbFuelFlowGal() const;

  void setClimbFuelFlow(float value)
  {
    climbFuelFlow = value;
  }

  /* Speed in cruise phase in knots TAS */
  float getCruiseSpeed() const
  {
    return cruiseSpeed;
  }

  void setCruiseSpeed(float value)
  {
    cruiseSpeed = value;
  }

  /* Average fuel flow in cruise phase in gallons/lbs per hour */
  float getCruiseFuelFlow() const
  {
    return cruiseFuelFlow;
  }

  float getCruiseFuelFlowLbs() const;
  float getCruiseFuelFlowGal() const;

  void setCruiseFuelFlow(float value)
  {
    cruiseFuelFlow = value;
  }

  /* Speed in descent phase in knots TAS */
  float getDescentSpeed() const
  {
    return descentSpeed;
  }

  void setDescentSpeed(float value)
  {
    descentSpeed = value;
  }

  /* Average fuel flow in descent phase in gallons/lbs per hour */
  float getDescentFuelFlow() const
  {
    return descentFuelFlow;
  }

  float getDescentFuelFlowLbs() const;
  float getDescentFuelFlowGal() const;

  void setDescentFuelFlow(float value)
  {
    descentFuelFlow = value;
  }

  /* Average descent speed in feet per minute. Always positive. */
  float getDescentVertSpeed() const
  {
    return descentVertSpeed;
  }

  void setDescentVertSpeed(float value)
  {
    descentVertSpeed = value;
  }

  /* Name for this profile  */
  QString getName() const
  {
    return name;
  }

  void setName(const QString& value)
  {
    name = value;
  }

  /* Aircraft typee for this profile like "B732" or "BE9" */
  QString getAircraftType() const
  {
    return type;
  }

  void setAircraftType(const QString& value)
  {
    type = value;
  }

  /* Free text description,  comments or remarks */
  QString getDescription() const
  {
    return description;
  }

  void setDescription(const QString& value)
  {
    description = value;
  }

  /* Current format */
  const static QLatin1Literal FORMAT_VERSION;

  bool isJetFuel() const
  {
    return jetFuel;
  }

  bool isAvgas() const
  {
    return !jetFuel;
  }

  void setJetFuel(bool value)
  {
    jetFuel = value;
  }

  void setJetFuel()
  {
    jetFuel = true;
  }

  void setAvgas()
  {
    jetFuel = false;
  }

  /* Usable fuel from aircraft lbs or gallons */
  float getUsableFuel() const
  {
    return usableFuel;
  }

  float getUsableFuelLbs() const;
  float getUsableFuelGal() const;

  void setUsableFuel(float value)
  {
    usableFuel = value;
  }

  /* Speed when flying to alternate in knots TAS */
  float getAlternateSpeed() const
  {
    return alternateSpeed;
  }

  void setAlternateSpeed(float value)
  {
    alternateSpeed = value;
  }

  /* Average fuel flow in hold or flight to alternate in gallons/lbs per hour */
  float getAlternateFuelFlow() const
  {
    return alternateFuelFlow;
  }

  float getAlternateFuelFlowLbs() const;
  float getAlternateFuelFlowGal() const;

  void setAlternateFuelFlow(float value)
  {
    alternateFuelFlow = value;
  }

  /* Estimate of minimum needed runway length for landing and/or takeoff - feet */
  float getMinRunwayLength() const
  {
    return minRunwayLength;
  }

  void setMinRunwayLength(float value)
  {
    minRunwayLength = value;
  }

  /* Required runway type */
  atools::fs::perf::RunwayType getRunwayType() const
  {
    return runwayType;
  }

  void setRunwayType(int value)
  {
    runwayType = static_cast<atools::fs::perf::RunwayType>(value);
  }

  void setRunwayType(atools::fs::perf::RunwayType value)
  {
    runwayType = value;
  }

private:
  void readFromSettings(const QSettings& settings);
  void writeToSettings(QSettings& settings);

  bool volume = false, jetFuel = false;

  QString name, type, description, programVersion, formatVersion;

  /* Default values give no fuel consumption, no reserve and about 3 NM per 1000 ft climb and descent */
  float taxiFuel = 0.f;
  float reserveFuel = 0.f;
  float extraFuel = 0.f;

  float climbVertSpeed = 550.f;
  float climbSpeed = 100.f;
  float climbFuelFlow = 0.f;

  float cruiseSpeed = 100.f;
  float cruiseFuelFlow = 0.f;
  float contingencyFuel = 0.f;

  float descentSpeed = 100.f;
  float descentVertSpeed = 550.f;
  float descentFuelFlow = 0.f;

  float alternateSpeed = 100.f;
  float alternateFuelFlow = 0.f;

  float usableFuel = 0.f;
  float minRunwayLength = 0.f;

  /* Default is soft and hard */
  atools::fs::perf::RunwayType runwayType = SOFT;
};

} // namespace perf
} // namespace fs
} // namespace atools

#endif // ATOOLS_AIRCRAFTPERF_H
