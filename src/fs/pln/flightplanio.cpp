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

#include "fs/pln/flightplanio.h"

#include "exception.h"
#include "geo/pos.h"
#include "geo/calculations.h"
#include "atools.h"
#include "fs/util/fsutil.h"
#include "geo/linestring.h"
#include "fs/util/coordinates.h"
#include "fs/pln/flightplan.h"

#include <QDataStream>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QXmlStreamReader>

using atools::geo::Pos;

namespace atools {
namespace fs {
namespace pln {

static const QStringList ACCEPTED_EXTENSIONS({"fms", "flp", "pln"});

static const QRegularExpression FLP_DCT_WPT("DctWpt(\\d+)(Coordinates)?", QRegularExpression::CaseInsensitiveOption);
static const QRegularExpression FLP_DCT_AWY("Airway(\\d+)(FROM|TO)?", QRegularExpression::CaseInsensitiveOption);

static const QRegularExpression FS9_MATCH(
  "(^appversion\\s*=|^title\\s*=|^description\\s*=|^type\\s*=|"
  "^routetype\\s*=|^cruising_altitude\\s*=|^departure_id\\s*=|^destination_id\\s*=)");

/* Format structs for the Majestic Software MJC8 Q400.
 * Structs need to be packed to avoid padding. */
namespace fpr {

const static qint32 NavSystem_UNITYPE_WPT = 1;
const static qint32 NavSystem_UNITYPE_NAV = 2;
const static qint32 NavSystem_UNITYPE_AIRPORT = 3;

const static qint32 NavSystem_PROCTYPE_FP = 4; // Usual FP (fix to fix, great circle)

const static qint32 NavSystem_LEG_SEGMENT_FP_CRS = 0x05;

const static qint32 FPR_FILE_SIZE = 36089;

#pragma pack(push, 1)

struct Waypoint
{
  char designator[10]; // short name

  char fullName[12]; // full name
  double latYRad;
  double lonXRad;
  qint8 waypointType; // of type NavSystem_UNITYPE_NONE
  qint32 databaseId; // -1 if unknown
  qint8 boolVal;
  double magvarRad;
};

struct Leg
{
  qint8 proc_type; // of type NavSystem_PROCTYPE_NONE. SID STAR and so on

  qint32 legHash; // unique hash for this leg
  char legType[3]; // FA and so on...
  char legSegType; // Transition type of type FMC_LEG_SEGMENT_XXX
  qint8 iapTypeIAP_type; // type for IAP. A = GPS (ILS), D = ILSDME, N=NDB, Q=?, S=?, I=ILS, L=?. BIG MISTERY!
  char transition[6]; // this is a runway leg also, otherwise transition name
  Waypoint waypoint;

  quint8 fill[210];
};

static const qint32 FMC_FLIGHT_PLAN_MAX_LEGS = 128;

struct FprPlan
{
  qint16 numLegs;

  Leg legs[FMC_FLIGHT_PLAN_MAX_LEGS];
  quint8 fill[503];
};

#pragma pack(pop)

} // namespace fpr

FlightplanIO::FlightplanIO()
{

}

FlightplanIO::~FlightplanIO()
{

}

void FlightplanIO::load(atools::fs::pln::Flightplan& plan, const QString& file)
{
  // Get first four non empty lines - always returns a list of four
  QStringList lines = probeFile4(file);

  if(lines.isEmpty())
    throw Exception(tr("Cannot open empty flight plan file \"%1\".").arg(file));

  if(lines.first().startsWith("[corte]"))
    // FLP: [CoRte]
    loadFlp(plan, file);
  else if(lines.at(0).startsWith("<?xml version") && lines.at(1).startsWith("<simbase.document"))
    // FSX PLN <?xml version
    loadFsx(plan, file);
  else if(lines.at(0).startsWith("[flightplan]") && FS9_MATCH.match(lines.at(1)).hasMatch())
    // FS9 ini format
    loadFs9(plan, file);
  else if(lines.at(0).startsWith("[fscfp]"))
    // FSC ini format
    loadFsc(plan, file);
  else if((lines.at(0) == "i" || lines.at(0) == "a") &&
          lines.at(1).startsWith("3 version") &&
          lines.at(2).at(0).isDigit() &&
          lines.at(3).at(0).isDigit())
    // Old format
    // I
    // 3 version
    // 1
    // 4
    loadFms(plan, file);
  else if((lines.at(0) == "i" || lines.at(0) == "a") &&
          lines.at(1).startsWith("1100 version") &&
          lines.at(2).startsWith("cycle"))
    // New v11 format
    // I
    // 1100 Version
    // CYCLE 1710
    loadFms(plan, file);
  else
    throw Exception(tr("Cannot open flight plan file \"%1\". No supported flight plan format detected. "
                       "Only PLN (FSX XML, FS9 INI and FSC), FMS and FLP are supported.").arg(file));

}

void FlightplanIO::loadFlp(atools::fs::pln::Flightplan& plan, const QString& file)
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
          plan.departureIdent = value;
          departure.setIcaoIdent(value);
          departure.setWaypointType(atools::fs::pln::entry::AIRPORT);
        }
        else if(key == "arptarr")
        {
          plan.destinationIdent = value;
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
              plan.entries.append(entry);
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
            entry.setPosition(Pos(value.section(',', 1, 1).toFloat(),
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
              plan.entries.append(entry);
              entry = FlightplanEntry();
            }
            wptNum = num;
          }

          if(fromTo.isEmpty())
            entry.setAirway(value);
          else if(fromTo.toLower() == "from")
          {
            if(plan.entries.isEmpty() || plan.entries.last().getIcaoIdent() != value)
            {
              FlightplanEntry from;
              from.setIcaoIdent(value);
              from.setWaypointId(value);
              plan.entries.append(from);
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
            plan.properties.insert(SIDAPPRRW, value.mid(plan.departureIdent.size()));
          else if(key == "sid")
            plan.properties.insert(SIDAPPR, value);
          else if(key == "sid_trans")
            plan.properties.insert(SIDTRANS, value);
          else if(key == STAR)
            plan.properties.insert(STAR, value);
          else if(key == "star_trans")
            plan.properties.insert(STARTRANS, value);
          else if(key == "rwyarr")
            plan.properties.insert(APPROACHRW, value.mid(plan.destinationIdent.size()));
          else if(key == "rwyarrfinal")
            plan.properties.insert(APPROACH, value);
          else if(key == "appr_trans")
            plan.properties.insert(TRANSITION, value);
        }
      }
    }
    plan.entries.append(entry);
    plan.entries.prepend(departure);
    plan.entries.append(destination);

    flpFile.close();
    plan.flightplanType = IFR;
    plan.routeType = UNKNOWN; // Determine type when resolving navaids from the database
    plan.cruisingAlt = 0.f; // Use either GUI value or calculate from airways

