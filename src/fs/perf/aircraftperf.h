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

#ifndef ATOOLS_AIRCRAFTPERF_H
#define ATOOLS_AIRCRAFTPERF_H

#include "fs/perf/aircraftperfconstants.h"

#include <QString>
#include <QCoreApplication>

class QSettings;

namespace atools {
namespace geo {
class LineString;
}

namespace fs {
namespace perf {

/*
 * Aircraft performance data which can be loaded and saved from or to an ini-file.
 *
 * All speeds are TAS knots, fuel is gallons/lbs and vertical speeds are feet/minute.
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

  /* Climb and descent rates to calculate profile slope */
  float getClimbRateFtPerNm() const;
  float getDescentRateFtPerNm() const;

  /* Flight path angles in degree */
  float getClimbFlightPathAngle() const;
  float getDescentFlightPathAngle() const;

  /* True if speed and vertical speed are valid */
  bool isClimbValid() const;
  bool isDescentValid() const;

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

  /* Use either gallons or lbs as fuel unit */
  bool useFuelAsVolume() const
  {
    return fuelAsVolume;
  }

  void setFuelAsVolume(bool fuelAsVol)
  {
    fuelAsVolume = fuelAsVol;
  }

  /* lbs or gallons - not part of trip fuel */
  float getTaxiFuel() const
  {
    return taxiFuel;
  }

  void setTaxiFuel(float value)
  {
    taxiFuel = value;
  }

  /* lbs or gallons - not part of trip fuel */
  float getReserveFuel() const
  {
    return reserveFuel;
  }

  void setReserveFuel(float value)
  {
    reserveFuel = value;
  }

  /* lbs or gallons - not part of trip fuel */
  float getExtraFuel() const
  {
    return extraFuel;
  }

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

private:
  void readFromSettings(const QSettings& settings);
  void writeToSettings(QSettings& settings);

  bool fuelAsVolume = false;

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
};

} // namespace perf
} // namespace fs
} // namespace atools

#endif // ATOOLS_AIRCRAFTPERF_H
