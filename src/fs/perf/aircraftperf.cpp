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

#include "fs/perf/aircraftperf.h"
#include "atools.h"
#include "exception.h"
#include "geo/calculations.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QSettings>

namespace atools {
namespace fs {
namespace perf {

const QLatin1Literal AircraftPerf::FORMAT_VERSION("2.4.0");

float AircraftPerf::getClimbRateFtPerNm() const
{
  return getClimbVertSpeed() * 60.f / getClimbSpeed();
}

float AircraftPerf::getDescentRateFtPerNm() const
{
  return getDescentVertSpeed() * 60.f / getDescentSpeed();
}

float AircraftPerf::getClimbFlightPathAngle() const
{
  return static_cast<float>(atools::geo::toDegree(std::atan(atools::geo::feetToNm(getClimbRateFtPerNm()))));
}

float AircraftPerf::getDescentFlightPathAngle() const
{
  return static_cast<float>(atools::geo::toDegree(std::atan(atools::geo::feetToNm(getDescentRateFtPerNm()))));
}

bool AircraftPerf::isClimbValid() const
{
  return climbSpeed > 10.f && climbSpeed < INVALID_PERF_VALUE &&
         climbVertSpeed > 10.f && climbVertSpeed < INVALID_PERF_VALUE;
}

bool AircraftPerf::isDescentValid() const
{
  return descentSpeed > 10.f && descentSpeed < INVALID_PERF_VALUE &&
         descentVertSpeed > 10.f && descentVertSpeed < INVALID_PERF_VALUE;
}

bool AircraftPerf::isFuelFlowValid() const
{
  return climbFuelFlow > 0.f && cruiseFuelFlow > 0.1 && descentFuelFlow > 0.1;
}

void AircraftPerf::load(const QString& filepath)
{
  QFileInfo fi(filepath);
  if(!fi.exists() || !fi.isReadable())
    throw atools::Exception(tr("Cannot open aircraft performance file \"%1\" for reading.").arg(filepath));

  QSettings settings(filepath, QSettings::IniFormat);
  settings.setIniCodec("UTF-8");
  readFromSettings(settings);

  if(settings.status() != QSettings::NoError)
    throw atools::Exception(tr("Cannot open aircraft performance file \"%1\" for reading.").arg(filepath));
}

void AircraftPerf::save(const QString& filepath)
{
  QSettings settings(filepath, QSettings::IniFormat);
  settings.setIniCodec("UTF-8");
  writeToSettings(settings);
  settings.sync();

  if(settings.status() != QSettings::NoError)
    throw atools::Exception(tr("Cannot open aircraft performance file \"%1\" for writing.").arg(filepath));
}

void AircraftPerf::resetToDefault()
{
  *this = AircraftPerf();
}

void AircraftPerf::setNull()
{
  resetToDefault();
  taxiFuel = reserveFuel = extraFuel =
    contingencyFuel =
      climbVertSpeed = climbSpeed = climbFuelFlow =
        cruiseSpeed = cruiseFuelFlow =
          descentSpeed = descentVertSpeed = descentFuelFlow =
            alternateSpeed = alternateFuelFlow =
              usableFuel =
                minRunwayLength =
                  0.f;
  runwayType = SOFT;
}

void AircraftPerf::fromGalToLbs()
{
  using atools::geo::fromGalToLbs;

  usableFuel = fromGalToLbs(jetFuel, usableFuel);
  taxiFuel = fromGalToLbs(jetFuel, taxiFuel);
  reserveFuel = fromGalToLbs(jetFuel, reserveFuel);
  extraFuel = fromGalToLbs(jetFuel, extraFuel);
  climbFuelFlow = fromGalToLbs(jetFuel, climbFuelFlow);
  cruiseFuelFlow = fromGalToLbs(jetFuel, cruiseFuelFlow);
  descentFuelFlow = fromGalToLbs(jetFuel, descentFuelFlow);
  alternateFuelFlow = fromGalToLbs(jetFuel, alternateFuelFlow);
}

void AircraftPerf::fromLbsToGal()
{
  using atools::geo::fromLbsToGal;

  usableFuel = fromLbsToGal(jetFuel, usableFuel);
  taxiFuel = fromLbsToGal(jetFuel, taxiFuel);
  reserveFuel = fromLbsToGal(jetFuel, reserveFuel);
  extraFuel = fromLbsToGal(jetFuel, extraFuel);
  climbFuelFlow = fromLbsToGal(jetFuel, climbFuelFlow);
  cruiseFuelFlow = fromLbsToGal(jetFuel, cruiseFuelFlow);
  descentFuelFlow = fromLbsToGal(jetFuel, descentFuelFlow);
  alternateFuelFlow = fromLbsToGal(jetFuel, alternateFuelFlow);
}

bool AircraftPerf::operator==(const AircraftPerf& other) const
{
  return volume == other.volume &&
         name == other.name &&
         type == other.type &&
         description == other.description &&
         atools::almostEqual(taxiFuel, other.taxiFuel) &&
         atools::almostEqual(reserveFuel, other.reserveFuel) &&
         atools::almostEqual(extraFuel, other.extraFuel) &&
         atools::almostEqual(climbVertSpeed, other.climbVertSpeed) &&
         atools::almostEqual(climbSpeed, other.climbSpeed) &&
         atools::almostEqual(climbFuelFlow, other.climbFuelFlow) &&
         atools::almostEqual(cruiseSpeed, other.cruiseSpeed) &&
         atools::almostEqual(cruiseFuelFlow, other.cruiseFuelFlow) &&
         atools::almostEqual(contingencyFuel, other.contingencyFuel) &&
         atools::almostEqual(descentSpeed, other.descentSpeed) &&
         atools::almostEqual(descentVertSpeed, other.descentVertSpeed) &&
         atools::almostEqual(descentFuelFlow, other.descentFuelFlow) &&
         atools::almostEqual(alternateSpeed, other.alternateSpeed) &&
         atools::almostEqual(alternateFuelFlow, other.alternateFuelFlow) &&
         atools::almostEqual(usableFuel, other.usableFuel) &&
         atools::almostEqual(minRunwayLength, other.minRunwayLength) &&
         runwayType == other.runwayType;

}

float AircraftPerf::getTaxiFuelLbs() const
{
  return volume ? atools::geo::fromGalToLbs(jetFuel, taxiFuel) : taxiFuel;
}

float AircraftPerf::getTaxiFuelGal() const
{
  return volume ? taxiFuel : atools::geo::fromLbsToGal(jetFuel, taxiFuel);
}

float AircraftPerf::getReserveFuelLbs() const
{
  return volume ? atools::geo::fromGalToLbs(jetFuel, reserveFuel) : reserveFuel;
}

float AircraftPerf::getReserveFuelGal() const
{
  return volume ? reserveFuel : atools::geo::fromLbsToGal(jetFuel, reserveFuel);
}

float AircraftPerf::getExtraFuelLbs() const
{
  return volume ? atools::geo::fromGalToLbs(jetFuel, extraFuel) : extraFuel;
}

float AircraftPerf::getExtraFuelGal() const
{
  return volume ? extraFuel : atools::geo::fromLbsToGal(jetFuel, extraFuel);
}

float AircraftPerf::getClimbFuelFlowLbs() const
{
  return volume ? atools::geo::fromGalToLbs(jetFuel, climbFuelFlow) : climbFuelFlow;
}

float AircraftPerf::getClimbFuelFlowGal() const
{
  return volume ? climbFuelFlow : atools::geo::fromLbsToGal(jetFuel, climbFuelFlow);
}

float AircraftPerf::getCruiseFuelFlowLbs() const
{
  return volume ? atools::geo::fromGalToLbs(jetFuel, cruiseFuelFlow) : cruiseFuelFlow;
}

float AircraftPerf::getCruiseFuelFlowGal() const
{
  return volume ? cruiseFuelFlow : atools::geo::fromLbsToGal(jetFuel, cruiseFuelFlow);
}

float AircraftPerf::getDescentFuelFlowLbs() const
{
  return volume ? atools::geo::fromGalToLbs(jetFuel, descentFuelFlow) : descentFuelFlow;
}

float AircraftPerf::getDescentFuelFlowGal() const
{
  return volume ? descentFuelFlow : atools::geo::fromLbsToGal(jetFuel, descentFuelFlow);
}

float AircraftPerf::getUsableFuelLbs() const
{
  return volume ? atools::geo::fromGalToLbs(jetFuel, usableFuel) : usableFuel;
}

float AircraftPerf::getUsableFuelGal() const
{
  return volume ? usableFuel : atools::geo::fromLbsToGal(jetFuel, usableFuel);
}

float AircraftPerf::getAlternateFuelFlowLbs() const
{
  return volume ? atools::geo::fromGalToLbs(jetFuel, alternateFuelFlow) : alternateFuelFlow;
}

float AircraftPerf::getAlternateFuelFlowGal() const
{
  return volume ? alternateFuelFlow : atools::geo::fromLbsToGal(jetFuel, alternateFuelFlow);
}

void AircraftPerf::readFromSettings(const QSettings& settings)
{
  name = settings.value("Options/Name").toString();
  type = settings.value("Options/AircraftType").toString();
  description = settings.value("Options/Description").toString();
  programVersion = settings.value("Options/ProgramVersion").toString();
  formatVersion = settings.value("Options/FormatVersion").toString();

  volume = settings.value("Options/FuelAsVolume").toBool();
  jetFuel = settings.value("Options/JetFuel").toBool();

  usableFuel = settings.value("Perf/UsableFuel").toFloat();

  if(settings.contains("Perf/TaxiFuelLbs"))
    taxiFuel = settings.value("Perf/TaxiFuelLbs").toFloat();
  if(settings.contains("Perf/TaxiFuelLbsGal"))
    taxiFuel = settings.value("Perf/TaxiFuelLbsGal").toFloat();

  if(settings.contains("Perf/ReserveFuelLbs"))
    reserveFuel = settings.value("Perf/ReserveFuelLbs").toFloat();
  if(settings.contains("Perf/ReserveFuelLbsGal"))
    reserveFuel = settings.value("Perf/ReserveFuelLbsGal").toFloat();

  if(settings.contains("Perf/ExtraFuelLbs"))
    extraFuel = settings.value("Perf/ExtraFuelLbs").toFloat();
  if(settings.contains("Perf/ExtraFuelLbsGal"))
    extraFuel = settings.value("Perf/ExtraFuelLbsGal").toFloat();

  contingencyFuel = settings.value("Perf/ContingencyFuelPercent").toFloat();

  climbVertSpeed = settings.value("Perf/ClimbVertSpeedFtPerMin").toFloat();
  climbSpeed = settings.value("Perf/ClimbSpeedKtsTAS").toFloat();
  climbFuelFlow = settings.value("Perf/ClimbFuelFlowLbsGalPerHour").toFloat();

  cruiseSpeed = settings.value("Perf/CruiseSpeedKtsTAS").toFloat();
  cruiseFuelFlow = settings.value("Perf/CruiseFuelFlowLbsGalPerHour").toFloat();

  descentSpeed = settings.value("Perf/DescentSpeedKtsTAS").toFloat();
  descentVertSpeed = settings.value("Perf/DescentVertSpeedFtPerMin").toFloat();
  descentFuelFlow = settings.value("Perf/DescentFuelFlowLbsGalPerHour").toFloat();

  alternateSpeed = settings.value("Perf/AlternateSpeed", cruiseSpeed).toFloat();
  alternateFuelFlow = settings.value("Perf/AlternateFuelFlow", cruiseFuelFlow).toFloat();

  minRunwayLength = settings.value("Perf/MinRunwayLength").toFloat();
  runwayType = runwayTypeFromStr(settings.value("Perf/RunwayType", "Soft").toString());
}

void AircraftPerf::writeToSettings(QSettings& settings)
{
  settings.remove("Perf/TaxiFuelLbs");
  settings.remove("Perf/ReserveFuelLbs");
  settings.remove("Perf/ExtraFuelLbs");

  settings.setValue("Options/Metadata", atools::programFileInfo());
  settings.setValue("Options/ProgramVersion", QCoreApplication::applicationVersion());
  settings.setValue("Options/FormatVersion", FORMAT_VERSION);
  settings.setValue("Options/Name", name);
  settings.setValue("Options/AircraftType", type);
  settings.setValue("Options/Description", description);

  settings.setValue("Options/FuelAsVolume", volume);
  settings.setValue("Options/JetFuel", jetFuel);

  settings.setValue("Perf/UsableFuel", QString::number(usableFuel));
  settings.setValue("Perf/TaxiFuelLbsGal", QString::number(taxiFuel));
  settings.setValue("Perf/ReserveFuelLbsGal", QString::number(reserveFuel));
  settings.setValue("Perf/ExtraFuelLbsGal", QString::number(extraFuel));
  settings.setValue("Perf/ContingencyFuelPercent", QString::number(contingencyFuel));

  settings.setValue("Perf/ClimbVertSpeedFtPerMin", QString::number(climbVertSpeed));
  settings.setValue("Perf/ClimbSpeedKtsTAS", QString::number(climbSpeed));
  settings.setValue("Perf/ClimbFuelFlowLbsGalPerHour", QString::number(climbFuelFlow));

  settings.setValue("Perf/CruiseSpeedKtsTAS", QString::number(cruiseSpeed));
  settings.setValue("Perf/CruiseFuelFlowLbsGalPerHour", QString::number(cruiseFuelFlow));

  settings.setValue("Perf/DescentSpeedKtsTAS", QString::number(descentSpeed));
  settings.setValue("Perf/DescentVertSpeedFtPerMin", QString::number(descentVertSpeed));
  settings.setValue("Perf/DescentFuelFlowLbsGalPerHour", QString::number(descentFuelFlow));

  settings.setValue("Perf/AlternateSpeed", QString::number(alternateSpeed));
  settings.setValue("Perf/AlternateFuelFlow", QString::number(alternateFuelFlow));

  settings.setValue("Perf/MinRunwayLength", QString::number(minRunwayLength));
  settings.setValue("Perf/RunwayType", runwayTypeToStr(runwayType));
}

} // namespace perf
} // namespace fs
} // namespace atools
