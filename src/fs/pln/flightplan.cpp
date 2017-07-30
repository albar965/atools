/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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
#include "geo/calculations.h"
#include "atools.h"
#include "geo/linestring.h"

#include <QDataStream>
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
  properties = other.properties;
  return *this;
}

void Flightplan::load(const QString& file)
{
  filename = file;
  QFile xmlFile(filename);

  if(xmlFile.open(QIODevice::ReadOnly))
  {
    entries.clear();
    QXmlStreamReader reader(&xmlFile);

    // Skip all the useless stuff until we hit the document
    readUntilElement(reader, "SimBase.Document");
    readUntilElement(reader, "Descr");
    while(!reader.atEnd())
    {
      reader.readNext();
      if(reader.isComment())
      {
        QString comment = reader.text().toString().trimmed();
        if(comment.startsWith("LNMDATA"))
        {
          comment.remove(0, 7);
          QStringList data = comment.split("|");
          for(const QString& prop : data)
            properties.insert(prop.section("=", 0, 0).trimmed(), prop.section("=", 1, 1).trimmed());
        }
      }
      if(reader.isStartElement())
        break;
    }
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

void Flightplan::save(const QString& file, bool clean)
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

    properties.insert("_lnm", programInfo());

    if(!clean)
    {
      QStringList comment;
      for(const QString& key : properties.keys())
      {
        if(!key.isEmpty())
          comment.append("\n         " + key + "=" + properties.value(key));
      }

      std::sort(comment.begin(), comment.end());
      writer.writeComment(" LNMDATA" + comment.join("|") + "\n");
    }

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

    for(const FlightplanEntry& entry : entries)
    {
      if(entry.isNoSave())
        // Do not save stuff like procedure points
        continue;

      writer.writeStartElement("ATCWaypoint");

      // Trim to max allowed length for FSX/P3D
      writer.writeAttribute("id", entry.getWaypointId().left(10));
      writer.writeTextElement("ATCWaypointType", entry.getWaypointTypeAsString());

      if(!entry.getPosition().isValid())
        throw atools::Exception("Invalid position in flightplan for id " + entry.getWaypointId());

      atools::geo::Pos pos = entry.getPosition();

      // Use null altitude for all except airports
      if(entry.getWaypointType() != atools::fs::pln::entry::AIRPORT)
        pos.setAltitude(0.f);

      writer.writeTextElement("WorldPosition", pos.toLongString());

      if(!entry.getAirway().isEmpty())
        writer.writeTextElement("ATCAirway", entry.getAirway());

      if(!entry.getIcaoRegion().isEmpty() || !entry.getIcaoIdent().isEmpty())
      {
        writer.writeStartElement("ICAO");

        if(!entry.getIcaoRegion().isEmpty())
          writer.writeTextElement("ICAORegion", entry.getIcaoRegion());
        writer.writeTextElement("ICAOIdent", entry.getIcaoIdent());

        writer.writeEndElement(); // ICAO
      }

      writer.writeEndElement(); // ATCWaypoint
    }

    writer.writeEndElement(); // FlightPlan.FlightPlan
    writer.writeEndElement(); // SimBase.Document
    writer.writeEndDocument();

    // Fixed Qt's retarded change where they think encoding is not needed in a string
    xmlString.replace("<?xml version=\"1.0\"?>", "<?xml version=\"1.0\" encoding=\"UTF-8\"?>");

#ifndef Q_OS_WIN32
    // Convert EOL always to Windows (0x0a -> 0x0d0a)
    xmlString.replace("\n", "\r\n");
#endif

    // Breaks XML standard but gives better compatibility for third party applications
    xmlString.replace("&quot;", "\"");

    QByteArray utf8 = xmlString.toUtf8();
    xmlFile.write(utf8.data(), utf8.size());
  }
  else
    throw Exception(tr("Cannot open PLN file %1. Reason: %2").arg(file).arg(xmlFile.errorString()));
}