    plan.fileFormat = FLP;
    adjustDepartureAndDestination(plan);
  }
}

void FlightplanIO::loadFms(atools::fs::pln::Flightplan& plan, const QString& file)
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
    QString destinationRwy;

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
            plan.properties.insert(SIDAPPRRW, value.mid(2));
            continue;
          }
          else if(key == "SID")
          {
            plan.properties.insert(SIDAPPR, value);
            continue;
          }
          else if(key == "SIDTRANS")
          {
            plan.properties.insert(SIDTRANS, value);
            continue;
          }
          else if(key == "STAR")
          {
            plan.properties.insert(STAR, value);
            continue;
          }
          else if(key == "STARTRANS")
          {
            plan.properties.insert(STARTRANS, value);
            continue;
          }
          else if(key == "APP")
          {
            plan.properties.insert(APPROACH_ARINC, value);
            continue;
          }
          else if(key == "APPTRANS")
          {
            plan.properties.insert(TRANSITION, value);
            continue;
          }
          else if(key == "DESRWY")
          {
            destinationRwy = value.mid(2);
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

          Pos position(list.at(4 + fieldOffset).toFloat(), list.at(3 + fieldOffset).toFloat(), altitude);
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

          plan.entries.append(entry);
        }
        else
          throw Exception(tr("Invalid FMS file. Number of sections is not %2: %1").
                          arg(fmsFile.fileName()).arg(minListSize));
      }
    }
    fmsFile.close();

    if(!destinationRwy.isEmpty())
    {
      if(plan.properties.contains(APPROACH))
        plan.properties.insert(APPROACHRW, destinationRwy);
      else if(plan.properties.contains(STAR))
        plan.properties.insert(STARRW, destinationRwy);
    }

    plan.flightplanType = IFR;
    plan.routeType = DIRECT;
    plan.cruisingAlt = atools::roundToInt(maxAlt > 0.f ? maxAlt : 0.f); // Use value from GUI
    adjustDepartureAndDestination(plan);
    assignAltitudeToAllEntries(plan);

    plan.fileFormat = v11Format ? FMS11 : FMS3;
  }
}

void FlightplanIO::loadFsc(atools::fs::pln::Flightplan& plan, const QString& file)
{
  qDebug() << Q_FUNC_INFO;
  // [FSCFP]
  // DepartAPCode=LIDT
  // DepartNum=0
  // DepartID=18
  // DepartType=Rwy
  // DestAPCode=LSZS
  // RouteType=0
  // SID=
  // STAR=
  // Transition=
  // WP=1,Fix,80VOR,TNT347008,46.15109,11.10340,0.00,353.2240,2.818,1,0,0,,0,0,61831
  // ...
  // WP=24,Int,RONAG,RONAG,46.77942,10.25900,0.00,252.4302,2.587,1,0,0,,0,0,22797
  // WP=25,Int,ARDED,ARDED,46.73528,10.12778,0.00,243.8965,2.587,1,0,0,,0,0,22725

  filename = file;

  QFile plnFile(filename);

  if(plnFile.open(QIODevice::ReadOnly))
  {
    QTextStream stream(&plnFile);
    FlightplanEntry departure, destination;

    while(!stream.atEnd())
    {
      QString line = stream.readLine().simplified();
      if(!line.isEmpty())
      {
        QString key = line.section('=', 0, 0).toLower().trimmed();
        QStringList values = line.section('=', 1).simplified().split(",");

        if(values.isEmpty())
          continue;

        if(key == "departapcode")
        {
          departure.setIcaoIdent(values.first());
          departure.setWaypointId(values.first());
          departure.setWaypointType(atools::fs::pln::entry::AIRPORT);
        }
        else if(key == "destapcode")
        {
          destination.setIcaoIdent(values.first());
          destination.setWaypointId(values.first());
          destination.setWaypointType(atools::fs::pln::entry::AIRPORT);
        }
        // Ignored keys
        // else if(key == "DepartNum")
        // else if(key == "DepartID")
        // else if(key == "DepartType")
        // else if(key == "SID")
        // else if(key == "STAR")
        // else if(key == "Transition")
        else if(key == "wp")
        {
          QString type = values.value(1).toLower();
          QString ident = values.value(2);
          QString name = values.value(3);
          QString airway = values.value(16);

          FlightplanEntry entry;
          entry.setIcaoIdent(ident);
          entry.setWaypointId(ident);
          entry.setName(name);
          entry.setAirway(airway);

          float latY = values.value(4).toFloat();
          float lonX = values.value(5).toFloat();
          entry.setPosition(Pos(lonX, latY));

          if(type == "fix" || type == "int")
            entry.setWaypointType(atools::fs::pln::entry::INTERSECTION);
          else if(type == "uwp" || !atools::fs::util::isValidIdent(ident))
            entry.setWaypointType(atools::fs::pln::entry::USER);

          plan.entries.append(entry);
        }
      }
    }

    plan.entries.prepend(departure);
    plan.entries.append(destination);

    plnFile.close();

    plan.flightplanType = IFR;
    plan.routeType = UNKNOWN; // Determine type when resolving navaids from the database
    plan.cruisingAlt = 0.f; // Use either GUI value or calculate from airways
    plan.fileFormat = PLN_FSC;
    adjustDepartureAndDestination(plan);
  }
}

void FlightplanIO::loadFs9(atools::fs::pln::Flightplan& plan, const QString& file)
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
          plan.appVersionMajor = value.section('.', 0, 1).trimmed();
          plan.appVersionBuild = value.section('.', 2, 2).trimmed();
        }
        else if(key == "title")
          plan.title = value;
        else if(key == "description")
          plan.description = value;
        else if(key == "type")
          // type=IFR
          plan.flightplanType = stringFlightplanType(value);
        else if(key == "routetype")
          // routetype=1
          plan.routeType = stringToRouteTypeFs9(value);
        else if(key == "cruising_altitude")
          plan.cruisingAlt = value.toInt();
        else if(key == "departure_id")
        {
          // departure_id=EGPB, N59* 52.88', W001* 17.63', +000020.00
          plan.departureIdent = value.section(',', 0, 0).trimmed();
          plan.departurePos = Pos(value.section(',', 1, 3).trimmed());
        }
        else if(key == "departure_position")
          // departure_position=1
          plan.departureParkingName = value;
        else if(key == "departure_name")
          // departure_name=SUMBURGH
          plan.departureAiportName = value;
        else if(key == "destination_id")
        {
          // destination_id=EISG, N54* 16.82', W008* 35.95', +000011.00
          plan.destinationIdent = value.section(',', 0, 0).trimmed();
          plan.destinationPos = Pos(value.section(',', 1, 3).trimmed());
        }
        else if(key == "destination_name")
          // destination_name=SLIGO
          plan.destinationAiportName = value;
        else if(key.startsWith("waypoint."))
        {
          FlightplanEntry entry;

          Pos pos(value.section(',', 5, 7).trimmed(), false);
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
            pos = Pos(value.section(',', 2, 4).trimmed(), false);

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

          plan.entries.append(entry);
        }
        // else if(key == "alternate_name") ignore
      }
    }
    plnFile.close();
    plan.fileFormat = PLN_FS9;
  }
}

