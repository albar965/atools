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

const QLatin1Literal AircraftPerf::FORMAT_VERSION("1.0.0");

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

void AircraftPerf::setNull()
{
  resetToDefault();
  taxiFuel =
    reserveFuel =
      extraFuel =
        climbVertSpeed =
          climbSpeed =
            climbFuelFlow =
              cruiseSpeed =
                cruiseFuelFlow =
                  contingencyFuel =
                    descentSpeed =
                      descentVertSpeed =
                        descentFuelFlow = 0.f;
}

void AircraftPerf::resetToDefault()
{
  *this = AircraftPerf();
}

float AircraftPerf::fromGalToLbs(bool jetFuel, float value)
{
  return value *= (jetFuel ? 6.7f : 6.f);
}

float AircraftPerf::fromLbsToGal(bool jetFuel, float value)
{
  return value /= (jetFuel ? 6.7f : 6.f);
}

void AircraftPerf::fromGalToLbs()
{
  taxiFuel = fromGalToLbs(jetFuel, taxiFuel);
  reserveFuel = fromGalToLbs(jetFuel, reserveFuel);
  extraFuel = fromGalToLbs(jetFuel, extraFuel);
  climbFuelFlow = fromGalToLbs(jetFuel, climbFuelFlow);
  cruiseFuelFlow = fromGalToLbs(jetFuel, cruiseFuelFlow);
  descentFuelFlow = fromGalToLbs(jetFuel, descentFuelFlow);
}

void AircraftPerf::fromLbsToGal()
{
  taxiFuel = fromLbsToGal(jetFuel, taxiFuel);
  reserveFuel = fromLbsToGal(jetFuel, reserveFuel);
  extraFuel = fromLbsToGal(jetFuel, extraFuel);
  climbFuelFlow = fromLbsToGal(jetFuel, climbFuelFlow);
  cruiseFuelFlow = fromLbsToGal(jetFuel, cruiseFuelFlow);
  descentFuelFlow = fromLbsToGal(jetFuel, descentFuelFlow);
}

bool AircraftPerf::operator==(const AircraftPerf& other) const
{
  return fuelAsVolume == other.fuelAsVolume &&
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
         atools::almostEqual(descentFuelFlow, other.descentFuelFlow);
}

void AircraftPerf::readFromSettings(const QSettings& settings)
{
  name = settings.value("Options/Name").toString();
  type = settings.value("Options/AircraftType").toString();
  description = settings.value("Options/Description").toString();
  programVersion = settings.value("Options/ProgramVersion").toString();
  formatVersion = settings.value("Options/FormatVersion").toString();

  fuelAsVolume = settings.value("Options/FuelAsVolume").toBool();
  jetFuel = settings.value("Options/JetFuel").toBool();

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

  settings.setValue("Options/FuelAsVolume", fuelAsVolume);
  settings.setValue("Options/JetFuel", jetFuel);

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
}

} // namespace perf
} // namespace fs
} // namespace atools
