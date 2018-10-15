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

AircraftPerf::AircraftPerf()
{

}

AircraftPerf::~AircraftPerf()
{
}

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

bool AircraftPerf::operator==(const AircraftPerf& other) const
{
  return fuelUnit == other.fuelUnit &&
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
  fuelUnit = fuelUnitFromString(settings.value("Options/FuelUnit").toString());

  name = settings.value("Options/Name").toString();
  type = settings.value("Options/AircraftType").toString();
  description = settings.value("Options/Description").toString();
  programVersion = settings.value("Options/ProgramVersion").toString();
  formatVersion = settings.value("Options/FormatVersion").toString();

  taxiFuel = settings.value("Perf/TaxiFuelLbs").toInt();
  reserveFuel = settings.value("Perf/ReserveFuelLbs").toInt();
  extraFuel = settings.value("Perf/ExtraFuelLbs").toInt();
  contingencyFuel = settings.value("Perf/ContingencyFuelPercent").toInt();

  climbVertSpeed = settings.value("Perf/ClimbVertSpeedFtPerMin").toInt();
  climbSpeed = settings.value("Perf/ClimbSpeedKtsTAS").toInt();
  climbFuelFlow = settings.value("Perf/ClimbFuelFlowLbsGalPerHour").toInt();

  cruiseSpeed = settings.value("Perf/CruiseSpeedKtsTAS").toInt();
  cruiseFuelFlow = settings.value("Perf/CruiseFuelFlowLbsGalPerHour").toInt();

  descentSpeed = settings.value("Perf/DescentSpeedKtsTAS").toInt();
  descentVertSpeed = settings.value("Perf/DescentVertSpeedFtPerMin").toInt();
  descentFuelFlow = settings.value("Perf/DescentFuelFlowLbsGalPerHour").toInt();
}

void AircraftPerf::writeToSettings(QSettings& settings)
{
  settings.setValue("Options/Metadata", atools::programFileInfo());
  settings.setValue("Options/ProgramVersion", QCoreApplication::applicationVersion());
  settings.setValue("Options/FormatVersion", FORMAT_VERSION);
  settings.setValue("Options/FuelUnit", fuelUnitToString(fuelUnit));
  settings.setValue("Options/Name", name);
  settings.setValue("Options/AircraftType", type);
  settings.setValue("Options/Description", description);

  settings.setValue("Perf/TaxiFuelLbs", taxiFuel);
  settings.setValue("Perf/ReserveFuelLbs", reserveFuel);
  settings.setValue("Perf/ExtraFuelLbs", extraFuel);
  settings.setValue("Perf/ContingencyFuelPercent", contingencyFuel);

  settings.setValue("Perf/ClimbVertSpeedFtPerMin", climbVertSpeed);
  settings.setValue("Perf/ClimbSpeedKtsTAS", climbSpeed);
  settings.setValue("Perf/ClimbFuelFlowLbsGalPerHour", climbFuelFlow);

  settings.setValue("Perf/CruiseSpeedKtsTAS", cruiseSpeed);
  settings.setValue("Perf/CruiseFuelFlowLbsGalPerHour", cruiseFuelFlow);

  settings.setValue("Perf/DescentSpeedKtsTAS", descentSpeed);
  settings.setValue("Perf/DescentVertSpeedFtPerMin", descentVertSpeed);
  settings.setValue("Perf/DescentFuelFlowLbsGalPerHour", descentFuelFlow);
}

} // namespace perf
} // namespace fs
} // namespace atools
