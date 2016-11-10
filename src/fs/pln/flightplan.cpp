/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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

#include "fs/pln/flightplan.h"
#include "exception.h"
#include "geo/pos.h"
#include "atools.h"

#include <QDateTime>
#include <QFile>
#include <QXmlStreamReader>

namespace atools {
namespace fs {
namespace pln {

Flightplan::Flightplan()
{

}

Flightplan::Flightplan(const Flightplan& other)
{
  this->operator=(other);

}

Flightplan::~Flightplan()
{

}

Flightplan& Flightplan::operator=(const Flightplan& other)
{
  flightplanType = other.flightplanType;
  routeType = other.routeType;
  cruisingAlt = other.cruisingAlt;
  appVersionMajor = other.appVersionMajor;
  appVersionBuild = other.appVersionBuild;
  filename = other.filename;
  title = other.title;
  departureIdent = other.departureIdent;
  destinationIdent = other.destinationIdent;
  description = other.description;
  departureParkingName = other.departureParkingName;
  departureAiportName = other.departureAiportName;
  destinationAiportName = other.destinationAiportName;
  departurePos = other.departurePos;
  destinationPos = other.destinationPos;
  entries = other.entries;
  return *this;
}

void Flightplan::load(const QString& file)
{
  filename = file;
  QFile xmlFile(filename);

  if(xmlFile.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    entries.clear();
    QXmlStreamReader reader;
    reader.setDevice(&xmlFile);

    // Skip all the useless stuff until we hit the document
    readUntilElement(reader, "SimBase.Document");

    // Skip all until the flightplan is found
    readUntilElement(reader, "FlightPlan.FlightPlan");

    while(reader.readNextStartElement())
    {
      if(reader.error() != QXmlStreamReader::NoError)
        throw Exception("Error reading \"" + filename + "\": " + reader.errorString());

      QStringRef name = reader.name();
      if(name == "Title")
        title = reader.readElementText();
      else if(name == "FPType")
        flightplanType = stringFlightplanType(reader.readElementText());
      else if(name == "RouteType")
        routeType = stringToRouteType(reader.readElementText());
      else if(name == "CruisingAlt")
        cruisingAlt = reader.readElementText().toInt();
      else if(name == "DepartureID")
        departureIdent = reader.readElementText();
      else if(name == "DepartureLLA")
      {
        QString txt = reader.readElementText();
        if(!txt.isEmpty())
          departurePos = geo::Pos(txt);
      }
      else if(name == "DestinationID")
        destinationIdent = reader.readElementText();
      else if(name == "DestinationLLA")
      {
        QString txt = reader.readElementText();
        if(!txt.isEmpty())
          destinationPos = geo::Pos(txt);
      }
      else if(name == "Descr")
        description = reader.readElementText();
      else if(name == "DeparturePosition")
        departureParkingName = reader.readElementText();
      else if(name == "DepartureName")
        departureAiportName = reader.readElementText();
      else if(name == "DestinationName")
        destinationAiportName = reader.readElementText();
      else if(name == "AppVersion")
        readAppVersion(reader);
      else if(name == "ATCWaypoint")
        readWaypoint(reader);
      else
        reader.skipCurrentElement();
    }
  }
  else
    throw Exception(tr("Cannot open file %1. Reason: %2").arg(file).arg(xmlFile.errorString()));
}

void Flightplan::save(const QString& file)
{
  filename = file;
  QFile xmlFile(filename);

  if(xmlFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QString xmlString;

    QXmlStreamWriter writer(&xmlString);
    writer.setCodec("UTF-8");
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(4);

    writer.writeStartDocument("1.0");
    writer.writeStartElement("SimBase.Document");
    writer.writeAttribute("Type", "AceXML");
    writer.writeAttribute("version", "1,0");
    writer.writeTextElement("Descr", "AceXML Document");

    writer.writeComment(tr(" Created by %1 Version %2 (revision %3) on %4 ").
                        arg(QApplication::applicationName()).
                        arg(QApplication::applicationVersion()).
                        arg(atools::gitRevision()).
                        arg(QLocale().toString(QDateTime::currentDateTime())).
                        replace("-", " "));

    writer.writeStartElement("FlightPlan.FlightPlan");

    writer.writeTextElement("Title", title);
    writer.writeTextElement("FPType", flightplanTypeToString(flightplanType));

    if(routeType != DIRECT)
      writer.writeTextElement("RouteType", routeTypeToString(routeType));

    writer.writeTextElement("CruisingAlt", QString().number(cruisingAlt));
    writer.writeTextElement("DepartureID", departureIdent);
    writer.writeTextElement("DepartureLLA",
                            departurePos.isValid() ? departurePos.toLongString() : QString());
    writer.writeTextElement("DestinationID", destinationIdent);
    writer.writeTextElement("DestinationLLA",
                            destinationPos.isValid() ? destinationPos.toLongString() : QString());
    writer.writeTextElement("Descr", description);
    writer.writeTextElement("DeparturePosition", departureParkingName);
    writer.writeTextElement("DepartureName", departureAiportName);
    writer.writeTextElement("DestinationName", destinationAiportName);

    writer.writeStartElement("AppVersion");
    writer.writeTextElement("AppVersionMajor", QString().number(appVersionMajor));
    writer.writeTextElement("AppVersionBuild", QString().number(appVersionBuild));
    writer.writeEndElement(); // AppVersion

    for(const FlightplanEntry& e : entries)
    {
      writer.writeStartElement("ATCWaypoint");

      writer.writeAttribute("id", e.getWaypointId());
      writer.writeTextElement("ATCWaypointType", e.getWaypointTypeAsString());

      if(!e.getPosition().isValid())
        throw atools::Exception("Invalid position in flightplan for id " + e.getWaypointId());

      writer.writeTextElement("WorldPosition", e.getPosition().toLongString());

      if(!e.getAirway().isEmpty())
        writer.writeTextElement("ATCAirway", e.getAirway());

      if(!e.getIcaoRegion().isEmpty() || !e.getIcaoIdent().isEmpty())
      {
        writer.writeStartElement("ICAO");

        if(!e.getIcaoRegion().isEmpty())
          writer.writeTextElement("ICAORegion", e.getIcaoRegion());
        writer.writeTextElement("ICAOIdent", e.getIcaoIdent());

        writer.writeEndElement(); // ICAO
      }

      writer.writeEndElement(); // ATCWaypoint
    }

    writer.writeEndElement(); // FlightPlan.FlightPlan
    writer.writeEndElement(); // SimBase.Document
    writer.writeEndDocument();

#ifndef Q_OS_WIN32
    // Convert EOL always to Windows (0x0a -> 0x0d0a)
    xmlString.replace("\n", "\r\n");
#endif

    QByteArray utf8 = xmlString.toUtf8();
    xmlFile.write(utf8.data(), utf8.size());
  }
  else
    throw Exception(tr("Cannot open file %1. Reason: %2").arg(file).arg(xmlFile.errorString()));
}

void Flightplan::readUntilElement(QXmlStreamReader& reader, const QString& name)
{
  while(reader.name() != name)
  {
    reader.readNextStartElement();

    if(reader.error() != QXmlStreamReader::NoError)
      throw Exception("Error reading \"" + filename + "\": " + reader.errorString());
  }
}

void Flightplan::readAppVersion(QXmlStreamReader& reader)
{
  while(reader.readNextStartElement())
  {
    QStringRef aName = reader.name();
    if(aName == "AppVersionMajor")
      appVersionMajor = reader.readElementText().toInt();
    else if(aName == "AppVersionBuild")
      appVersionBuild = reader.readElementText().toInt();
    else
      reader.skipCurrentElement();
  }
}

void Flightplan::readWaypoint(QXmlStreamReader& reader)
{
  FlightplanEntry entry;

  entry.setWaypointId(reader.attributes().value("id").toString());

  while(reader.readNextStartElement())
  {
    QStringRef rName = reader.name();
    if(rName == "ATCWaypointType")
      entry.setWaypointType(reader.readElementText());
    else if(rName == "WorldPosition")
      entry.setPosition(geo::Pos(reader.readElementText()));
    else if(rName == "ATCAirway")
      entry.setAirway(reader.readElementText());
    else if(rName == "ICAO")
      while(reader.readNextStartElement())
      {
        QStringRef iName = reader.name();
        if(iName == "ICAORegion")
          entry.setIcaoRegion(reader.readElementText());
        else if(iName == "ICAOIdent")
          entry.setIcaoIdent(reader.readElementText());
        else
          reader.skipCurrentElement();
      }
    else
      reader.skipCurrentElement();
  }
  entries.append(entry);
}

void Flightplan::clear()
{
  entries.clear();

  filename.clear();
  title.clear();
  departureIdent.clear();
  destinationIdent.clear();
  description.clear();
  departureParkingName.clear();
  departureAiportName.clear();
  destinationAiportName.clear();

  departurePos = atools::geo::Pos();
  destinationPos = atools::geo::Pos();

  flightplanType = VFR;
  routeType = DIRECT;
  cruisingAlt = 10000;

  appVersionBuild = APPVERSION_BUILD;
  appVersionMajor = APPVERSION_MAJOR;
}

void Flightplan::reverse()
{
  std::reverse(entries.begin(), entries.end());

  departureAiportName.swap(destinationAiportName);
  departureIdent.swap(destinationIdent);
  departurePos.swap(destinationPos);
  setDepartureParkingName(QString());
}

QString Flightplan::flightplanTypeToString(FlightplanType type)
{
  switch(type)
  {
    case atools::fs::pln::IFR:
      return "IFR";

    case atools::fs::pln::VFR:
      return "VFR";
  }
  return QString();
}

FlightplanType Flightplan::stringFlightplanType(const QString& str)
{
  if(str == "IFR")
    return IFR;

  return VFR;
}

QString Flightplan::routeTypeToString(RouteType type)
{
  switch(type)
  {
    case atools::fs::pln::LOW_ALTITUDE:
      return "LowAlt";

    case atools::fs::pln::HIGH_ALTITUDE:
      return "HighAlt";

    case atools::fs::pln::VOR:
      return "VOR";

    case atools::fs::pln::DIRECT:
      return "Direct"; // Not an actual value in the XML
  }
  return QString();
}

RouteType Flightplan::stringToRouteType(const QString& str)
{
  if(str == "LowAlt")
    return LOW_ALTITUDE;
  else if(str == "HighAlt")
    return HIGH_ALTITUDE;
  else if(str == "VOR")
    return VOR;

  return DIRECT;
}

} // namespace pln
} // namespace fs
} // namespace atools
