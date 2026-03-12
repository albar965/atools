/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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
#include "util/xmlstreamreader.h"
#include "zip/gzip.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSettings>
#include <QXmlStreamReader>

namespace ageo = atools::geo;

namespace atools {
namespace fs {
namespace perf {

const QLatin1String AircraftPerf::FORMAT_VERSION("2.4.0");

float AircraftPerf::getTimeToClimb(float departureAltFt, float cruiseAltFt) const
{
  return (cruiseAltFt - departureAltFt) / getClimbVertSpeed() / 60.f;
}

float AircraftPerf::getTimeToDescent(float destinationAltFt, float cruiseAltFt) const
{
  return (cruiseAltFt - destinationAltFt) / getDescentVertSpeed() / 60.f;
}

float AircraftPerf::getClimbRateFtPerNm(float headWind) const
{
  return getClimbVertSpeed() * 60.f / (getClimbSpeed() - headWind);
}

float AircraftPerf::getDescentRateFtPerNm(float headWind) const
{
  return getDescentVertSpeed() * 60.f / (getDescentSpeed() - headWind);
}

float AircraftPerf::getClimbFlightPathAngle(float headWind) const
{
  return static_cast<float>(ageo::atanDeg(ageo::feetToNm(getClimbRateFtPerNm(headWind))));
}

float AircraftPerf::getDescentFlightPathAngle(float headWind) const
{
  return static_cast<float>(ageo::atanDeg(ageo::feetToNm(getDescentRateFtPerNm(headWind))));
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
  return climbFuelFlow > 0.1f && cruiseFuelFlow > 0.1f && descentFuelFlow > 0.1f;
}

bool AircraftPerf::isSpeedValid() const
{
  return climbSpeed > 1.0f && cruiseSpeed > 1.0f && descentSpeed > 1.0f;
}

AircraftPerf::AircraftPerf()
{
  defaultName = tr("Example Performance Profile");
  defaultType = tr("C172");

  name = defaultName;
  type = defaultType;

  volume = false;
  jetFuel = false;

  // Default values for C172 give no fuel consumption, no reserve and about 3 NM per 1000 ft climb and descent
  taxiFuel = 0.f;
  reserveFuel = 60.f;
  extraFuel = 0.f;

  climbVertSpeed = 550.f;
  climbSpeed = 100.f;
  climbFuelFlow = 80.f;

  cruiseSpeed = 120.f;
  cruiseFuelFlow = 60.f;
  contingencyFuel = 0.f;

  descentSpeed = 100.f;
  descentVertSpeed = 550.f;
  descentFuelFlow = 40.f;

  alternateSpeed = 100.f;
  alternateFuelFlow = 60.f;

  usableFuel = 260.f;
  minRunwayLength = 0.f;
}

void AircraftPerf::load(const QString& filename)
{
  QFileInfo fi(filename);
  if(!fi.exists() || !fi.isReadable())
    throw atools::Exception(tr("Cannot open aircraft performance file \"%1\" for reading.").arg(filename));

  switch(detectFormat(filename))
  {
    case atools::fs::perf::FORMAT_INI:
      loadIniInternal(filename);
      break;

    case atools::fs::perf::FORMAT_XML:
      loadXml(filename);
      break;

    case atools::fs::perf::FORMAT_NONE:
      throw atools::Exception(tr("Cannot open aircraft performance file \"%1\" for reading. Invalid format.").
                              arg(filename));
  }
}

QString AircraftPerf::saveXmlStr() const
{
  QString retval;
  QXmlStreamWriter writer(&retval);
  saveXmlInternal(writer);
  return retval;
}

QByteArray AircraftPerf::saveXmlGz() const
{
  return atools::zip::gzipCompress(saveXmlStr().toUtf8());
}

void AircraftPerf::saveXml(const QString& filename) const
{
  QFile xmlFile(filename);
  if(xmlFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QXmlStreamWriter writer(&xmlFile);
    saveXmlInternal(writer);
    xmlFile.close();
  }
  else
    throw Exception(tr("Cannot open file %1. Reason: %2").arg(filename).arg(xmlFile.errorString()));
}

void AircraftPerf::saveXmlInternal(QXmlStreamWriter& writer) const
{
  writer.setAutoFormatting(true);
  writer.setAutoFormattingIndent(2);

  writer.writeStartDocument(QStringLiteral("1.0"));
  writer.writeStartElement(QStringLiteral("LittleNavmap"));

  // Schema namespace and reference to XSD ======================
  writer.writeAttribute(QStringLiteral("xmlns:xsi"), QStringLiteral("http://www.w3.org/2001/XMLSchema-instance"));
  writer.writeAttribute(QStringLiteral("xsi:noNamespaceSchemaLocation"), QStringLiteral("https://www.littlenavmap.org/schema/lnmperf.xsd"));

  writer.writeStartElement(QStringLiteral("AircraftPerf"));

  // Save header and metadata =======================================================
  writer.writeStartElement(QStringLiteral("Header"));
  writer.writeTextElement(QStringLiteral("CreationDate"), atools::currentIsoWithOffset(false /* milliseconds */));
  writer.writeTextElement(QStringLiteral("FileVersion"), QStringLiteral("%1.%2").arg(LNMPERF_VERSION_MAJOR).arg(LNMPERF_VERSION_MINOR));
  writer.writeTextElement(QStringLiteral("ProgramName"), QCoreApplication::applicationName());
  writer.writeTextElement(QStringLiteral("ProgramVersion"), QCoreApplication::applicationVersion());
  writer.writeTextElement(QStringLiteral("Documentation"), QStringLiteral("https://www.littlenavmap.org/lnmperf.html"));
  writer.writeEndElement(); // Header

  // Options =======================================================
  writer.writeStartElement(QStringLiteral("Options"));
  writer.writeTextElement(QStringLiteral("Name"), name);
  writer.writeTextElement(QStringLiteral("AircraftType"), type);
  writer.writeTextElement(QStringLiteral("Simulator"), simulator);
  writer.writeTextElement(QStringLiteral("Description"), description);
  writer.writeTextElement(QStringLiteral("FuelAsVolume"), QString::number(volume));
  writer.writeTextElement(QStringLiteral("JetFuel"), QString::number(jetFuel));
  writer.writeEndElement(); // Options

  // Performance =======================================================
  writer.writeStartElement(QStringLiteral("Perf"));

  // Performance general ============================
  writer.writeTextElement(QStringLiteral("ContingencyFuelPercent"), QString::number(contingencyFuel, 'f', 1));
  writer.writeTextElement(QStringLiteral("ExtraFuelLbsGal"), QString::number(extraFuel, 'f', 3));
  writer.writeTextElement(QStringLiteral("MinRunwayLengthFt"), QString::number(minRunwayLength, 'f', 3));
  writer.writeTextElement(QStringLiteral("ReserveFuelLbsGal"), QString::number(reserveFuel, 'f', 3));
  writer.writeTextElement(QStringLiteral("RunwayType"), runwayTypeToStr(runwayType));
  writer.writeTextElement(QStringLiteral("TaxiFuelLbsGal"), QString::number(taxiFuel, 'f', 3));
  writer.writeTextElement(QStringLiteral("UsableFuelLbsGal"), QString::number(usableFuel, 'f', 3));

  // Performance alternate ============================
  writer.writeStartElement(QStringLiteral("Alternate"));
  writer.writeTextElement(QStringLiteral("FuelFlowLbsGalPerHour"), QString::number(alternateFuelFlow, 'f', 3));
  writer.writeTextElement(QStringLiteral("SpeedKtsTAS"), QString::number(alternateSpeed, 'f', 3));
  writer.writeEndElement(); // Alternate

  // Performance climb ============================
  writer.writeStartElement(QStringLiteral("Climb"));
  writer.writeTextElement(QStringLiteral("FuelFlowLbsGalPerHour"), QString::number(climbFuelFlow, 'f', 3));
  writer.writeTextElement(QStringLiteral("SpeedKtsTAS"), QString::number(climbSpeed, 'f', 3));
  writer.writeTextElement(QStringLiteral("VertSpeedFtPerMin"), QString::number(climbVertSpeed, 'f', 3));
  writer.writeEndElement(); // Climb

  // Performance cruise ============================
  writer.writeStartElement(QStringLiteral("Cruise"));
  writer.writeTextElement(QStringLiteral("FuelFlowLbsGalPerHour"), QString::number(cruiseFuelFlow, 'f', 3));
  writer.writeTextElement(QStringLiteral("SpeedKtsTAS"), QString::number(cruiseSpeed, 'f', 3));
  writer.writeEndElement(); // Cruise

  // Performance descent ============================
  writer.writeStartElement(QStringLiteral("Descent"));
  writer.writeTextElement(QStringLiteral("FuelFlowLbsGalPerHour"), QString::number(descentFuelFlow, 'f', 3));
  writer.writeTextElement(QStringLiteral("SpeedKtsTAS"), QString::number(descentSpeed, 'f', 3));
  writer.writeTextElement(QStringLiteral("VertSpeedFtPerMin"), QString::number(descentVertSpeed, 'f', 3));
  writer.writeEndElement(); // Descent

  writer.writeEndElement(); // Perf
  writer.writeEndElement(); // AircraftPerf
  writer.writeEndElement(); // LittleNavmap
  writer.writeEndDocument();
}

void AircraftPerf::loadXml(const QString& filename)
{
  QFile xmlFile(filename);

  if(xmlFile.open(QIODevice::ReadOnly))
  {
    atools::util::XmlStreamReader xmlStream(&xmlFile, filename);
    loadXmlInternal(xmlStream);
    xmlFile.close();
  }
  else
    throw Exception(tr("Cannot open file \"%1\". Reason: %2").arg(filename).arg(xmlFile.errorString()));
}

void AircraftPerf::loadXmlStr(const QString& string)
{
  atools::util::XmlStreamReader xmlStream(string);
  loadXmlInternal(xmlStream);
}

void AircraftPerf::loadXmlGz(const QByteArray& bytes)
{
  loadXmlStr(QString(atools::zip::gzipDecompress(bytes)));
}

void AircraftPerf::loadXmlInternal(atools::util::XmlStreamReader& xmlStream)
{
  xmlStream.readUntilElement(QStringLiteral("LittleNavmap"));
  xmlStream.readUntilElement(QStringLiteral("AircraftPerf"));

  while(xmlStream.readNextStartElement())
  {
    // Read data from header =========================================
    if(xmlStream.name() == QStringLiteral("Header"))
    {
      // Skip header without warning
      xmlStream.skipCurrentElement();
      continue;
    }
    else if(xmlStream.name() == QStringLiteral("Options"))
    {
      while(xmlStream.readNextStartElement())
      {
        if(xmlStream.name() == QStringLiteral("Name"))
          name = xmlStream.readElementTextStr();
        else if(xmlStream.name() == QStringLiteral("AircraftType"))
          type = xmlStream.readElementTextStr();
        else if(xmlStream.name() == QStringLiteral("Simulator"))
          simulator = xmlStream.readElementTextStr();
        else if(xmlStream.name() == QStringLiteral("Description"))
          description = xmlStream.readElementTextStr();
        else if(xmlStream.name() == QStringLiteral("FuelAsVolume"))
          volume = xmlStream.readElementTextStr().toInt();
        else if(xmlStream.name() == QStringLiteral("JetFuel"))
          jetFuel = xmlStream.readElementTextStr().toInt();
        else
          xmlStream.skipCurrentElement(true /* warn */);
      }
    }
    else if(xmlStream.name() == QStringLiteral("Perf"))
    {
      // Performance =======================================================
      while(xmlStream.readNextStartElement())
      {
        if(xmlStream.name() == QStringLiteral("Alternate"))
        {
          while(xmlStream.readNextStartElement())
          {
            if(xmlStream.name() == QStringLiteral("FuelFlowLbsGalPerHour"))
              alternateFuelFlow = xmlStream.readElementTextFloat();
            else if(xmlStream.name() == QStringLiteral("SpeedKtsTAS"))
              alternateSpeed = xmlStream.readElementTextFloat();
            else
              xmlStream.skipCurrentElement(true /* warn */);
          }
        }
        else if(xmlStream.name() == QStringLiteral("Climb"))
        {
          while(xmlStream.readNextStartElement())
          {
            if(xmlStream.name() == QStringLiteral("FuelFlowLbsGalPerHour"))
              climbFuelFlow = xmlStream.readElementTextFloat();
            else if(xmlStream.name() == QStringLiteral("SpeedKtsTAS"))
              climbSpeed = xmlStream.readElementTextFloat();
            else if(xmlStream.name() == QStringLiteral("VertSpeedFtPerMin"))
              climbVertSpeed = xmlStream.readElementTextFloat();
            else
              xmlStream.skipCurrentElement(true /* warn */);
          }
        }
        else if(xmlStream.name() == QStringLiteral("Cruise"))
        {
          while(xmlStream.readNextStartElement())
          {
            if(xmlStream.name() == QStringLiteral("FuelFlowLbsGalPerHour"))
              cruiseFuelFlow = xmlStream.readElementTextFloat();
            else if(xmlStream.name() == QStringLiteral("SpeedKtsTAS"))
              cruiseSpeed = xmlStream.readElementTextFloat();
            else
              xmlStream.skipCurrentElement(true /* warn */);
          }
        }
        else if(xmlStream.name() == QStringLiteral("Descent"))
        {
          while(xmlStream.readNextStartElement())
          {
            if(xmlStream.name() == QStringLiteral("FuelFlowLbsGalPerHour"))
              descentFuelFlow = xmlStream.readElementTextFloat();
            else if(xmlStream.name() == QStringLiteral("SpeedKtsTAS"))
              descentSpeed = xmlStream.readElementTextFloat();
            else if(xmlStream.name() == QStringLiteral("VertSpeedFtPerMin"))
              descentVertSpeed = xmlStream.readElementTextFloat();
            else
              xmlStream.skipCurrentElement(true /* warn */);
          }
        }
        // Performance general ============================
        else if(xmlStream.name() == QStringLiteral("ContingencyFuelPercent"))
          contingencyFuel = xmlStream.readElementTextFloat();
        else if(xmlStream.name() == QStringLiteral("ExtraFuelLbsGal"))
          extraFuel = xmlStream.readElementTextFloat();
        else if(xmlStream.name() == QStringLiteral("MinRunwayLengthFt"))
          minRunwayLength = xmlStream.readElementTextFloat();
        else if(xmlStream.name() == QStringLiteral("ReserveFuelLbsGal"))
          reserveFuel = xmlStream.readElementTextFloat();
        else if(xmlStream.name() == QStringLiteral("RunwayType"))
          runwayType = runwayTypeFromStr(xmlStream.readElementTextStr());
        else if(xmlStream.name() == QStringLiteral("TaxiFuelLbsGal"))
          taxiFuel = xmlStream.readElementTextFloat();
        else if(xmlStream.name() == QStringLiteral("UsableFuelLbsGal"))
          usableFuel = xmlStream.readElementTextFloat();
        else
          xmlStream.skipCurrentElement(true /* warn */);
      }
    }
  }
}

void AircraftPerf::loadIniInternal(const QString& filename)
{
  QSettings settings(filename, QSettings::IniFormat);
  readFromSettings(settings);

  if(settings.status() != QSettings::NoError)
    throw atools::Exception(tr("Cannot open aircraft performance file \"%1\" for reading.").arg(filename));
}

void AircraftPerf::saveIni(const QString& filename)
{
  if(QFile::exists(filename))
    QFile::remove(filename);

  QSettings settings(filename, QSettings::IniFormat);
  writeToSettings(settings);
  settings.sync();

  if(settings.status() != QSettings::NoError)
    throw atools::Exception(tr("Cannot open aircraft performance file \"%1\" for writing.").arg(filename));
}

FileFormat AircraftPerf::detectFormat(const QString& filename)
{
  const QStringList probe = atools::probeFile(filename, 30);

  // Assume LNM XML files to be formatted
  if(probe.at(0).startsWith(QStringLiteral("<?xml version")) &&
     probe.at(1).startsWith(QStringLiteral("<littlenavmap")) &&
     probe.at(2).startsWith(QStringLiteral("<aircraftperf>")))
    return FORMAT_XML;
  else if(probe.contains(QStringLiteral("[options]")) && probe.contains(QStringLiteral("[perf]")))
    return FORMAT_INI;
  else
    return FORMAT_NONE;
}

void AircraftPerf::resetToDefault(const QString& simulatorParam)
{
  *this = AircraftPerf();
  simulator = simulatorParam;
}

void AircraftPerf::setNull()
{
  resetToDefault(QStringLiteral());
  taxiFuel = reserveFuel = extraFuel =
    contingencyFuel =
      climbVertSpeed = climbSpeed = climbFuelFlow =
        cruiseSpeed = cruiseFuelFlow =
          descentSpeed = descentVertSpeed = descentFuelFlow =
            alternateSpeed = alternateFuelFlow = usableFuel = minRunwayLength = 0.f;
  runwayType = SOFT;
  name.clear();
  type.clear();
}

void AircraftPerf::fromGalToLbs()
{
  using ageo::fromGalToLbs;
  volume = false;

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
  using ageo::fromLbsToGal;
  volume = true;

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
         simulator == other.simulator &&
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
  return volume ? ageo::fromGalToLbs(jetFuel, taxiFuel) : taxiFuel;
}

float AircraftPerf::getTaxiFuelGal() const
{
  return volume ? taxiFuel : ageo::fromLbsToGal(jetFuel, taxiFuel);
}

float AircraftPerf::getReserveFuelLbs() const
{
  return volume ? ageo::fromGalToLbs(jetFuel, reserveFuel) : reserveFuel;
}

float AircraftPerf::getReserveFuelGal() const
{
  return volume ? reserveFuel : ageo::fromLbsToGal(jetFuel, reserveFuel);
}

float AircraftPerf::getExtraFuelLbs() const
{
  return volume ? ageo::fromGalToLbs(jetFuel, extraFuel) : extraFuel;
}

float AircraftPerf::getExtraFuelGal() const
{
  return volume ? extraFuel : ageo::fromLbsToGal(jetFuel, extraFuel);
}

float AircraftPerf::getClimbFuelFlowLbs() const
{
  return volume ? ageo::fromGalToLbs(jetFuel, climbFuelFlow) : climbFuelFlow;
}

float AircraftPerf::getClimbFuelFlowGal() const
{
  return volume ? climbFuelFlow : ageo::fromLbsToGal(jetFuel, climbFuelFlow);
}

float AircraftPerf::getCruiseFuelFlowLbs() const
{
  return volume ? ageo::fromGalToLbs(jetFuel, cruiseFuelFlow) : cruiseFuelFlow;
}

float AircraftPerf::getCruiseFuelFlowGal() const
{
  return volume ? cruiseFuelFlow : ageo::fromLbsToGal(jetFuel, cruiseFuelFlow);
}

float AircraftPerf::getDescentFuelFlowLbs() const
{
  return volume ? ageo::fromGalToLbs(jetFuel, descentFuelFlow) : descentFuelFlow;
}

float AircraftPerf::getDescentFuelFlowGal() const
{
  return volume ? descentFuelFlow : ageo::fromLbsToGal(jetFuel, descentFuelFlow);
}

float AircraftPerf::getUsableFuelLbs() const
{
  return volume ? ageo::fromGalToLbs(jetFuel, usableFuel) : usableFuel;
}

float AircraftPerf::getUsableFuelGal() const
{
  return volume ? usableFuel : ageo::fromLbsToGal(jetFuel, usableFuel);
}

float AircraftPerf::getAlternateFuelFlowLbs() const
{
  return volume ? ageo::fromGalToLbs(jetFuel, alternateFuelFlow) : alternateFuelFlow;
}

float AircraftPerf::getAlternateFuelFlowGal() const
{
  return volume ? alternateFuelFlow : ageo::fromLbsToGal(jetFuel, alternateFuelFlow);
}

float AircraftPerf::toFuelLbs(float fuelGalLbs) const
{
  // Convert to lbs if this perf is volume based
  return volume ? ageo::fromGalToLbs(jetFuel, fuelGalLbs) : fuelGalLbs;
}

bool AircraftPerf::isDefault() const
{
  return name == defaultName && type == defaultType;
}

bool AircraftPerf::isNull() const
{
  return atools::almostEqual(climbVertSpeed, 0.f) &&
         atools::almostEqual(climbSpeed, 0.f) &&
         atools::almostEqual(climbFuelFlow, 0.f) &&
         atools::almostEqual(cruiseSpeed, 0.f) &&
         atools::almostEqual(cruiseFuelFlow, 0.f) &&
         atools::almostEqual(descentSpeed, 0.f) &&
         atools::almostEqual(descentVertSpeed, 0.f) &&
         atools::almostEqual(descentFuelFlow, 0.f);
}

float AircraftPerf::toFuelGal(float fuelGalLbs) const
{
  // Convert to gal if this perf is weight based
  return volume ? fuelGalLbs : ageo::fromLbsToGal(jetFuel, fuelGalLbs);
}

void AircraftPerf::readFromSettings(const QSettings& settings)
{
  name = settings.value(QStringLiteral("Options/Name")).toString();
  type = settings.value(QStringLiteral("Options/AircraftType")).toString();
  description = settings.value(QStringLiteral("Options/Description")).toString();
  simulator = settings.value(QStringLiteral("Options/Simulator")).toString();

  volume = settings.value(QStringLiteral("Options/FuelAsVolume")).toBool();
  jetFuel = settings.value(QStringLiteral("Options/JetFuel")).toBool();

  usableFuel = settings.value(QStringLiteral("Perf/UsableFuel")).toFloat();

  if(settings.contains(QStringLiteral("Perf/TaxiFuelLbs")))
    taxiFuel = settings.value(QStringLiteral("Perf/TaxiFuelLbs")).toFloat();
  if(settings.contains(QStringLiteral("Perf/TaxiFuelLbsGal")))
    taxiFuel = settings.value(QStringLiteral("Perf/TaxiFuelLbsGal")).toFloat();

  if(settings.contains(QStringLiteral("Perf/ReserveFuelLbs")))
    reserveFuel = settings.value(QStringLiteral("Perf/ReserveFuelLbs")).toFloat();
  if(settings.contains(QStringLiteral("Perf/ReserveFuelLbsGal")))
    reserveFuel = settings.value(QStringLiteral("Perf/ReserveFuelLbsGal")).toFloat();

  if(settings.contains(QStringLiteral("Perf/ExtraFuelLbs")))
    extraFuel = settings.value(QStringLiteral("Perf/ExtraFuelLbs")).toFloat();
  if(settings.contains(QStringLiteral("Perf/ExtraFuelLbsGal")))
    extraFuel = settings.value(QStringLiteral("Perf/ExtraFuelLbsGal")).toFloat();

  contingencyFuel = settings.value(QStringLiteral("Perf/ContingencyFuelPercent")).toFloat();

  climbVertSpeed = settings.value(QStringLiteral("Perf/ClimbVertSpeedFtPerMin")).toFloat();
  climbSpeed = settings.value(QStringLiteral("Perf/ClimbSpeedKtsTAS")).toFloat();
  climbFuelFlow = settings.value(QStringLiteral("Perf/ClimbFuelFlowLbsGalPerHour")).toFloat();

  cruiseSpeed = settings.value(QStringLiteral("Perf/CruiseSpeedKtsTAS")).toFloat();
  cruiseFuelFlow = settings.value(QStringLiteral("Perf/CruiseFuelFlowLbsGalPerHour")).toFloat();

  descentSpeed = settings.value(QStringLiteral("Perf/DescentSpeedKtsTAS")).toFloat();
  descentVertSpeed = settings.value(QStringLiteral("Perf/DescentVertSpeedFtPerMin")).toFloat();
  descentFuelFlow = settings.value(QStringLiteral("Perf/DescentFuelFlowLbsGalPerHour")).toFloat();

  alternateSpeed = settings.value(QStringLiteral("Perf/AlternateSpeed"), cruiseSpeed).toFloat();
  alternateFuelFlow = settings.value(QStringLiteral("Perf/AlternateFuelFlow"), cruiseFuelFlow).toFloat();

  minRunwayLength = settings.value(QStringLiteral("Perf/MinRunwayLength")).toFloat();
  runwayType = runwayTypeFromStr(settings.value(QStringLiteral("Perf/RunwayType"), QStringLiteral("Soft")).toString());
}

void AircraftPerf::writeToSettings(QSettings& settings)
{
  settings.remove(QStringLiteral("Perf/TaxiFuelLbs"));
  settings.remove(QStringLiteral("Perf/ReserveFuelLbs"));
  settings.remove(QStringLiteral("Perf/ExtraFuelLbs"));

  settings.setValue(QStringLiteral("Options/Metadata"), atools::programFileInfo());
  settings.setValue(QStringLiteral("Options/ProgramVersion"), QCoreApplication::applicationVersion());
  settings.setValue(QStringLiteral("Options/FormatVersion"), FORMAT_VERSION);
  settings.setValue(QStringLiteral("Options/Name"), name);
  settings.setValue(QStringLiteral("Options/AircraftType"), type);
  settings.setValue(QStringLiteral("Options/Description"), description);
  settings.setValue(QStringLiteral("Options/Simulator"), simulator);

  settings.setValue(QStringLiteral("Options/FuelAsVolume"), volume);
  settings.setValue(QStringLiteral("Options/JetFuel"), jetFuel);

  settings.setValue(QStringLiteral("Perf/UsableFuel"), QString::number(usableFuel));
  settings.setValue(QStringLiteral("Perf/TaxiFuelLbsGal"), QString::number(taxiFuel));
  settings.setValue(QStringLiteral("Perf/ReserveFuelLbsGal"), QString::number(reserveFuel));
  settings.setValue(QStringLiteral("Perf/ExtraFuelLbsGal"), QString::number(extraFuel));
  settings.setValue(QStringLiteral("Perf/ContingencyFuelPercent"), QString::number(contingencyFuel));

  settings.setValue(QStringLiteral("Perf/ClimbVertSpeedFtPerMin"), QString::number(climbVertSpeed));
  settings.setValue(QStringLiteral("Perf/ClimbSpeedKtsTAS"), QString::number(climbSpeed));
  settings.setValue(QStringLiteral("Perf/ClimbFuelFlowLbsGalPerHour"), QString::number(climbFuelFlow));

  settings.setValue(QStringLiteral("Perf/CruiseSpeedKtsTAS"), QString::number(cruiseSpeed));
  settings.setValue(QStringLiteral("Perf/CruiseFuelFlowLbsGalPerHour"), QString::number(cruiseFuelFlow));

  settings.setValue(QStringLiteral("Perf/DescentSpeedKtsTAS"), QString::number(descentSpeed));
  settings.setValue(QStringLiteral("Perf/DescentVertSpeedFtPerMin"), QString::number(descentVertSpeed));
  settings.setValue(QStringLiteral("Perf/DescentFuelFlowLbsGalPerHour"), QString::number(descentFuelFlow));

  settings.setValue(QStringLiteral("Perf/AlternateSpeed"), QString::number(alternateSpeed));
  settings.setValue(QStringLiteral("Perf/AlternateFuelFlow"), QString::number(alternateFuelFlow));

  settings.setValue(QStringLiteral("Perf/MinRunwayLength"), QString::number(minRunwayLength));
  settings.setValue(QStringLiteral("Perf/RunwayType"), runwayTypeToStr(runwayType));
}

} // namespace perf
} // namespace fs
} // namespace atools