void Flightplan::saveFlp(const QString& file)
{
  filename = file;
  QFile flpFile(filename);

  if(flpFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QString flpString;
    QTextStream stream(&flpString);

    stream << "[CoRte]" << endl;
    stream << "ArptDep=" << departureIdent << endl;
    stream << "ArptArr=" << destinationIdent << endl;
    stream << "RwyDep=" << endl;
    stream << "RwyArr=" << endl;
    stream << "RwyArrFinal=" << endl;
    stream << "SID=" << endl;
    stream << "STAR=" << endl;
    stream << "APPR_Trans=" << endl;

    // Check if all legs have an airway assignment
    bool hasMissingAirways = false;
    for(int i = 2; i < entries.size() - 1; i++)
    {
      if(entries.at(i).isNoSave())
        // Do not save procedure points
        continue;
      hasMissingAirways |= entries.at(i).getAirway().isEmpty();
    }

    if(hasMissingAirways)
    {
      // Save with direct waypoints and coordinates
      int index = 1;
      for(int i = 1; i < entries.size() - 1; i++)
      {
        const FlightplanEntry& entry = entries.at(i);
        if(entry.isNoSave())
          // Do not save stuff like procedure points
          continue;

        stream << "DctWpt" << index << "=" << entry.getWaypointId() << endl;

        QString coords = QString("%1,%2").
                         arg(entry.getPosition().getLatY(), 0, 'f', 6).
                         arg(entry.getPosition().getLonX(), 0, 'f', 6);

        stream << "DctWpt" << index << "Coordinates=" << coords << endl;
        index++;
      }
    }
    else
    {
      // Save with airways
      int index = 1;
      for(int i = 1; i < entries.size() - 2; i++)
      {
        const FlightplanEntry& entry = entries.at(i);
        if(entry.isNoSave())
          // Do not save stuff like procedure points
          continue;

        stream << "Airway" << index << "=" << entries.at(i + 1).getAirway() << endl;
        stream << "Airway" << index << "FROM=" << entry.getWaypointId() << endl;
        stream << "Airway" << index << "TO=" << entries.at(i + 1).getWaypointId() << endl;
        index++;
      }
    }

#ifndef Q_OS_WIN32
    // Convert EOL always to Windows (0x0a -> 0x0d0a)
    flpString.replace("\n", "\r\n");
#endif

    QByteArray utf8 = flpString.toUtf8();
    flpFile.write(utf8.data(), utf8.size());
  }
  else
    throw Exception(tr("Cannot open FLP file %1. Reason: %2").arg(file).arg(flpFile.errorString()));
}

void Flightplan::saveGpx(const QString& file, const geo::LineString& track, const QVector<quint32>& timestamps)
{
  filename = file;

  QFile xmlFile(filename);

  if(xmlFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QXmlStreamWriter writer(&xmlFile);
    writer.setCodec("UTF-8");
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(2);
    writer.writeStartDocument("1.0");

    // <gpx xmlns="http://www.topografix.com/GPX/1/1" version="1.1" creator="Wikipedia"
    // xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
    // xsi:schemaLocation="http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd">
    writer.writeStartElement("gpx");
    // writer.writeDefaultNamespace("http://www.topografix.com/GPX/1/1");
    // writer.writeAttribute("version", "1.1");
    writer.writeAttribute("creator", "Little Navmap");
    // writer.writeNamespace("http://www.w3.org/2001/XMLSchema-instance", "xsi");
    // writer.writeAttribute("xsi:schemaLocation",
    // "http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd");

    // writer.writeComment(programInfo());

    // <metadata>
    // <link href="http://www.garmin.com">
    // <text>Garmin International</text>
    // </link>
    // <time>2009-10-17T22:58:43Z</time>
    // </metadata>
    writer.writeStartElement("metadata");
    writer.writeStartElement("link");
    writer.writeAttribute("href", "https://albar965.github.io/littlenavmap.html");
    writer.writeTextElement("text", programInfo());
    writer.writeEndElement(); // link
    writer.writeEndElement(); // metadata

    writer.writeStartElement("rte");

    QString descr = QString("%1 (%2) to %3 (%4) at %5 ft, %6, %7").
                    arg(departureAiportName).arg(departureIdent).
                    arg(destinationAiportName).arg(destinationIdent).
                    arg(getCruisingAltitude()).
                    arg(flightplanTypeToString(flightplanType)).
                    arg(routeTypeToString(routeType));

    writer.writeTextElement("name", title + tr(" Flight Plan"));
    writer.writeTextElement("desc", descr);

    // Write route ========================================================
    for(const FlightplanEntry& entry : entries)
    {
      if(entry.isNoSave())
        // Do not save procedure points
        continue;

      // <rtept lat="52.0" lon="13.5">
      // <ele>33.0</ele>
      // <time>2011-12-13T23:59:59Z</time>
      // <name>rtept 1</name>
      // </rtept>
      writer.writeStartElement("rtept");
      writer.writeAttribute("lat", QString::number(entry.getPosition().getLatY(), 'f', 6));
      writer.writeAttribute("lon", QString::number(entry.getPosition().getLonX(), 'f', 6));

      writer.writeTextElement("name", entry.getWaypointId());
      writer.writeTextElement("desc", entry.getWaypointTypeAsString());

      writer.writeEndElement(); // rtept
    }

    writer.writeEndElement(); // rte

    // Write track ========================================================
    if(!track.isEmpty())
    {
      writer.writeStartElement("trk");
      writer.writeTextElement("name", title + tr(" Track"));
      writer.writeTextElement("desc", descr);

      writer.writeStartElement("trkseg");

      for(int i = 0; i < track.size(); ++i)
      {
        const atools::geo::Pos& pos = track.at(i);
        writer.writeStartElement("trkpt");

        writer.writeAttribute("lat", QString::number(pos.getLatY(), 'f', 6));
        writer.writeAttribute("lon", QString::number(pos.getLonX(), 'f', 6));

        writer.writeTextElement("ele", QString::number(atools::geo::feetToMeter(pos.getAltitude())));

        if(!timestamps.isEmpty())
        {
          // (UTC/Zulu) in ISO 8601 format: yyyy-mm-ddThh:mm:ssZ
          // <time>2011-01-16T23:59:01Z</time>
          writer.writeTextElement("time", QDateTime::fromTime_t(timestamps.at(i), Qt::UTC).
                                  toString("yyyy-MM-ddTHH:mm:ssZ"));
        }

        writer.writeEndElement(); // trkpt
      }
      writer.writeEndElement(); // trkseg
      writer.writeEndElement(); // trk
    }

    writer.writeEndElement(); // gpx
    writer.writeEndDocument();

  }
  else
    throw Exception(tr("Cannot open PLN file %1. Reason: %2").arg(file).arg(xmlFile.errorString()));
}

