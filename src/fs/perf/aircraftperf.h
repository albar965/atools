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
  AircraftPerf();
  ~AircraftPerf();

  /* Load and save throw Exception in case of error */
  void load(const QString& filepath);
  void save(const QString& filepath);

  /* Climb and descent rates to calculate profile slope */
  float getClimbRateFtPerNm() const;
  float getDescentRateFtPerNm() const;

  /* Flight path angles in degree */
  float getClimbFlightPathAngle() const;
  float getDescentFlightPathAngle() const;

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
  atools::fs::perf::FuelUnit getFuelUnit() const
  {
    return fuelUnit;
  }

  void setFuelUnit(const atools::fs::perf::FuelUnit& value)
  {
    fuelUnit = value;
  }

  /* lbs or gallons - not part of trip fuel */
  int getTaxiFuel() const
  {
    return taxiFuel;
  }

  void setTaxiFuel(int value)
  {
    taxiFuel = value;
  }

  /* lbs or gallons - not part of trip fuel */
  int getReserveFuel() const
  {
    return reserveFuel;
  }

  void setReserveFuel(int value)
  {
    reserveFuel = value;
  }

  /* lbs or gallons - not part of trip fuel */
  int getExtraFuel() const
  {
    return extraFuel;
  }

  void setExtraFuel(int value)
  {
    extraFuel = value;
  }

  /* Should be added to trip fuel. Factor (1.0 - 5.0) */
  float getContingencyFuelFactor() const
  {
    return (getContingencyFuel() + 100.f) / 100.f;
  }

  /* Should be added to trip fuel. Value in percent (0 - 500) */
  int getContingencyFuel() const
  {
    return contingencyFuel;
  }

  void setContingencyFuel(int value)
  {
    contingencyFuel = value;
  }

  /* Average climb speed in feet per minute */
  int getClimbVertSpeed() const
  {
    return climbVertSpeed;
  }

  void setClimbVertSpeed(int value)
  {
    climbVertSpeed = value;
  }

  /* Speed in climb phase in knots TAS */
  int getClimbSpeed() const
  {
    return climbSpeed;
  }

  void setClimbSpeed(int value)
  {
    climbSpeed = value;
  }

  /* Average fuel flow in climb phase in gallons/lbs per hour */
  int getClimbFuelFlow() const
  {
    return climbFuelFlow;
  }

  void setClimbFuelFlow(int value)
  {
    climbFuelFlow = value;
  }

  /* Speed in cruise phase in knots TAS */
  int getCruiseSpeed() const
  {
    return cruiseSpeed;
  }

  void setCruiseSpeed(int value)
  {
    cruiseSpeed = value;
  }

  /* Average fuel flow in cruise phase in gallons/lbs per hour */
  int getCruiseFuelFlow() const
  {
    return cruiseFuelFlow;
  }

  void setCruiseFuelFlow(int value)
  {
    cruiseFuelFlow = value;
  }

  /* Speed in descent phase in knots TAS */
  int getDescentSpeed() const
  {
    return descentSpeed;
  }

  void setDescentSpeed(int value)
  {
    descentSpeed = value;
  }

  /* Average fuel flow in descent phase in gallons/lbs per hour */
  int getDescentFuelFlow() const
  {
    return descentFuelFlow;
  }

  void setDescentFuelFlow(int value)
  {
    descentFuelFlow = value;
  }

  /* Average descent speed in feet per minute */
  int getDescentVertSpeed() const
  {
    return descentVertSpeed;
  }

  void setDescentVertSpeed(int value)
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

  atools::fs::perf::FuelUnit fuelUnit = WEIGHT;

  QString name, type, description, programVersion, formatVersion;

  int taxiFuel = 0;
  int reserveFuel = 0;
  int extraFuel = 0;

  int climbVertSpeed = 1500;
  int climbSpeed = 100;
  int climbFuelFlow = 0;

  int cruiseSpeed = 100;
  int cruiseFuelFlow = 0;
  int contingencyFuel = 0;

  int descentSpeed = 100;
  int descentVertSpeed = 1500;
  int descentFuelFlow = 0;
};

} // namespace perf
} // namespace fs
} // namespace atools

#endif // ATOOLS_AIRCRAFTPERF_H
