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

#ifndef ATOOLS_AIRCRAFTPERF_H
#define ATOOLS_AIRCRAFTPERF_H

#include "fs/perf/aircraftperfconstants.h"

#include <QString>
#include <QCoreApplication>

class QSettings;
class QXmlStreamWriter;
class QXmlStreamReader;

namespace atools {

namespace util {
class XmlStream;
}

namespace fs {
namespace perf {

/* Detected file format. */
enum FileFormat
{
  FORMAT_NONE, /* Not a valid performance file */
  FORMAT_INI, /* Old INI format (<= LNM 2.4.5) */
  FORMAT_XML /* New XML format (>= LNM 2.6.X) */
};

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
  AircraftPerf();
  /* Load and save throw Exception in case of error */

  /* Load old INI or new XML format. Format is detected automatically. */
  void load(const QString& filename);

  /* Save using the new XML format (>= LNM 2.6.X) */
  void saveXml(const QString& filename) const;
  QString saveXmlStr() const;
  QByteArray saveXmlGz() const;

  /* Load XML format from file, string or Gzip compressed bytes */
  void loadXml(const QString& filename);
  void loadXmlStr(const QString& string);
  void loadXmlGz(const QByteArray& bytes);

  /* Save using the old INI format (<= LNM 2.4.5) */
  void saveIni(const QString& filename);

  static FileFormat detectFormat(const QString& filename);

  /* Set all speed, fuel flow and fuel values to 0 */
  void setNull();

  /* Reset all back to default values */
  void resetToDefault(const QString& simulatorParam);

  /* Change all fuel values in this object */
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

  /* true if all speeds are > 0 */
  bool isSpeedValid() const;

  /* Does not compare version numbers and other metadata */
  bool operator==(const AircraftPerf& other) const;

  bool operator!=(const AircraftPerf& other) const
  {
    return !operator==(other);
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
  const QString& getName() const
  {
    return name;
  }

  void setName(const QString& value)
  {
    name = value;
  }

  /* Aircraft type for this profile like "B732" or "BE9L" */
  const QString& getAircraftType() const
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

  const QString& getSimulator() const
  {
    return simulator;
  }

  void setSimulator(const QString& value)
  {
    simulator = value;
  }

  /* Current format */
  const static QLatin1String FORMAT_VERSION;

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

  static bool isAircraftTypeValid(const QString& type);

  bool isAircraftTypeValid() const
  {
    return isAircraftTypeValid(getAircraftType());
  }

  /* Fuel to volume/weight or original value depending on volume flag.
   * The input value must match the volume flag in this object. */
  float toFuelGal(float fuelGalLbs) const;
  float toFuelLbs(float fuelGalLbs) const;

  /* true if this is the example performance after creating a new file */
  bool isDefault() const;

  /* True if all fuel flow and speed values are zero by performance collection. */
  bool isNull() const;

  /* Version number to save into LNMPERF XML files */
  static const int LNMPERF_VERSION_MAJOR = 1;
  static const int LNMPERF_VERSION_MINOR = 0;

private:
  void readFromSettings(const QSettings& settings);
  void writeToSettings(QSettings& settings);
  void loadIniInternal(const QString& filename);

  void loadXmlInternal(atools::util::XmlStream& xmlStream);
  void saveXmlInternal(QXmlStreamWriter& writer) const;

  bool volume = false, jetFuel = false;

  QString name, type, simulator, description;

  QString defaultName, defaultType;

  /* Default values set in constructor give no fuel consumption, no reserve and about 3 NM per 1000 ft climb and descent */
  float taxiFuel, reserveFuel, extraFuel;
  float climbVertSpeed, climbSpeed, climbFuelFlow;
  float cruiseSpeed, cruiseFuelFlow, contingencyFuel;
  float descentSpeed, descentVertSpeed, descentFuelFlow;
  float alternateSpeed, alternateFuelFlow;
  float usableFuel, minRunwayLength;

  /* Default is soft and hard */
  atools::fs::perf::RunwayType runwayType = SOFT;
};

} // namespace perf
} // namespace fs
} // namespace atools

#endif // ATOOLS_AIRCRAFTPERF_H
