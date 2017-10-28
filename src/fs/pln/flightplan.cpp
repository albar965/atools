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
#include "fs/util/coordinates.h"

#include <QDataStream>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QXmlStreamReader>

namespace atools {
namespace fs {
namespace pln {

static const QRegularExpression FLP_DCT_WPT("DctWpt(\\d+)(Coordinates)?", QRegularExpression::CaseInsensitiveOption);
static const QRegularExpression FLP_DCT_AWY("Airway(\\d+)(FROM|TO)?", QRegularExpression::CaseInsensitiveOption);

static const QRegularExpression FS9_MATCH("(^appversion\\s*=|^title\\s*=|^description\\s*=|^type\\s*=|"
                                          "^routetype\\s*=|^cruising_altitude\\s*=|^departure_id\\s*=|^destination_id\\s*=)");
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
  fileFormat = other.fileFormat;
  return *this;
}

void Flightplan::load(const QString& file)
{
  QStringList lines = probeFile(file);

  if(lines.isEmpty())
    throw Exception(tr("Cannot open empty flight plan file \"%1\".").arg(file));

  if(!lines.isEmpty() &&
     lines.first().startsWith("[corte]"))
    // FLP: [CoRte]
    loadFlp(file);
  else if(lines.size() >= 2 &&
          lines.at(0).startsWith("<?xml version") &&
          lines.at(1).startsWith("<simbase.document"))
    // FSX PLN <?xml version
    loadFsx(file);
  else if(lines.size() >= 2 &&
          lines.at(0).startsWith("[flightplan]") &&
          FS9_MATCH.match(lines.at(1)).hasMatch())
    loadFs9(file);
  else if(lines.size() >= 4 &&
          // Old format
          // I
          // 3 version
          // 1
          // 4
          (lines.at(0) == "i" || lines.at(0) == "a") &&
          lines.at(1).startsWith("3 version") &&
          lines.at(2).at(0).isDigit() &&
          lines.at(3).at(0).isDigit())
    loadFms(file);
  else if(lines.size() >= 3 &&
          // New v11 format
          // I
          // 1100 Version
          // CYCLE 1710
          (lines.at(0) == "i" || lines.at(0) == "a") &&
          lines.at(1).startsWith("1100 version") &&
          lines.at(2).startsWith("cycle "))
    loadFms(file);
  else
    throw Exception(tr("Cannot open flight plan file \"%1\". No supported flight plan format detected. "
                       "Only PLN (FSX XML and FS9 INI), FMS and FLP are supported.").arg(file));

}

void Flightplan::loadFlp(const QString& file)
{
  qDebug() << Q_FUNC_INFO;

  // [CoRte]
  // ArptDep=EDDF
  // ArptArr=LOWW
  // RwyDep=EDDF07C
  // RwyArr=LOWW16
  // RwyArrFinal=FI16
  // SID=SULU7E
  // STAR=VENE2W
  // APPR_Trans=NER6L
  // DctWpt1=BOMBI
  // DctWpt1Coordinates=50.056667,8.800278
  // DctWpt2=ESATI
  // DctWpt14Coordinates=48.802502,13.651111
  // DctWpt15=DEGAB
  // DctWpt15Coordinates=48.705685,13.931495

  filename = file;

  QFile flpFile(filename);

  if(flpFile.open(QIODevice::ReadOnly))
  {
    FlightplanEntry entry, departure, destination;
    int wptNum = -1;

    QTextStream stream(&flpFile);
    stream.setCodec("UTF-8");
    stream.setAutoDetectUnicode(true);
    while(!stream.atEnd())
    {
      QString line = stream.readLine().simplified();
      if(!line.isEmpty())
      {
        QString key = line.section('=', 0, 0).toLower().trimmed();
        QString value = line.section('=', 1, 1).trimmed();

        if(key == "arptdep")
        {
          departureIdent = value;
          departure.setIcaoIdent(value);
          departure.setWaypointType(atools::fs::pln::entry::AIRPORT);
        }
        else if(key == "arptarr")
        {
          destinationIdent = value;
          destination.setIcaoIdent(value);
          destination.setWaypointType(atools::fs::pln::entry::AIRPORT);
        }
        else if(key.startsWith("dctwpt"))
        {
          QRegularExpressionMatch localMatch = FLP_DCT_WPT.match(key);
          int num = localMatch.captured(1).toInt();
          QString coords = localMatch.captured(2);

          if(num > wptNum)
          {
            // Number has changed - add new one
            if(wptNum != -1)
            {
              // not the first iteration
              entries.append(entry);
              entry = FlightplanEntry();
            }
            wptNum = num;
          }

          if(coords.isEmpty())
          {
            entry.setIcaoIdent(value);
            entry.setWaypointId(value);
          }
          else if(coords.toLower() == "coordinates")
            entry.setPosition(atools::geo::Pos(value.section(',', 1, 1).toFloat(),
                                               value.section(',', 0, 0).toFloat()));
        }
        else if(key.startsWith("airway"))
        {
          QRegularExpressionMatch match = FLP_DCT_AWY.match(key);
          int num = match.captured(1).toInt();
          QString fromTo = match.captured(2);

          if(num > wptNum)
          {
            if(wptNum != -1)
            {
              entries.append(entry);
              entry = FlightplanEntry();
            }
            wptNum = num;
          }

          if(fromTo.isEmpty())
            entry.setAirway(value);
          else if(fromTo.toLower() == "from")
          {
            if(entries.isEmpty() || entries.last().getIcaoIdent() != value)
            {
              FlightplanEntry from;
              from.setIcaoIdent(value);
              from.setWaypointId(value);
              entries.append(from);
            }
          }
          else if(fromTo.toLower() == "to")
          {
            entry.setIcaoIdent(value);
            entry.setWaypointId(value);
          }
        }
        else if(!value.isEmpty())
        {
          if(key == "rwydep")
            properties.insert(SIDAPPRRW, value.mid(departureIdent.size()));
          else if(key == "sid")
            properties.insert(SIDAPPR, value);
          else if(key == "sid_trans")
            properties.insert(SIDTRANS, value);
          else if(key == STAR)
            properties.insert(STAR, value);
          else if(key == "star_trans")
            properties.insert(STARTRANS, value);
          else if(key == "rwyarr")
            properties.insert(APPROACHRW, value.mid(destinationIdent.size()));
          else if(key == "rwyarrfinal")
            properties.insert(APPROACH, value);
          else if(key == "appr_trans")
            properties.insert(TRANSITION, value);
        }
      }
    }
    entries.append(entry);
    entries.prepend(departure);
    entries.append(destination);

    flpFile.close();
    flightplanType = IFR;
    routeType = UNKNOWN; // Determine type when resolving navaids from the database
    cruisingAlt = 0.f; // Use either GUI value or calculate from airways

    fileFormat = FLP;
    adjustDepartureAndDestination();
  }
}

void Flightplan::loadFms(const QString& file)
{
  qDebug() << Q_FUNC_INFO;

  // Old format
  // I
  // 3 version
  // 1
  // 4
  // 1 EDDM 0.000000 48.364822 11.794361
  // 2 GL 1000 57.083820 9.680093
  // 3 KFK 2000 38.803889 30.546944
  // 11 DETKO 0.600000 28.097222 49.525000
  // 28 +13.691_+100.760 0.000000 13.691230 100.760811

  // New v11 format
  // I
  // 1100 Version
  // CYCLE 1710
  // ADEP KCUB
  // DEPRWY RW13
  // ADES KRDU
  // DESRWY RW05L
  // APP I05L
  // NUMENR 9
  // 1 KCUB ADEP 0.000000 33.970470 -80.995247
  // 3 CTF DRCT 0.000000 34.650497 -80.274918
  // 11 NOMOE V155 0.000000 34.880920 -79.996437
  // 11 LILLS V155 0.000000 34.935440 -79.930206
  // 3 SDZ V155 0.000000 35.215481 -79.587936
  // 11 OCHOC V155 0.000000 35.402336 -79.361153
  // 11 MOATS V155 0.000000 35.621601 -79.092964
  // 3 RDU V155 0.000000 35.872520 -78.783340
  // 1 KRDU ADES 435.000000 35.877640 -78.787476

  filename = file;

  QFile fmsFile(filename);
  int version = 0;
  bool v11Format = false;
  int minListSize = 5;
  int fieldOffset = 0;

  if(fmsFile.open(QIODevice::ReadOnly))
  {
    QTextStream stream(&fmsFile);
    stream.setCodec("UTF-8");
    stream.setAutoDetectUnicode(true);

    stream.readLine(); // I
    bool ok = false;
    version = stream.readLine().section(" ", 0, 0).toInt(&ok); // 3 version
    if(!ok)
      throw Exception(tr("Invalid FMS file. Cannot read version number: %1").arg(fmsFile.fileName()));

    if(version == 3)
    {
      v11Format = false;
      minListSize = 5;
      fieldOffset = 0;
    }
    else if(version == 1100)
    {
      v11Format = true;
      minListSize = 6;
      fieldOffset = 1;
    }
    else
      throw Exception(tr("Invalid FMS file. Invalid version %2: %1").arg(fmsFile.fileName()).arg(version));

    float maxAlt = std::numeric_limits<float>::min();

    while(!stream.atEnd())
    {
      QString line = stream.readLine().simplified();
      if(line.size() > 4)
      {
        if(line.startsWith("0 ----")) // End of file indicator
          break;

        QList<QString> list = line.split(" ");

        QString airway;
        if(v11Format)
        {
          // Read keywords from version 11
          QString key = list.value(0);
          QString value = list.value(1);
          if(key == "CYCLE")
          {
            qInfo() << "Flight plan cycle" << value;
            continue;
          }
          else if(key == "DEPRWY")
          {
            properties.insert(SIDAPPRRW, value.mid(2));
            continue;
          }
          else if(key == "SID")
          {
            properties.insert(SIDAPPR, value);
            continue;
          }
          else if(key == "SIDTRANS")
          {
            properties.insert(SIDTRANS, value);
            continue;
          }
          else if(key == "STAR")
          {
            properties.insert(STAR, value);
            continue;
          }
          else if(key == "STARTRANS")
          {
            properties.insert(STARTRANS, value);
            continue;
          }
          else if(key == "APP")
          {
            properties.insert(APPROACH_ARINC, value);
            continue;
          }
          else if(key == "APPTRANS")
          {
            properties.insert(TRANSITION, value);
            continue;
          }
          else if(key == "DESRWY")
          {
            properties.insert(APPROACHRW, value.mid(2));
            continue;
          }
          else if(key == "ADES" || key == "DES" || key == "ADEP" || key == "DEP" || key == "NUMENR")
            // Ignored keywords
            continue;

          // Airway column
          QString col2 = list.value(2);
          if(!col2.isEmpty() && col2 != "DRCT" && col2 != "ADEP" && col2 != "DEP" && col2 != "ADES" && col2 != "DES")
            airway = col2;
        }

        if(list.size() >= minListSize)
        {
          float altitude = list.at(2 + fieldOffset).toFloat();
          if(altitude > std::numeric_limits<int>::max() / 2)
            // Avoid excessive altitudes
            altitude = 0.f;

          atools::geo::Pos position(list.at(4 + fieldOffset).toFloat(), list.at(3 + fieldOffset).toFloat(), altitude);
          if(!position.isValid() || position.isNull())
            break;

          FlightplanEntry entry;
          const QString& ident = list.at(1);

          maxAlt = std::max(maxAlt, altitude);

          entry.setPosition(position);

          int type = list.at(0).toInt();
          switch(type)
          {
            case 1: // - Airport ICAO
              entry.setWaypointType(atools::fs::pln::entry::AIRPORT);
              break;

            case 2: // - NDB
              entry.setWaypointType(atools::fs::pln::entry::NDB);
              break;

            case 3: // - VOR
              entry.setWaypointType(atools::fs::pln::entry::VOR);
              break;

            case 11: // - Fix
              entry.setWaypointType(atools::fs::pln::entry::INTERSECTION);
              break;

            case 28: // - Lat/Lon Position
            case 13: // - Lat/Lon Position
              entry.setWaypointType(atools::fs::pln::entry::USER);
              break;
          }

          entry.setIcaoIdent(ident);
          entry.setWaypointId(ident);
          entry.setAirway(airway);

          entries.append(entry);
        }
        else
          throw Exception(tr("Invalid FMS file. Number of sections is not %2: %1").
                          arg(fmsFile.fileName()).arg(minListSize));
      }
    }
    fmsFile.close();

    flightplanType = IFR;
    routeType = DIRECT;
    cruisingAlt = atools::roundToInt(maxAlt > 0.f ? maxAlt : 0.f); // Use value from GUI
    adjustDepartureAndDestination();
    assignAltitudeToAllEntries(cruisingAlt);

    fileFormat = v11Format ? FMS11 : FMS3;
  }
}

void Flightplan::loadFs9(const QString& file)
{
  qDebug() << Q_FUNC_INFO;
  // [flightplan]
  // AppVersion=9.1.40901
  // title=EGPB to EISG
  // description=EGPB, EISG
  // type=IFR
  // routetype=1
  // cruising_altitude=22000
  // departure_id=EGPB, N59* 52.88', W001* 17.63', +000020.00
  // departure_position=1
  // destination_id=EISG, N54* 16.82', W008* 35.95', +000011.00
  // departure_name=SUMBURGH
  // destination_name=SLIGO
  // alternate_name=
  // waypoint.0=, EGPB, , EGPB, A, N59* 52.88', W001* 17.63', +000020.00,
  // waypoint.1=KK, WIK, , WIK, V, N58* 27.53', W003* 06.02', +000000.00,J555
  // waypoint.2=KK, ADN, , ADN, V, N57* 18.63', W002* 16.03', +000000.00,J555
  // ...
  // waypoint.11=KK, RUBEX, , RUBEX, I, N55* 19.20', W006* 56.12', +000000.00,AXXX
  // waypoint.12=, EISG, , EISG, A, N54* 16.82', W008* 35.95', +000011.00,

  // [flightplan]
  // title=EGPD to EGHI
  // description=EGPD, EGHI
  // type=IFR
  // routetype=1
  // cruising_altitude=24000
  // departure_id=EGPD, N57* 12.25', W2* 12.02', +000216.54
  // departure_position=GATE 5
  // destination_id=EGHI, N50* 57.01', W1* 21.41', +000042.65
  // departure_name=Dyce
  // destination_name=Southampton Intl
  // waypoint.0=EGPD, A, N57* 12.25', W2* 12.02', +000216.54,
  // waypoint.1=D205T, I, N56* 59.87', W2* 28.54', +000000.00,
  // waypoint.2=LUK, V, N56* 22.37', W2* 51.82', +000000.00,

  filename = file;

  QFile plnFile(filename);

  if(plnFile.open(QIODevice::ReadOnly))
  {
    QTextStream stream(&plnFile);
    while(!stream.atEnd())
    {
      QString line = stream.readLine().simplified();
      if(!line.isEmpty() && !line.startsWith("[flightplan]"))
      {
        QString key = line.section('=', 0, 0).toLower().trimmed();
        QString value = line.section('=', 1, 1).trimmed();

        if(key == "appversion")
        {
          appVersionMajor = value.section('.', 0, 1).trimmed();
          appVersionBuild = value.section('.', 2, 2).trimmed();
        }
        else if(key == "title")
          title = value;
        else if(key == "description")
          description = value;
        else if(key == "type")
          // type=IFR
          flightplanType = stringFlightplanType(value);
        else if(key == "routetype")
          // routetype=1
          routeType = stringToRouteTypeFs9(value);
        else if(key == "cruising_altitude")
          cruisingAlt = value.toInt();
        else if(key == "departure_id")
        {
          // departure_id=EGPB, N59* 52.88', W001* 17.63', +000020.00
          departureIdent = value.section(',', 0, 0).trimmed();
          departurePos = atools::geo::Pos(value.section(',', 1, 3).trimmed());
        }
        else if(key == "departure_position")
          // departure_position=1
          departureParkingName = value;
        else if(key == "departure_name")
          // departure_name=SUMBURGH
          departureAiportName = value;
        else if(key == "destination_id")
        {
          // destination_id=EISG, N54* 16.82', W008* 35.95', +000011.00
          destinationIdent = value.section(',', 0, 0).trimmed();
          destinationPos = atools::geo::Pos(value.section(',', 1, 3).trimmed());
        }
        else if(key == "destination_name")
          // destination_name=SLIGO
          destinationAiportName = value;
        else if(key.startsWith("waypoint."))
        {
          FlightplanEntry entry;

          atools::geo::Pos pos(value.section(',', 5, 7).trimmed(), false);
          if(pos.isValid())
          {
            // waypoint.0=   , EGPB, , EGPB, A, N59* 52.88', W001* 17.63', +000020.00,
            // waypoint.1= KK, WIK , , WIK , V, N58* 27.53', W003* 06.02', +000000.00,
            // ----------- 0   1    2  3     4  5            6             7          8
            entry.setIcaoRegion(value.section(',', 0, 0).trimmed());
            entry.setIcaoIdent(value.section(',', 1, 1).trimmed());
            // ignore airport name at 2
            entry.setWaypointId(value.section(',', 3, 3).trimmed());
            entry.setWaypointType(value.section(',', 4, 4).trimmed());
            entry.setPosition(pos);
            entry.setAirway(value.section(',', 8, 8).trimmed());
          }
          else
          {
            // waypoint.1= D205T, I, N56* 59.87', W2* 28.54', +000000.00,
            // ----------- 0      1  2            3           4           5
            pos = atools::geo::Pos(value.section(',', 2, 4).trimmed(), false);

            if(pos.isValid())
            {
              // No region
              entry.setIcaoIdent(value.section(',', 0, 0).trimmed());
              // ignore airport name at 2
              entry.setWaypointId(value.section(',', 0, 0).trimmed());
              entry.setWaypointType(value.section(',', 1, 1).trimmed());
              entry.setPosition(pos);
              entry.setAirway(value.section(',', 5, 5).trimmed());
            }
            else
              throw Exception(tr("Invalid flight plan file \"%1\".").arg(file));
          }

          entries.append(entry);
        }
        // else if(key == "alternate_name") ignore
      }
    }
    plnFile.close();
    fileFormat = PLN_FS9;
  }
}

void Flightplan::loadFsx(const QString& file)
{
  qDebug() << Q_FUNC_INFO;

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

    xmlFile.close();
    fileFormat = PLN_FSX;

    if(!isEmpty())
    {
      // Clear airway of departure airport to avoid problems from third party tools
      // like PFPX that abuse the airway name to add approach procedures
      entries.first().setAirway(QString());

      if(entries.size() > 1)
      {
        // Clear airway to first waypoint
        entries[1].setAirway(QString());

        if(entries.last().getWaypointType() == entry::AIRPORT)
          // Clear airway to destination
          entries.last().setAirway(QString());
      }
    }
  }
  else
    throw Exception(tr("Cannot open file \"%1\". Reason: %2").arg(file).arg(xmlFile.errorString()));
}

void Flightplan::save(const QString& file, const QString& airacCycle, bool clean)
{
  switch(fileFormat)
  {
    case atools::fs::pln::NONE:
    case atools::fs::pln::PLN_FSX:
    case atools::fs::pln::PLN_FS9:
      saveFsx(file, clean);
      break;

    case atools::fs::pln::FMS3:
      saveFms(file, airacCycle, false);
      break;

    case atools::fs::pln::FMS11:
      saveFms(file, airacCycle, true);
      break;

    case atools::fs::pln::FLP:
      saveFlp(file, true /* procedures */);
      break;
  }
}

void Flightplan::saveFsx(const QString& file, bool clean)
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
    writer.writeTextElement("AppVersionMajor", APPVERSION_MAJOR); // Always use fsx values
    writer.writeTextElement("AppVersionBuild", APPVERSION_BUILD);
    writer.writeEndElement(); // AppVersion