void FlightplanIO::loadFsx(atools::fs::pln::Flightplan& plan, const QString& file)
{
  qDebug() << Q_FUNC_INFO;

  filename = file;

  QFile xmlFile(filename);

  if(xmlFile.open(QIODevice::ReadOnly))
  {
    plan.entries.clear();
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
            plan.properties.insert(prop.section("=", 0, 0).trimmed(), prop.section("=", 1, 1).trimmed());
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
        plan.title = reader.readElementText();
      else if(name == "FPType")
        plan.flightplanType = stringFlightplanType(reader.readElementText());
      else if(name == "RouteType")
        plan.routeType = stringToRouteType(reader.readElementText());
      else if(name == "CruisingAlt")
        plan.cruisingAlt = reader.readElementText().toInt();
      else if(name == "DepartureID")
        plan.departureIdent = reader.readElementText();
      else if(name == "DepartureLLA")
      {
        QString txt = reader.readElementText();
        if(!txt.isEmpty())
          plan.departurePos = geo::Pos(txt);
      }
      else if(name == "DestinationID")
        plan.destinationIdent = reader.readElementText();
      else if(name == "DestinationLLA")
      {
        QString txt = reader.readElementText();
        if(!txt.isEmpty())
          plan.destinationPos = geo::Pos(txt);
      }
      else if(name == "Descr")
        plan.description = reader.readElementText();
      else if(name == "DeparturePosition")
        plan.departureParkingName = reader.readElementText();
      else if(name == "DepartureName")
        plan.departureAiportName = reader.readElementText();
      else if(name == "DestinationName")
        plan.destinationAiportName = reader.readElementText();
      else if(name == "AppVersion")
        readAppVersion(plan, reader);
      else if(name == "ATCWaypoint")
        readWaypoint(plan, reader);
      else
        reader.skipCurrentElement();
    }

    xmlFile.close();
    plan.fileFormat = PLN_FSX;

    if(!plan.isEmpty())
    {
      // Clear airway of departure airport to avoid problems from third party tools
      // like PFPX that abuse the airway name to add approach procedures
      plan.entries.first().setAirway(QString());

      if(plan.entries.size() > 1)
      {
        // Clear airway to first waypoint
        plan.entries[1].setAirway(QString());

        if(plan.entries.last().getWaypointType() == entry::AIRPORT)
          // Clear airway to destination
          plan.entries.last().setAirway(QString());
      }
    }
  }
  else
    throw Exception(tr("Cannot open file \"%1\". Reason: %2").arg(file).arg(xmlFile.errorString()));
}

void FlightplanIO::save(const atools::fs::pln::Flightplan& flightplan, const QString& file, const QString& airacCycle,
                        SaveOptions options)
{
  switch(flightplan.fileFormat)
  {
    case atools::fs::pln::NONE:
    case atools::fs::pln::PLN_FSX:
    // FS9 and FSC formats are read only and will be saved as FSX
    case atools::fs::pln::PLN_FS9:
    case atools::fs::pln::PLN_FSC:
      saveFsx(flightplan, file, options);
      break;

    case atools::fs::pln::FMS3:
      saveFms(flightplan, file, airacCycle, false /* FMS 11 */);
      break;

    case atools::fs::pln::FMS11:
      saveFms(flightplan, file, airacCycle, true /* FMS 11 */);
      break;

    case atools::fs::pln::FLP:
      saveFlp(flightplan, file);
      break;
  }
}