void Flightplan::saveFms(const QString& file)
{
  filename = file;
  QFile fmsFile(filename);

  if(fmsFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QTextStream stream(&fmsFile);

    // OS
     #if defined(Q_OS_MACOS)
    stream << "A" << endl;
     #else
    stream << "I" << endl;
     #endif

    // File version
    stream << "3 version" << endl;
    stream << "1" << endl;

    // Number of waypoints
    stream << (entries.size() - 1) << endl;

    int i = 0;
    for(const FlightplanEntry& entry : entries)
    {
      if(entry.isNoSave() || entry.getWaypointType() == atools::fs::pln::entry::UNKNOWN)
        continue;

      // 1 - Airport ICAO
      // 2 - NDB
      // 3 - VOR
      // 11 - Fix
      // 28 - Lat/Lon Position

      if(entry.getWaypointType() == atools::fs::pln::entry::USER)
      {
        float laty = std::abs(entry.getPosition().getLatY());
        float lonx = std::abs(entry.getPosition().getLonX());

        stream << "28 ";
        // +12.345_+009.459 Correct for a waypoint at 12.345°/0.459°.
        // -28.478_-056.370 Correct for a waypoint at -28.478°/-56.370°.
        stream << (laty < 0.f ? "-" : "+")
               << QString("%1").arg(laty, 2, 'f', 3, QChar('0')).rightJustified(6, QChar('0'), false)
               << "_"
               << (lonx < 0.f ? "-" : "+")
               << QString("%1").arg(lonx, 3, 'f', 3, QChar('0')).rightJustified(7, QChar('0'), false)
               << " ";
      }
      else
      {
        if(entry.getWaypointType() == atools::fs::pln::entry::AIRPORT)
          stream << "1 ";
        else if(entry.getWaypointType() == atools::fs::pln::entry::INTERSECTION)
          stream << "11 ";
        else if(entry.getWaypointType() == atools::fs::pln::entry::VOR)
          stream << "3 ";
        else if(entry.getWaypointType() == atools::fs::pln::entry::NDB)
          stream << "2 ";

        stream << entry.getWaypointId() << " ";
      }

      float alt = getCruisingAltitude();
      if(i == 0 || i >= entries.size() - 1)
        alt = 0.f;

      stream << QString::number(alt, 'f', 6) << " "
             << QString::number(entry.getPosition().getLatY(), 'f', 6)
             << " "
             << QString::number(entry.getPosition().getLonX(), 'f', 6)
             << endl;

      i++;
    }
  }
  else
    throw Exception(tr("Cannot open FMS file %1. Reason: %2").arg(file).arg(fmsFile.errorString()));
}