    for(const FlightplanEntry& entry : entries)
    {
      if(entry.isNoSave())
        // Do not save stuff like procedure points
        continue;

      writer.writeStartElement("ATCWaypoint");

      // Trim to max allowed length for FSX/P3D and remove any special chars otherwise FSX/P3D will ignore the plan
      QString name = entry.getWaypointId();
      name.replace(QRegularExpression("[^A-Za-z0-9_ ]"), "");
      name = name.left(10);
      if(name.isEmpty())
        name = "User_WP";
      writer.writeAttribute("id", name);
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
    xmlFile.close();
  }
  else
    throw Exception(tr("Cannot open PLN file %1. Reason: %2").arg(file).arg(xmlFile.errorString()));
}

void Flightplan::saveFlp(const QString& file, bool saveProcedures)
{
  filename = file;
  QFile flpFile(filename);

  if(flpFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QTextStream stream(&flpFile);
    stream.setCodec("UTF-8");

    stream << "[CoRte]" << endl;
    stream << "ArptDep=" << departureIdent << endl;
    stream << "ArptArr=" << destinationIdent << endl;

    if(saveProcedures)
    {
      // Departure - SID
      if(!properties.value(SIDAPPRRW).isEmpty())
        stream << "RwyDep=" << departureIdent << properties.value(SIDAPPRRW) << endl;
      else
        stream << "RwyDep=" << endl;

      if(!properties.value(SIDAPPR).isEmpty())
        stream << "SID=" << properties.value(SIDAPPR) << endl;
      else
        stream << "SID=" << endl;

      if(!properties.value(SIDTRANS).isEmpty())
        stream << "SID_Trans=" << properties.value(SIDTRANS) << endl;
      else
        stream << "SID_Trans=" << endl;

      // Arrival STAR
      if(!properties.value(STAR).isEmpty())
        stream << "STAR=" << properties.value(STAR) << endl;
      else
        stream << "STAR=" << endl;

      if(!properties.value(STARTRANS).isEmpty())
        stream << "STAR_Trans=" << properties.value(STARTRANS) << endl;
      else
        stream << "STAR_Trans=" << endl;

      // Arrival approach and transition
      if(!properties.value(APPROACHRW).isEmpty())
        stream << "RwyArr=" << destinationIdent << properties.value(APPROACHRW) << endl;
      else
        stream << "RwyArr=" << endl;

      if(!properties.value(APPROACH).isEmpty())
        stream << "RwyArrFinal=" << properties.value(APPROACH) << endl;
      else
        stream << "RwyArrFinal=" << endl;

      if(!properties.value(TRANSITION).isEmpty())
        stream << "APPR_Trans=" << properties.value(TRANSITION) << endl;
      else
        stream << "APPR_Trans=" << endl;
    }
    else
    {
      stream << "RwyDep=" << endl;
      stream << "RwyArr=" << endl;
      stream << "RwyArrFinal=" << endl;
      stream << "SID=" << endl;
      stream << "STAR=" << endl;
      stream << "APPR_Trans=" << endl;
    }

    QString lastAirwayTo;
    int index = 1;
    for(int i = 1; i < entries.size() - 1; i++)
    {
      const FlightplanEntry& entry = entries.at(i);
      if(entry.isNoSave())
        // Do not save stuff like procedure points
        continue;

      const FlightplanEntry& next = entries.at(i + 1);
      QString coords = QString("%1,%2").
                       arg(entry.getPosition().getLatY(), 0, 'f', 6).
                       arg(entry.getPosition().getLonX(), 0, 'f', 6);

      if(!next.getAirway().isEmpty())
      {
        stream << "Airway" << index << "=" << next.getAirway() << endl;
        stream << "Airway" << index << "FROM=" << entry.getWaypointId() << endl;
        stream << "Airway" << index << "TO=" << next.getWaypointId() << endl;
        lastAirwayTo = next.getWaypointId();
        index++;
      }
      else if(entry.getWaypointId() != lastAirwayTo)
      {
        if(entry.getWaypointType() == atools::fs::pln::entry::USER)
          // Use parseable coordinate format instead of waypoint name
          stream << "DctWpt" << index << "=" << atools::fs::util::toDegMinFormat(entry.getPosition()) << endl;
        else
          stream << "DctWpt" << index << "=" << entry.getWaypointId() << endl;

        stream << "DctWpt" << index << "Coordinates=" << coords << endl;
        lastAirwayTo.clear();
        index++;
      }
    }

    flpFile.close();
  }
  else
    throw Exception(tr("Cannot open FLP file %1. Reason: %2").arg(file).arg(flpFile.errorString()));
}