void FlightplanIO::saveFsx(const Flightplan& plan, const QString& file, SaveOptions options)
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

    if(!(options & SAVE_CLEAN))
    {
      QStringList comment;
      for(const QString& key : plan.properties.keys())
      {
        if(key == "_lnm")
          continue;

        if(!key.isEmpty())
          comment.append("\n         " + key + "=" + plan.properties.value(key));
      }

      std::sort(comment.begin(), comment.end());

      comment.prepend("\n         _lnm=" + atools::programFileInfo());

      writer.writeComment(" LNMDATA" + comment.join("|") + "\n");
    }

    writer.writeStartElement("FlightPlan.FlightPlan");

    writer.writeTextElement("Title", plan.title);
    writer.writeTextElement("FPType", flightplanTypeToString(plan.flightplanType));

    if(plan.routeType != DIRECT)
      writer.writeTextElement("RouteType", routeTypeToString(plan.routeType));

    writer.writeTextElement("CruisingAlt", QString().number(plan.cruisingAlt));
    writer.writeTextElement("DepartureID", plan.departureIdent);
    writer.writeTextElement("DepartureLLA",
                            plan.departurePos.isValid() ? plan.departurePos.toLongString() : QString());
    writer.writeTextElement("DestinationID", plan.destinationIdent);
    writer.writeTextElement("DestinationLLA",
                            plan.destinationPos.isValid() ? plan.destinationPos.toLongString() : QString());
    writer.writeTextElement("Descr", plan.description);
    writer.writeTextElement("DeparturePosition", plan.departureParkingName);
    writer.writeTextElement("DepartureName", plan.departureAiportName);
    writer.writeTextElement("DestinationName", plan.destinationAiportName);

    writer.writeStartElement("AppVersion");
    writer.writeTextElement("AppVersionMajor", plan.APPVERSION_MAJOR); // Always use fsx values
    writer.writeTextElement("AppVersionBuild", plan.APPVERSION_BUILD);
    writer.writeEndElement(); // AppVersion

    for(const FlightplanEntry& entry : plan.entries)
    {
      if(entry.isNoSave())
        // Do not save stuff like procedure points
        continue;

      writer.writeStartElement("ATCWaypoint");

      // Trim to max allowed length for FSX/P3D and remove any special chars otherwise FSX/P3D will ignore the plan
      writer.writeAttribute("id", atools::fs::util::adjustFsxUserWpName(entry.getWaypointId()));
      writer.writeTextElement("ATCWaypointType", entry.getWaypointTypeAsString());

      if(!entry.getPosition().isValid())
        throw atools::Exception("Invalid position in flightplan for id " + entry.getWaypointId());

      Pos pos = entry.getPosition();

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

void FlightplanIO::saveFlp(const atools::fs::pln::Flightplan& plan, const QString& file)
{
  filename = file;
  QFile flpFile(filename);

  if(flpFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QTextStream stream(&flpFile);
    stream.setCodec("UTF-8");

    stream << "[CoRte]" << endl;
    stream << "ArptDep=" << plan.departureIdent << endl;
    stream << "ArptArr=" << plan.destinationIdent << endl;

    // Departure - SID
    if(!plan.properties.value(SIDAPPRRW).isEmpty())
      stream << "RwyDep=" << plan.departureIdent << plan.properties.value(SIDAPPRRW) << endl;
    else
      stream << "RwyDep=" << endl;

    if(!plan.properties.value(SIDAPPR).isEmpty())
      stream << "SID=" << plan.properties.value(SIDAPPR) << endl;
    else
      stream << "SID=" << endl;

    if(!plan.properties.value(SIDTRANS).isEmpty())
      stream << "SID_Trans=" << plan.properties.value(SIDTRANS) << endl;
    else
      stream << "SID_Trans=" << endl;

    // Arrival STAR
    if(!plan.properties.value(STAR).isEmpty())
      stream << "STAR=" << plan.properties.value(STAR) << endl;
    else
      stream << "STAR=" << endl;

    if(!plan.properties.value(STARTRANS).isEmpty())
      stream << "STAR_Trans=" << plan.properties.value(STARTRANS) << endl;
    else
      stream << "STAR_Trans=" << endl;

    // Arrival approach and transition
    if(!plan.properties.value(APPROACHRW).isEmpty())
      stream << "RwyArr=" << plan.destinationIdent << plan.properties.value(APPROACHRW) << endl;
    else
      stream << "RwyArr=" << endl;

    if(!plan.properties.value(APPROACH).isEmpty())
      stream << "RwyArrFinal=" << plan.properties.value(APPROACH) << endl;
    else
      stream << "RwyArrFinal=" << endl;

    if(!plan.properties.value(TRANSITION).isEmpty())
      stream << "APPR_Trans=" << plan.properties.value(TRANSITION) << endl;
    else
      stream << "APPR_Trans=" << endl;

    // stream << "RwyDep=" << endl;
    // stream << "RwyArr=" << endl;
    // stream << "RwyArrFinal=" << endl;
    // stream << "SID=" << endl;
    // stream << "STAR=" << endl;
    // stream << "APPR_Trans=" << endl;

    QString lastAirwayTo;
    int index = 1;
    for(int i = 1; i < plan.entries.size() - 1; i++)
    {
      const FlightplanEntry& entry = plan.entries.at(i);
      if(entry.isNoSave())
        // Do not save stuff like procedure points
        continue;

      const FlightplanEntry& next = plan.entries.at(i + 1);
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

void FlightplanIO::saveGpx(const atools::fs::pln::Flightplan& plan, const QString& file,
                           const geo::LineString& track,
                           const QVector<quint32>& timestamps, int cruiseAltFt)
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

    // writer.writeComment(programFileInfo());

    // <metadata>
    // <link href="http://www.garmin.com">
    // <text>Garmin International</text>
    // </link>
    // <time>2009-10-17T22:58:43Z</time>
    // </metadata>
    writer.writeStartElement("metadata");
    writer.writeStartElement("link");
    writer.writeAttribute("href", "https://albar965.github.io/littlenavmap.html");
    writer.writeTextElement("text", atools::programFileInfo());
    writer.writeEndElement(); // link
    writer.writeEndElement(); // metadata

    writer.writeStartElement("rte");

    QString descr = QString("%1 (%2) to %3 (%4) at %5 ft, %6, %7").
                    arg(plan.departureAiportName).arg(plan.departureIdent).
                    arg(plan.destinationAiportName).arg(plan.destinationIdent).
                    arg(plan.getCruisingAltitude()).
                    arg(flightplanTypeToString(plan.flightplanType)).
                    arg(routeTypeToString(plan.routeType));

    writer.writeTextElement("name", plan.title + tr(" Flight Plan"));
    writer.writeTextElement("desc", descr);

    // Write route ========================================================
    for(int i = 0; i < plan.entries.size(); i++)
    {
      const FlightplanEntry& entry = plan.entries.at(i);
      // <rtept lat="52.0" lon="13.5">
      // <ele>33.0</ele>
      // <time>2011-12-13T23:59:59Z</time>
      // <name>rtept 1</name>
      // </rtept>

      if(i > 0)
      {
        // Skip equal points from procedures
        const FlightplanEntry& prev = plan.entries.at(i - 1);
        if(entry.getIcaoIdent() == prev.getIcaoIdent() &&
           entry.getIcaoRegion() == prev.getIcaoRegion() &&
           entry.getPosition().almostEqual(prev.getPosition(), Pos::POS_EPSILON_100M)
           )
          continue;
      }

      writer.writeStartElement("rtept");
      writer.writeAttribute("lat", QString::number(entry.getPosition().getLatY(), 'f', 6));
      writer.writeAttribute("lon", QString::number(entry.getPosition().getLonX(), 'f', 6));

      if(i > 0 && i < plan.entries.size() - 1)
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
      writer.writeTextElement("name", plan.title + tr(" Track"));
      writer.writeTextElement("desc", descr);

      writer.writeStartElement("trkseg");

      for(int i = 0; i < track.size(); ++i)
      {
        const Pos& pos = track.at(i);
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
void FlightplanIO::saveFms(const atools::fs::pln::Flightplan& plan, const QString& file,
                           const QString& airacCycle, bool version11Format)
{
  filename = file;
  QFile fmsFile(filename);

  if(fmsFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    int numEntries = numEntriesSave(plan);
    QTextStream stream(&fmsFile);
    stream.setCodec("UTF-8");

    // OS
    stream << "I" << endl;

    // File version
    if(version11Format)
    {
      // New X-Plane 11 format
      stream << "1100 Version" << endl;
      stream << "CYCLE " << airacCycle << endl;

      // Departure
      if(plan.entries.first().getWaypointType() == entry::AIRPORT)
        stream << "ADEP " << plan.getDepartureIdent() << endl;
      else
        stream << "DEP " << plan.getDepartureIdent() << endl;

      // Departure - SID
      if(!plan.properties.value(SIDAPPRRW).isEmpty())
        stream << "DEPRWY RW" << plan.properties.value(SIDAPPRRW) << endl;

      if(!plan.properties.value(SIDAPPR).isEmpty())
        stream << "SID " << plan.properties.value(SIDAPPR) << endl;

      if(!plan.properties.value(SIDTRANS).isEmpty())
        stream << "SIDTRANS " << plan.properties.value(SIDTRANS) << endl;

      // Destination
      if(plan.entries.last().getWaypointType() == entry::AIRPORT)
        stream << "ADES " << plan.getDestinationIdent() << endl;
      else
        stream << "DES " << plan.getDestinationIdent() << endl;

      // Arrival runway
      if(!plan.properties.value(APPROACHRW).isEmpty())
        // Use approach runway if there is an approach
        stream << "DESRWY RW" << plan.properties.value(APPROACHRW) << endl;
      else if(!plan.properties.value(STARRW).isEmpty())
        // Use STAR runway if no approach but STAR
        stream << "DESRWY RW" << plan.properties.value(STARRW) << endl;

      // Arrival approach and transition
      // Arrival STAR
      if(!plan.properties.value(STAR).isEmpty())
        stream << "STAR " << plan.properties.value(STAR) << endl;

      if(!plan.properties.value(STARTRANS).isEmpty())
        stream << "STARTRANS " << plan.properties.value(STARTRANS) << endl;

      // Approach
      if(!plan.properties.value(APPROACH_ARINC).isEmpty())
        stream << "APP " << plan.properties.value(APPROACH_ARINC) << endl;

      if(!plan.properties.value(TRANSITION).isEmpty())
        stream << "APPTRANS " << plan.properties.value(TRANSITION) << endl;

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
    for(const FlightplanEntry& entry : plan.entries)
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

      float alt = plan.getCruisingAltitude();
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

void FlightplanIO::saveRte(const atools::fs::pln::Flightplan& plan, const QString& file)
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

    stream << numEntriesSave(plan) << endl << endl;

    stream << plan.departureIdent << endl << AIRPORT << endl << "DIRECT" << endl;
    posToRte(stream, plan.entries.first().getPosition(), true);
    stream << endl << NO_DATA_STR << endl
           << 1 /* Departure*/ << endl << 0 /* Runway position */ << endl << endl;

    stream << CLIMB << endl; // Restriction phase climb
    stream << atools::roundToInt(plan.entries.first().getPosition().getAltitude()); // Restriction altitude, if restricted

    // Restriction type, altitude and speed
    stream << endl << NO_DATA_STR << endl << NO_DATA_NUM << endl << NO_DATA_NUM << endl << endl;

    for(int i = 1; i < plan.entries.size() - 1; i++)
    {
      const FlightplanEntry& entry = plan.entries.at(i);

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

      QString nextAirway = plan.entries.at(i + 1).getAirway();
      stream << (nextAirway.isEmpty() ? "DIRECT" : nextAirway) << endl;

      posToRte(stream, entry.getPosition(), false);
      stream << endl << 0 << endl << 0 << endl << 0 << endl << endl; // Restriction fields
    }

    stream << plan.destinationIdent << endl << AIRPORT << endl << NO_DATA_STR << endl;
    posToRte(stream, plan.destinationPos, true);
    stream << endl << NO_DATA_STR << endl
           << 0 /* no departure*/ << endl << 0 /* Runway position */ << endl << endl;

    stream << CLIMB << endl; // Restriction phase
    stream << atools::roundToInt(plan.destinationPos.getAltitude()) << endl; // Restriction altitude, if restricted
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

void FlightplanIO::saveFpr(const atools::fs::pln::Flightplan& plan, const QString& file)
{
  filename = file;
  QFile fprFile(filename);

  // Create base hash from 0 to 32768 - use qt functions which also compile on mac
  qsrand(static_cast<unsigned int>(std::time(0)));
  int hashSeed = qrand() * std::numeric_limits<qint16>::max() / RAND_MAX;

  if(fprFile.open(QIODevice::WriteOnly))
  {
    QDataStream ds(&fprFile);

    fpr::FprPlan fprplan;
    memset(&fprplan, 0, sizeof(fprplan));

    int legIdx = 0;
    for(int i = 0; i < plan.entries.size(); ++i)
    {
      const FlightplanEntry& e = plan.entries.at(i);

      if(e.isNoSave())
        // Omit proceedures
        continue;

      fpr::Leg *leg = &fprplan.legs[legIdx];

      switch(e.getWaypointType())
      {
        // Omit any user and unknown waypoints
        case atools::fs::pln::entry::USER:
        case atools::fs::pln::entry::UNKNOWN:
          continue;

        case atools::fs::pln::entry::AIRPORT:
          leg->waypoint.waypointType = fpr::NavSystem_UNITYPE_AIRPORT;
          break;

        case atools::fs::pln::entry::INTERSECTION:
          leg->waypoint.waypointType = fpr::NavSystem_UNITYPE_WPT;
          break;

        case atools::fs::pln::entry::VOR:
          leg->waypoint.waypointType = fpr::NavSystem_UNITYPE_NAV;
          break;

        case atools::fs::pln::entry::NDB:
          leg->waypoint.waypointType = fpr::NavSystem_UNITYPE_NAV;
          break;
      }

      leg->proc_type = fpr::NavSystem_PROCTYPE_FP;
      leg->legHash = hashSeed + i;
      leg->legSegType = fpr::NavSystem_LEG_SEGMENT_FP_CRS;

      writeBinaryString(leg->legType, "TF", sizeof(leg->legType));
      writeBinaryString(leg->transition, QString(), sizeof(leg->transition));
      writeBinaryString(leg->waypoint.designator, e.getIcaoIdent(), sizeof(leg->waypoint.designator));
      writeBinaryString(leg->waypoint.fullName, e.getName(), sizeof(leg->waypoint.fullName));

      leg->waypoint.latYRad = atools::geo::toRadians(e.getPosition().getLatY());
      leg->waypoint.lonXRad = atools::geo::toRadians(e.getPosition().getLonX());

      // Will not show valid coordinates in the FMS but flight plan is usable anyway
      leg->waypoint.databaseId = -1;
      leg->waypoint.magvarRad = atools::geo::toRadians(e.getMagvar());

      fprplan.numLegs++;
      legIdx++;
    }

    // Write the memory block to the file
    ds.writeRawData(reinterpret_cast<const char *>(&fprplan), sizeof(fprplan));

    qint64 size = fprFile.size();
    fprFile.close();

    if(size != fpr::FPR_FILE_SIZE)
      throw Exception(tr("Internal error writing %1: File has invalid size %2 != %3 will not be usable.").
                      arg(filename).arg(size).arg(fpr::FPR_FILE_SIZE));
  }
}

void FlightplanIO::saveFltplan(const Flightplan& plan, const QString& file)
{
  // YSSY,
  // YMML,
  // ,
  // 32000,
  // ,
  // ,
  // ,
  // ,
  // ,
  // ,
  // -1,
  // ,
  // ,
  // ,
  // ,
  // -1,
  // DIRECT,3,WOL,0,-34.558056 150.791111,0,0,195.40055,0,0,1,321,0.000,0,18763,-1000,13468,457,-1,0,0,000.00000,0,0,,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1000,0,
  // H65,2,RAZZI,0,-35.054166 149.960277,0,0,220.43300,0,0,0,0,0.797,0,28908,-1000,12935,859,-1,0,0,000.00000,0,0,,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1000,0,
  // Q29,2,TANTA,0,-35.880000 148.531666,0,0,221.25749,0,0,0,0,0.793,0,32000,-1000,12355,1506,-1,0,0,000.00000,0,0,,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1000,0,
  // Q29,2,RUMIE,0,-36.329721 147.728611,0,0,222.05078,0,0,0,0,0.793,0,32000,-1000,12053,1868,-1,0,0,000.00000,0,0,,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1000,0,
  // Q29,2,NABBA,0,-36.705277 147.041944,0,0,222.94175,0,0,0,0,0.793,0,32000,-1000,11798,2174,-1,0,0,000.00000,0,0,,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1000,0,
  // Q29,2,BULLA,0,-37.077778 146.347221,0,0,223.35175,0,0,0,0,0.793,0,32000,-1000,11544,2480,-1,0,0,000.00000,0,0,,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1000,0,
  // Q29,2,TAREX,0,-37.306666 145.914999,0,0,224.05443,0,0,0,0,0.793,0,32000,-1000,11386,2670,-1,0,0,000.00000,0,0,,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1000,0,

  filename = file;
  QFile fltplanFile(filename);

  if(fltplanFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QString textString;
    QTextStream stream(&textString);
    stream.setCodec("UTF-8");

    // YSSY,
    // YMML,
    stream << plan.getDepartureIdent() << "," << endl << plan.getDestinationIdent() << "," << endl;

    // ,
    stream << "," << endl;

    // 32000,
    stream << plan.getCruisingAltitude() << "," << endl;

    // ,
    // ,
    // ,
    // ,
    // ,
    // ,
    stream << "," << endl << "," << endl << "," << endl << "," << endl << "," << endl << "," << endl;

    // -1,
    // ,
    // ,
    // ,
    // ,
    // -1,
    stream << "-1," << endl << "," << endl << "," << endl << "," << endl << "," << endl << "-1," << endl;

    for(int i = 0; i < plan.entries.size(); i++)
    {
      if(i == 0 || i == plan.entries.size() - 1)
        // Start or destination
        continue;

      const FlightplanEntry& entry = plan.entries.at(i);
      if(entry.isNoSave())
        continue;

      // DIRECT,3,WOL,0,-34.558056 150.791111,0,0,195.40055,0,0,1,321,0.000,0,18763,-1000,13468,457,-1,0,0,000.00000,0,0,,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1000,0,
      // H65,2,RAZZI,0,-35.054166 149.960277,0,0,220.43300,0,0,0,0,0.797,0,28908,-1000,12935,859,-1,0,0,000.00000,0,0,,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1000,0,
      if(entry.getAirway().isEmpty())
      {
        if(i == 1)
          stream << "DIRECT,3,";
        else
          stream << ",2,";
      }
      else
        stream << entry.getAirway() << ",2,";

      int heading = atools::roundToInt(plan.entries.at(i - 1).getPosition().angleDegToRhumb(entry.getPosition()));
      if(entry.getMagvar() < std::numeric_limits<float>::max())
        heading -= entry.getMagvar();

      // 51.578888-000.918889
      // 51.330874 000.034811
      QString latY = QString("%1").arg(std::abs(entry.getPosition().getLatY()), 9, 'f', 6);
      if(entry.getPosition().getLatY() < 0.f)
        latY.prepend("-");
      else
        latY.prepend(" ");

      QString lonX = QString("%1").arg(std::abs(entry.getPosition().getLonX()), 10, 'f', 6, '0');
      if(entry.getPosition().getLonX() < 0.f)
        lonX.prepend("-");
      else
        lonX.prepend(" ");

      stream << entry.getIcaoIdent() << ",0, ";
      stream << latY << lonX;
      stream << ",0,0," << QString("%1").arg(heading, 3, 10, QChar('0')) << ".00000";

      // Ignore rest of the fields
      stream << ",0,0,1,-1,0.000,0,-1000,-1000,-1,-1,-1,0,0,000.00000,0,0,,"
                "-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1000,0,";
      stream << endl;
    }

#ifndef Q_OS_WIN32
    // Convert EOL always to Windows (0x0a -> 0x0d0a)
    textString.replace("\n", "\r\n");
#endif

    QByteArray utf8 = textString.toUtf8();
    fltplanFile.write(utf8.data(), utf8.size());
    fltplanFile.close();
  }
  else
    throw Exception(tr("Cannot open FLTPLAN file %1. Reason: %2").arg(file).arg(fltplanFile.errorString()));
}

QString FlightplanIO::coordStringFs9(const atools::geo::Pos& pos)
{
  // N53* 37.82', E009* 59.29', +000053.00
  const static QString LONG_FORMAT("%1%2* %3', %4%5* %6', %7%8");

  return LONG_FORMAT.
         arg(pos.getLatY() > 0.f ? "N" : "S").
         arg(atools::absInt(pos.getLatYDeg()), 2, 10, QChar('0')).
         arg(std::abs(pos.getLatYMin() + pos.getLatYSec() / 60.f), 2, 'f', 0, QChar('0')).
         arg(pos.getLonX() > 0.f ? "E" : "W").
         arg(atools::absInt(pos.getLonXDeg()), 3, 10, QChar('0')).
         arg(std::abs(pos.getLonXMin() + pos.getLonXSec() / 60.f), 2, 'f', 0, QChar('0')).
         arg(pos.getAltitude() >= 0.f ? "+" : "-").
         arg(pos.getAltitude(), 6, 'f', 2, QChar('0'));
}

void FlightplanIO::saveBbsPln(const Flightplan& plan, const QString& file)
{
  filename = file;
  QFile fltplanFile(filename);

  if(fltplanFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QTextStream stream(&fltplanFile);
    stream.setCodec("UTF-8");

    // [flightplan]
    // title=EDDH to LIRF
    // description=EDDH, LIRF
    // type=IFR
    // routetype=3
    stream << "[flightplan]" << endl;
    stream << "title=" << plan.getDepartureIdent() << " to " << plan.getDestinationIdent() << endl;
    stream << "description=" << plan.getDepartureIdent() << ", " << plan.getDestinationIdent() << endl;
    stream << "type=" << flightplanTypeToString(plan.getFlightplanType()) << endl;
    stream << "routetype=" << routeTypeToStringFs9(plan.getRouteType()) << endl;

    // cruising_altitude=29000
    // departure_id=EDDH, N53* 37.82', E009* 59.29', +000053.00
    // destination_id=LIRF, N41* 48.02', E012* 14.33', +000014.00
    // departure_name=HAMBURG
    // destination_name=FIUMICINO
    stream << "cruising_altitude" << plan.getCruisingAltitude() << endl;
    stream << "departure_id=" << plan.getDepartureIdent() << ", "
           << coordStringFs9(plan.getDeparturePosition()) << endl;
    stream << "destination_id=" << plan.getDestinationIdent() << ", "
           << coordStringFs9(plan.getDestinationPosition()) << endl;
    stream << "departure_name=" << plan.getDepartureAiportName().toUpper() << endl;
    stream << "destination_name=" << plan.getDestinationAiportName().toUpper() << endl;

    // waypoint.0=EDDH, A, N53* 37.82', E009* 59.29', +000053.00,
    // waypoint.1=AMLUH, I, N53* 25.74', E010* 19.35', +000000.00,
    // ...
    // waypoint.19=RITEB, I, N42* 41.92', E012* 9.82', +000000.00, T369
    // waypoint.20=LIRF, A, N41* 48.02', E012* 14.33', +000014.00,
    int idx = 0;
    for(const FlightplanEntry& entry : plan.entries)
    {
      if(entry.isNoSave())
        // Do not save procedure points
        continue;
      stream << "waypoint." << idx << "=" << entry.getIcaoIdent() << ", ";
      stream << entry.getWaypointTypeAsStringShort() << ", ";
      stream << coordStringFs9(entry.getPosition()) << ", ";
      stream << entry.getAirway();

      stream << endl;
      idx++;
    }

    fltplanFile.close();
  }
  else
    throw Exception(tr("Cannot open PLN file %1. Reason: %2").arg(file).arg(fltplanFile.errorString()));
}

void FlightplanIO::saveGarminGns(const atools::fs::pln::Flightplan& plan, const QString& file,
                                 SaveOptions options)
{
  // Create a copy so we can easily change all waypoints to user defined is this is desired
  QList<atools::fs::pln::FlightplanEntry> planEntries = plan.entries;

  if(options & SAVE_GNS_USER_WAYPOINTS)
  {
    // Convert all waypoints to user defined waypoints keeping the names
    int i = 0;
    for(FlightplanEntry& entry : planEntries)
    {
      if(entry.isNoSave())
        // Do not save procedure points
        continue;

      if(i > 0 && i < planEntries.size() - 1)
      {
        entry.setAirway(QString());
        entry.setIcaoRegion(QString());
        entry.setWaypointId(entry.getIcaoIdent());
        entry.setWaypointType(entry::USER);
      }

      i++;
    }
  }

  filename = file;
  QFile xmlFile(filename);

  if(xmlFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QXmlStreamWriter writer(&xmlFile);
    writer.setCodec("utf-8");
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(2);

    writer.writeStartDocument("1.0");
    writer.writeStartElement("flight-plan");
    writer.writeAttribute("xmlns", "http://www8.garmin.com/xmlschemas/FlightPlan/v1");
    // 2017-01-15T15:20:54Z
    writer.writeTextElement("file-description", atools::programFileInfo());
    writer.writeTextElement("created", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

    // <xsd:simpleType name="Identifier_t">
    // <xsd:restriction base="xsd:string">
    // <xsd:pattern value="[A-Z0-9]{1,12}" />
    // </xsd:restriction>
    // </xsd:simpleType>

    // Collect waypoints first for a unique list =============================
    int wpNum = 1;
    int curIdx = 0;

    // Unique list of waypoints for output
    QMap<QStringList, Pos> waypointList;

    // Remember renamed user waypoints by index
    QHash<int, QString> userWaypointNameIndex;

    // Remember already added user waypoints
    QSet<QString> addedUserWaypoints;

    for(const FlightplanEntry& entry : planEntries)
    {
      if(entry.isNoSave())
        // Do not save procedure points
        continue;

      // Adjust name of user waypoints
      QString ident;
      if(entry.getWaypointType() == entry::USER || entry.getWaypointType() == entry::UNKNOWN)
      {
        ident = entry.getWaypointId();

        // Remove all invalid characters
        ident.replace(QRegularExpression("[^A-Z0-9]"), "");
        ident = ident.left(12);

        if(ident.isEmpty() || addedUserWaypoints.contains(ident))
          // Replace with own name if nothing left or a user waypoint with the same name already exists
          ident = QString("UPT%1").arg(wpNum++, 2, 10, QChar('0'));

        // Remember changed name in index
        userWaypointNameIndex.insert(curIdx, ident);
        addedUserWaypoints.insert(ident);
      }
      else
        // Normal waypoint - airport, vor, ndb, intersection
        ident = entry.getIcaoIdent();

      QStringList wptDat({ident, gnsType(entry), entry.getIcaoRegion()});

      if(waypointList.contains(wptDat))
      {
        // Waypoint already in index
        const Pos& pos = waypointList.value(wptDat);

        if(!pos.almostEqual(entry.getPosition(), Pos::POS_EPSILON_5M))
        {
          // Same identifier but different location - add as user waypoint
          wptDat[0] = QString("UPT%1").arg(wpNum++, 2, 10, QChar('0'));
          wptDat[1] = "USER WAYPOINT";
          userWaypointNameIndex.insert(curIdx, ident);
          addedUserWaypoints.insert(ident);
        }
      }

      // Add and deduplicate at the same time
      waypointList.insert(wptDat, entry.getPosition());

      curIdx++;
    }

    // Write waypoint list =============================
    writer.writeStartElement("waypoint-table");
    // <identifier>LFAT</identifier>
    // <type>AIRPORT</type>
    // <country-code>LF</country-code>
    // <lat>50.514722</lat>
    // <lon>1.627500</lon>
    for(const QStringList& key : waypointList.keys())
    {
      writer.writeStartElement("waypoint");

      writer.writeTextElement("identifier", key.value(0).toUpper());
      writer.writeTextElement("type", key.value(1));

      QString region = key.value(2).toUpper();
      if(region.isEmpty() && key.value(1) != "USER WAYPOINT")
        qDebug() << Q_FUNC_INFO << "Region is empty for" << key << "in" << filename;

      writer.writeTextElement("country-code", region);

      const Pos& pos = waypointList.value(key);
      writer.writeTextElement("lat", QString::number(pos.getLatY(), 'f', 6));
      writer.writeTextElement("lon", QString::number(pos.getLonX(), 'f', 6));

      writer.writeTextElement("comment", QString());

      writer.writeEndElement(); // waypoint
    }

    writer.writeEndElement(); // waypoint-table

    // Write route =============================
    writer.writeStartElement("route");

    writer.writeTextElement("route-name", plan.getDepartureIdent() + " / " + plan.getDestinationIdent());
    writer.writeTextElement("flight-plan-index", "1");

    // <route-point>
    // <waypoint-identifier>LFAT</waypoint-identifier>
    // <waypoint-type>AIRPORT</waypoint-type>
    // <waypoint-country-code>LF</waypoint-country-code>
    // </route-point>
    curIdx = 0;
    for(const FlightplanEntry& entry : planEntries)
    {
      if(entry.isNoSave())
        // Do not save procedure points
        continue;

      writer.writeStartElement("route-point");

      if(userWaypointNameIndex.contains(curIdx))
      {
        // Probably renamed user waypoint
        writer.writeTextElement("waypoint-identifier", userWaypointNameIndex.value(curIdx).toUpper());
        writer.writeTextElement("waypoint-type", "USER WAYPOINT");
      }
      else
      {
        writer.writeTextElement("waypoint-identifier", entry.getIcaoIdent().toUpper());
        writer.writeTextElement("waypoint-type", gnsType(entry));
      }

      writer.writeTextElement("waypoint-country-code", entry.getIcaoRegion().toUpper());

      writer.writeEndElement(); // route-point
      curIdx++;
    }

    writer.writeEndElement(); // route
    writer.writeEndElement(); // flight-plan
    writer.writeEndDocument();
    xmlFile.close();
  }
  else
    throw Exception(tr("Cannot open PLN file %1. Reason: %2").arg(file).arg(xmlFile.errorString()));

}

const QStringList& FlightplanIO::getAcceptedFlightPlanExtensions()
{
  return ACCEPTED_EXTENSIONS;
}

QStringList FlightplanIO::probeFile4(const QString& file)
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

    for(int i = lines.size(); i < 4; i++)
      lines.append(QString());
    testFile.close();
  }
  else
    throw Exception("Error reading \"" + filename + "\": " + testFile.errorString());

  return lines;
}

QString FlightplanIO::flightplanTypeToString(FlightplanType type)
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

FlightplanType FlightplanIO::stringFlightplanType(const QString& str)
{
  if(str == "IFR")
    return IFR;

  return VFR;
}

QString FlightplanIO::routeTypeToString(RouteType type)
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

RouteType FlightplanIO::stringToRouteType(const QString& str)
{
  if(str == "LowAlt")
    return LOW_ALTITUDE;
  else if(str == "HighAlt")
    return HIGH_ALTITUDE;
  else if(str == "VOR")
    return VOR;

  return DIRECT;
}

RouteType FlightplanIO::stringToRouteTypeFs9(const QString& str)
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

int FlightplanIO::routeTypeToStringFs9(atools::fs::pln::RouteType type)
{
  switch(type)
  {
    case atools::fs::pln::LOW_ALTITUDE:
      return 2;

    case atools::fs::pln::HIGH_ALTITUDE:
      return 3;

    case atools::fs::pln::VOR:
      return 1;

    case atools::fs::pln::DIRECT:
    case atools::fs::pln::UNKNOWN:
      return 0;
  }
  return 0;
}

void FlightplanIO::adjustDepartureAndDestination(atools::fs::pln::Flightplan& plan)
{
  if(!plan.entries.isEmpty())
  {
    if(plan.departureIdent.isEmpty())
      plan.departureIdent = plan.entries.first().getIcaoIdent();
    if(!plan.departurePos.isValid())
      plan.departurePos = plan.entries.first().getPosition();

    if(plan.destinationIdent.isEmpty())
      plan.destinationIdent = plan.entries.last().getIcaoIdent();

    if(!plan.destinationPos.isValid())
      plan.destinationPos = plan.entries.last().getPosition();

    if(plan.title.isEmpty())
      plan.title = QString("%1 to %2").arg(plan.departureIdent).arg(plan.destinationIdent);

    if(plan.description.isEmpty())
      plan.description = QString("%1, %2").arg(plan.departureIdent).arg(plan.destinationIdent);
    // These remain empty
    // departureParkingName, departureAiportName, destinationAiportName, appVersionMajor, appVersionBuild;
  }
}

int FlightplanIO::numEntriesSave(const atools::fs::pln::Flightplan& plan)
{
  int num = 0;
  for(const FlightplanEntry& entry : plan.entries)
  {
    if(!entry.isNoSave())
      // Do not save stuff like procedure points
      num++;
  }
  return num;
}

void FlightplanIO::assignAltitudeToAllEntries(atools::fs::pln::Flightplan& plan)
{
  int altitude = plan.cruisingAlt;
  for(FlightplanEntry& entry : plan.entries)
    entry.setPosition(Pos(entry.getPosition().getLonX(),
                          entry.getPosition().getLatY(), altitude));
}

QString FlightplanIO::gnsType(const atools::fs::pln::FlightplanEntry& entry)
{
  switch(entry.getWaypointType())
  {
    case atools::fs::pln::entry::UNKNOWN:
    case atools::fs::pln::entry::USER:
      return "USER WAYPOINT";

    case atools::fs::pln::entry::AIRPORT:
      return "AIRPORT";

    case atools::fs::pln::entry::INTERSECTION:
      return "INT";

    case atools::fs::pln::entry::VOR:
      return "VOR";

    case atools::fs::pln::entry::NDB:
      return "NDB";
      // <xsd:enumeration value="INT-VRP" /> ignored
  }
  return QString();
}

void FlightplanIO::writeBinaryString(char *mem, QString str, int length)
{
  // Cut off if too long and leave space for trailing 0
  str.truncate(length - 1);

  const char *data = str.toLatin1().data();
  memcpy(mem, data, strlen(data));

  // Fill rest with nulls
  size_t rest = static_cast<size_t>(length) - strlen(data);
  if(rest > 0)
    memset(&mem[length], 0, rest);
}

void FlightplanIO::posToRte(QTextStream& stream, const geo::Pos& pos, bool alt)
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

void FlightplanIO::readUntilElement(QXmlStreamReader& reader, const QString& name)
{
  while(reader.name() != name)
  {
    reader.readNextStartElement();

    if(reader.error() != QXmlStreamReader::NoError)
      throw Exception("Error reading \"" + filename + "\": " + reader.errorString());
  }
}

void FlightplanIO::readAppVersion(atools::fs::pln::Flightplan& plan, QXmlStreamReader& reader)
{
  while(reader.readNextStartElement())
  {
    QStringRef aName = reader.name();
    if(aName == "AppVersionMajor")
      plan.appVersionMajor = reader.readElementText();
    else if(aName == "AppVersionBuild")
      plan.appVersionBuild = reader.readElementText();
    else
      reader.skipCurrentElement();
  }
}

void FlightplanIO::readWaypoint(atools::fs::pln::Flightplan& plan, QXmlStreamReader& reader)
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
  plan.entries.append(entry);
}

} // namespace pln
} // namespace fs
} // namespace atools