void Flightplan::saveRte(const QString& file)
{
  namespace ple = atools::fs::pln::entry;
  int userWaypointNum = 1;

  const int NO_DATA_NUM = -1000000;
  const QString NO_DATA_STR("-");
  enum
  {
    AIRPORT = 1, OTHER = 2, WAYPOINT = 5
  };

  enum
  {
    NONE = 0, CLIMB = 1, CRUISE = 2, DESCENT = 3
  };

  filename = file;
  QFile rteFile(filename);

  if(rteFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QString rteString;
    QTextStream stream(&rteString);

    stream << tr("PMDG RTE Created by %1 Version %2 (revision %3) on %4 ").
      arg(QApplication::applicationName()).
      arg(QApplication::applicationVersion()).
      arg(atools::gitRevision()).
      arg(QLocale().toString(QDateTime::currentDateTime())).
      replace("-", " ") << endl << endl;

    stream << entries.size() << endl << endl;

    stream << departureIdent << endl << AIRPORT << endl << "DIRECT" << endl;
    posToRte(stream, departurePos, true);
    stream << endl << NO_DATA_STR << endl
           << 1 /* Departure*/ << endl << 0 /* Runway position */ << endl << endl;

    stream << CLIMB << endl; // Restriction phase climb
    stream << atools::roundToInt(departurePos.getAltitude()); // Restriction altitude, if restricted

    // Restriction type, altitude and speed
    stream << endl << NO_DATA_STR << endl << NO_DATA_NUM << endl << NO_DATA_NUM << endl << endl;

    for(int i = 1; i < entries.size() - 1; i++)
    {
      const FlightplanEntry& entry = entries.at(i);

      if(entry.isNoSave())
        // Do not save stuff like procedure points
        continue;

      if(entry.getWaypointType() == ple::USER)
      {
        stream << "WPT" << userWaypointNum++ << endl;
        stream << OTHER << endl;
      }
      else
      {
        stream << (entry.getIcaoIdent().isEmpty() ? NO_DATA_STR : entry.getIcaoIdent()) << endl;
        stream << (entry.getWaypointType() == ple::AIRPORT ? AIRPORT : WAYPOINT) << endl;
      }

      QString nextAirway = entries.at(i + 1).getAirway();
      stream << (nextAirway.isEmpty() ? "DIRECT" : nextAirway) << endl;

      posToRte(stream, entry.getPosition(), false);
      stream << endl << 0 << endl << 0 << endl << 0 << endl << endl; // Restriction fields
    }

    stream << destinationIdent << endl << AIRPORT << endl << NO_DATA_STR << endl;
    posToRte(stream, destinationPos, true);
    stream << endl << NO_DATA_STR << endl
           << 0 /* no departure*/ << endl << 0 /* Runway position */ << endl << endl;

    stream << CLIMB << endl; // Restriction phase
    stream << atools::roundToInt(destinationPos.getAltitude()) << endl; // Restriction altitude, if restricted
    // Restriction type, altitude and speed
    stream << NO_DATA_STR << endl << NO_DATA_NUM << endl << NO_DATA_NUM << endl;

#ifndef Q_OS_WIN32
    // Convert EOL always to Windows (0x0a -> 0x0d0a)
    rteString.replace("\n", "\r\n");
#endif

    QByteArray utf8 = rteString.toUtf8();
    rteFile.write(utf8.data(), utf8.size());
  }
  else
    throw Exception(tr("Cannot open RTE file %1. Reason: %2").arg(file).arg(rteFile.errorString()));
}

void Flightplan::removeNoSaveEntries()
{
  auto it = std::remove_if(entries.begin(), entries.end(),
                           [](const FlightplanEntry& type) -> bool
        {
          return type.isNoSave();
        });

  if(it != entries.end())
    entries.erase(it, entries.end());
}

void Flightplan::posToRte(QTextStream& stream, const geo::Pos& pos, bool alt)
{
  stream.setRealNumberNotation(QTextStream::FixedNotation);
  stream.setRealNumberPrecision(4);

  stream << 1 << (pos.getLatY() > 0.f ? " N " : " S ")
         << std::abs(pos.getLatY())
         << (pos.getLonX() > 0.f ? " E " : " W ")
         << std::abs(pos.getLonX());

  stream.setRealNumberPrecision(0);
  stream << " " << (alt ? pos.getAltitude() : 0.f);
}

float Flightplan::getDistanceNm() const
{
  float distanceMeter = 0.f;
  if(entries.size() > 1)
  {
    for(int i = 0; i < entries.size() - 1; i++)
      distanceMeter += entries.at(i).getPosition().distanceMeterTo(entries.at(i + 1).getPosition());
  }
  return atools::geo::meterToNm(distanceMeter);
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

QString Flightplan::programInfo()
{
  return tr("Created by %1 Version %2 (revision %3) on %4").
         arg(QApplication::applicationName()).
         arg(QApplication::applicationVersion()).
         arg(atools::gitRevision()).
         arg(QLocale().toString(QDateTime::currentDateTime())).
         replace("-", " ");
}

} // namespace pln
} // namespace fs
} // namespace atools