void Flightplan::saveGpx(const QString& file, const geo::LineString& track, const QVector<quint32>& timestamps,
                         int cruiseAltFt)
{
  filename = file;

  QFile gpxFile(filename);

  if(gpxFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QXmlStreamWriter writer(&gpxFile);
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
    for(int i = 0; i < entries.size(); i++)
    {
      const FlightplanEntry& entry = entries.at(i);
      // <rtept lat="52.0" lon="13.5">
      // <ele>33.0</ele>
      // <time>2011-12-13T23:59:59Z</time>
      // <name>rtept 1</name>
      // </rtept>

      if(i > 0)
      {
        // Skip equal points from procedures
        const FlightplanEntry& prev = entries.at(i - 1);
        if(entry.getIcaoIdent() == prev.getIcaoIdent() &&
           entry.getIcaoRegion() == prev.getIcaoRegion() &&
           entry.getPosition().almostEqual(prev.getPosition(), atools::geo::Pos::POS_EPSILON_100M)
           )
          continue;
      }

      writer.writeStartElement("rtept");
      writer.writeAttribute("lat", QString::number(entry.getPosition().getLatY(), 'f', 6));
      writer.writeAttribute("lon", QString::number(entry.getPosition().getLonX(), 'f', 6));

      if(i > 0 && i < entries.size() - 1)
        writer.writeTextElement("ele", QString::number(atools::geo::feetToMeter(cruiseAltFt)));
      else
        writer.writeTextElement("ele", QString::number(atools::geo::feetToMeter(entry.getPosition().getAltitude())));

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

    gpxFile.close();
  }
  else
    throw Exception(tr("Cannot open PLN file %1. Reason: %2").arg(file).arg(gpxFile.errorString()));
}

// I
// 1100 Version
// CYCLE 1710
// ADEP KCUB
// DEPRWY RW13
// ADES KRDU
// DESRWY RW05L
// APP I05L
// NUMENR 9
// 1 KCUB ADEP 0.000000 33.970470 -80.995247
// 3 CTF DRCT 0.000000 34.650497 -80.274918
// 11 NOMOE V155 0.000000 34.880920 -79.996437
// 11 LILLS V155 0.000000 34.935440 -79.930206
// 3 SDZ V155 0.000000 35.215481 -79.587936
// 11 OCHOC V155 0.000000 35.402336 -79.361153
// 11 MOATS V155 0.000000 35.621601 -79.092964
// 3 RDU V155 0.000000 35.872520 -78.783340
// 1 KRDU ADES 435.000000 35.877640 -78.787476
void Flightplan::saveFms(const QString& file, const QString& airacCycle, bool version11Format)
{
  filename = file;
  QFile fmsFile(filename);
  int numEntries = numEntriesSave();

  if(fmsFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QTextStream stream(&fmsFile);
    stream.setCodec("UTF-8");

    // OS
     #if defined(Q_OS_MACOS)
    stream << "A" << endl;
     #else
    stream << "I" << endl;
     #endif

    // File version
    if(version11Format)
    {
      // New X-Plane 11 format
      stream << "1100 Version" << endl;
      stream << "CYCLE " << airacCycle << endl;

      // Departure
      if(entries.first().getWaypointType() == entry::AIRPORT)
        stream << "ADEP " << getDepartureIdent() << endl;
      else
        stream << "DEP " << getDepartureIdent() << endl;

      // Departure - SID
      if(!properties.value(SIDAPPRRW).isEmpty())
        stream << "DEPRWY RW" << properties.value(SIDAPPRRW) << endl;

      if(!properties.value(SIDAPPR).isEmpty())
        stream << "SID " << properties.value(SIDAPPR) << endl;

      if(!properties.value(SIDTRANS).isEmpty())
        stream << "SIDTRANS " << properties.value(SIDTRANS) << endl;

      // Destination
      if(entries.last().getWaypointType() == entry::AIRPORT)
        stream << "ADES " << getDestinationIdent() << endl;
      else
        stream << "DES " << getDestinationIdent() << endl;

      // Arrival approach and transition
      if(!properties.value(APPROACHRW).isEmpty())
        stream << "DESRWY RW" << properties.value(APPROACHRW) << endl;

      // Arrival STAR
      if(!properties.value(STAR).isEmpty())
        stream << "STAR " << properties.value(STAR) << endl;

      if(!properties.value(STARTRANS).isEmpty())
        stream << "STARTRANS " << properties.value(STARTRANS) << endl;

      // Approach
      if(!properties.value(APPROACH_ARINC).isEmpty())
        stream << "APP " << properties.value(APPROACH_ARINC) << endl;

      if(!properties.value(TRANSITION).isEmpty())
        stream << "APPTRANS " << properties.value(TRANSITION) << endl;

      // Number of waypoints
      stream << "NUMENR " << numEntries << endl;
    }
    else
    {
      stream << "3 version" << endl;
      stream << "1" << endl;
      stream << (numEntries - 1) << endl; // Number of waypoints
    }

    int index = 0;
    for(const FlightplanEntry& entry : entries)
    {
      if(entry.isNoSave())
        continue;

      // 1 - Airport ICAO
      // 2 - NDB
      // 3 - VOR
      // 11 - Fix
      // 28 - Lat/Lon Position

      if(entry.getWaypointType() == atools::fs::pln::entry::USER ||
         entry.getWaypointType() == atools::fs::pln::entry::UNKNOWN)
      {
        stream << "28 ";

        // Replace spaces
        QString name = entry.getWaypointId();
        name.replace(QRegularExpression("[\\s]"), "_");

        stream << name << " ";

        // Disabled user waypoints as coordinates
        // +12.345_+009.459 Correct for a waypoint at 12.345°/0.459°.
        // -28.478_-056.370 Correct for a waypoint at -28.478°/-56.370°.
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

      if(version11Format)
      {
        if(index == 0)
          stream << (entry.getWaypointType() == entry::AIRPORT ? "ADEP " : "DEP ");
        else if(index == numEntries - 1)
          stream << (entry.getWaypointType() == entry::AIRPORT ? "ADES " : "DES ");
        else
          stream << (entry.getAirway().isEmpty() ? "DRCT " : entry.getAirway() + " ");
      }

      float alt = getCruisingAltitude();
      if(index == 0 || index >= numEntries - 1)
        alt = 0.f;

      stream << QString::number(alt, 'f', 6) << " "
             << QString::number(entry.getPosition().getLatY(), 'f', 6)
             << " "
             << QString::number(entry.getPosition().getLonX(), 'f', 6)
             << endl;

      index++;
    }

    fmsFile.close();
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
    stream.setCodec("UTF-8");

    stream << tr("PMDG RTE Created by %1 Version %2 (revision %3) on %4 ").
      arg(QApplication::applicationName()).
      arg(QApplication::applicationVersion()).
      arg(atools::gitRevision()).
      arg(QDateTime::currentDateTime().toString(Qt::ISODate)).
      replace("-", " ") << endl << endl;

    stream << numEntriesSave() << endl << endl;

    stream << departureIdent << endl << AIRPORT << endl << "DIRECT" << endl;
    posToRte(stream, entries.first().getPosition(), true);
    stream << endl << NO_DATA_STR << endl
           << 1 /* Departure*/ << endl << 0 /* Runway position */ << endl << endl;

    stream << CLIMB << endl; // Restriction phase climb
    stream << atools::roundToInt(entries.first().getPosition().getAltitude()); // Restriction altitude, if restricted

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
    rteFile.close();
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
  fileFormat = PLN_FSX;
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

  // Overwrite parking position with airport position
  departurePos = entries.first().getPosition();
  setDepartureParkingName(QString());
}

FileFormat Flightplan::getFileFormatBySuffix(const QString& file) const
{
  if(file.endsWith(".fms", Qt::CaseInsensitive))
    return FMS11;
  else if(file.endsWith(".flp", Qt::CaseInsensitive))
    return FLP;
  else
    return PLN_FSX;
}

void Flightplan::setFileFormatBySuffix(const QString& file)
{
  if(file.endsWith(".fms", Qt::CaseInsensitive))
    fileFormat = FMS11;
  else if(file.endsWith(".flp", Qt::CaseInsensitive))
    fileFormat = FLP;
  // else leave as is
}

bool Flightplan::canSaveAltitude() const
{
  // FS9 format can be used here since it is always overwritten with FSX
  return fileFormat == PLN_FSX || fileFormat == PLN_FS9 || fileFormat == FMS11 || fileFormat == FMS3;
}

bool Flightplan::canSaveFlightplanType() const
{
  return fileFormat == PLN_FSX || fileFormat == PLN_FS9;
}

bool Flightplan::canSaveRouteType() const
{
  return fileFormat == PLN_FSX || fileFormat == PLN_FS9;
}

bool Flightplan::canSaveSpeed() const
{
  return fileFormat == PLN_FSX || fileFormat == PLN_FS9;
}

bool Flightplan::canSaveDepartureParking() const
{
  return fileFormat == PLN_FSX || fileFormat == PLN_FS9;
}

bool Flightplan::canSaveUserWaypointName() const
{
  return fileFormat == PLN_FSX || fileFormat == PLN_FS9;
}

bool Flightplan::canSaveAirways() const
{
  return fileFormat == PLN_FSX || fileFormat == PLN_FS9 || fileFormat == FLP || fileFormat == FMS11;
}

bool Flightplan::canSaveProcedures() const
{
  return fileFormat == PLN_FSX || fileFormat == PLN_FS9 || fileFormat == FLP || fileFormat == FMS11;
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

    case atools::fs::pln::UNKNOWN:
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

RouteType Flightplan::stringToRouteTypeFs9(const QString& str)
{
  int routetype = str.toInt();
  switch(routetype)
  {
    case 0:
      return DIRECT;

    case 1:
      return VOR;

    case 2:
      return LOW_ALTITUDE;

    case 3:
      return HIGH_ALTITUDE;
  }
  return DIRECT;
}

QString Flightplan::programInfo()
{
  return tr("Created by %1 Version %2 (revision %3) on %4").
         arg(QApplication::applicationName()).
         arg(QApplication::applicationVersion()).
         arg(atools::gitRevision()).
         arg(QDateTime::currentDateTime().toString(Qt::ISODate)).
         replace("-", " ");
}

QStringList Flightplan::probeFile(const QString& file)
{
  QFile testFile(file);

  QStringList lines;
  if(testFile.open(QIODevice::ReadOnly))
  {
    QTextStream stream(&testFile);
    stream.setCodec("UTF-8");
    stream.setAutoDetectUnicode(true);
    int numLines = 0;
    while(!stream.atEnd() && numLines < 4)
    {
      QString line = stream.readLine().trimmed();
      if(!line.isEmpty())
      {
        lines.append(line.toLower().simplified());
        numLines++;
      }
    }
    testFile.close();
  }
  else
    throw Exception("Error reading \"" + filename + "\": " + testFile.errorString());

  return lines;
}

void Flightplan::adjustDepartureAndDestination()
{
  if(!entries.isEmpty())
  {
    if(departureIdent.isEmpty())
      departureIdent = entries.first().getIcaoIdent();
    if(!departurePos.isValid())
      departurePos = entries.first().getPosition();

    if(destinationIdent.isEmpty())
      destinationIdent = entries.last().getIcaoIdent();

    if(!destinationPos.isValid())
      destinationPos = entries.last().getPosition();

    if(title.isEmpty())
      title = QString("%1 to %2").arg(departureIdent).arg(destinationIdent);

    if(description.isEmpty())
      description = QString("%1, %2").arg(departureIdent).arg(destinationIdent);
    // These remain empty
    // departureParkingName, departureAiportName, destinationAiportName, appVersionMajor, appVersionBuild;
  }
}

int Flightplan::numEntriesSave()
{
  int num = 0;
  for(const FlightplanEntry& entry : entries)
  {
    if(!entry.isNoSave())
      // Do not save stuff like procedure points
      num++;
  }
  return num;
}

void Flightplan::assignAltitudeToAllEntries(int altitude)
{
  for(FlightplanEntry& entry : entries)
    entry.setPosition(atools::geo::Pos(entry.getPosition().getLonX(),
                                       entry.getPosition().getLatY(), altitude));
}

} // namespace pln
} // namespace fs
} // namespace atools
