/*****************************************************************************
* Copyright 2015-2023 Alexander Barthel alex@littlenavmap.org
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
#include "util/xmlstream.h"
#include "zip/gzip.h"

#include <QBitArray>
#include <QDataStream>
#include <QDateTime>
#include <QTimeZone>
#include <QFile>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QXmlStreamReader>
#include <QDir>

using atools::geo::Pos;
using atools::fs::pln::Flightplan;
using atools::fs::pln::FlightplanEntry;

namespace atools {
namespace fs {
namespace pln {

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
using Qt::endl;
#endif

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
  errorMsg = tr("Cannot open file %1. Reason: %2");
}

FlightplanIO::~FlightplanIO()
{

}

atools::fs::pln::FileFormat FlightplanIO::load(atools::fs::pln::Flightplan& plan, const QString& file)
{
  FileFormat format = detectFormat(file);

  plan.clearAll();

  switch(format)
  {
    case atools::fs::pln::NONE:
      throw Exception(tr("Cannot open flight plan file \"%1\". No supported flight plan format detected. "
                         "Supported formats are LNMPLN, PLN (FSX XML, MSFS XML, FS9 INI and FSC), X-Plane FMS, FLP and "
                         "FlightGear FGFP.").arg(file));

    case atools::fs::pln::LNM_PLN:
      loadLnm(plan, file);
      plan.setLnmFormat(true); // Indicate that plan was loaded using new native format
      break;

    case atools::fs::pln::MSFS_PLN:
    case atools::fs::pln::FSX_PLN:
      loadPln(plan, file);
      plan.setLnmFormat(false); // Indicate that a "foreign" format was user to load which cannot be saved directly
      break;

    case atools::fs::pln::FS9_PLN:
      loadFs9(plan, file);
      plan.setLnmFormat(false);
      break;

    case atools::fs::pln::FMS11:
    case atools::fs::pln::FMS3:
      loadFms(plan, file);
      plan.setLnmFormat(false);
      break;

    case atools::fs::pln::FLP:
      loadFlp(plan, file);
      plan.setLnmFormat(false);
      break;

    case atools::fs::pln::FSC_PLN:
      loadFsc(plan, file);
      plan.setLnmFormat(false);
      break;

    case atools::fs::pln::FLIGHTGEAR:
      loadFlightGear(plan, file);
      plan.setLnmFormat(false);
      break;

    case atools::fs::pln::GARMIN_FPL:
      loadGarminFpl(plan, file);
      plan.setLnmFormat(false);
      break;

    case atools::fs::pln::GARMIN_GFP:
      loadGarminGfp(plan, file);
      plan.setLnmFormat(false);
      break;
  }
  return format;
}

FileFormat FlightplanIO::detectFormat(const QString& file)
{
  // Get first 30 non empty lines - always returns a list of 30
  QStringList lines = probeFile(file, 30 /* numLinesRead */);

  if(lines.isEmpty())
    throw Exception(tr("Cannot open empty flight plan file \"%1\".").arg(file));

  if(lines.constFirst().startsWith("[corte]"))
    // FLP: [CoRte]
    return FLP;
  else if(lines.at(0).startsWith("<?xml version") &&
          (lines.at(1).startsWith("<simbase.document type=\"acexml\"") ||
           lines.at(0).contains("<simbase.document type=\"acexml\"")))
  {
    // FSX PLN or MSFS PLN <?xml version
    if(atools::strAnyStartsWith(lines, "<appversionmajor>11"))
      // Major version 11 is MSFS
      return MSFS_PLN;
    else
      // Major version 10 is FSX and P3D
      return FSX_PLN;
  }
  else if(lines.at(0).startsWith("<?xml version") &&
          lines.at(1).startsWith("<littlenavmap") &&
          lines.at(2).startsWith("<flightplan"))
    return LNM_PLN;
  else if(lines.at(0).startsWith("[flightplan]") && FS9_MATCH.match(lines.at(1)).hasMatch())
    // FS9 ini format
    return FS9_PLN;
  else if(lines.at(0).startsWith("[fscfp]"))
    // FSC ini format
    return FSC_PLN;
  else if((lines.at(0) == "i" || lines.at(0) == "a") &&
          lines.at(1).startsWith("3 version") &&
          lines.at(2).at(0).isDigit() &&
          lines.at(3).at(0).isDigit())
    // Old format
    // I
    // 3 version
    // 1
    // 4
    return FMS3;
  else if((lines.at(0) == "i" || lines.at(0) == "a") &&
          lines.at(1).startsWith("1100 version") &&
          lines.at(2).startsWith("cycle"))
    // New v11 format
    // I
    // 1100 Version
    // CYCLE 1710
    return FMS11;
  else if(lines.at(0).startsWith("<?xml version") &&
          (lines.at(1).startsWith("<propertylist") || lines.at(0).contains("<propertylist")) &&
          (lines.at(2).startsWith("<version") || lines.at(0).contains("<version") ||
           lines.at(2).startsWith("<departure") || lines.at(0).contains("<departure") ||
           lines.at(2).startsWith("<source") || lines.at(0).contains("<source"))
          /* && lines.at(2).startsWith("<version type=\"int\">")*/)
    // <?xml version="1.0" encoding="UTF-8"?>
    // <PropertyList>
    // <version type="int">1</version>
    return FLIGHTGEAR;
  else if((lines.at(0).startsWith("<?xml version") &&
           (lines.at(1).startsWith("<flight-plan") &&
            !lines.filter("<waypoint-table").isEmpty())) ||
          (lines.at(0).startsWith("<flight-plan") &&
           !lines.filter("<waypoint-table").isEmpty()))
    // <?xml version="1.0" encoding="utf-8"?>
    // <flight-plan xmlns="http://www8.garmin.com/xmlschemas/FlightPlan/v1">
    // <created>2010-11-20T20:54:34Z</created>
    // <waypoint-table>
    return GARMIN_FPL;
  else if(lines.at(0).startsWith("fpn/ri:"))
    // FPN/RI:F:BIKF:F:RIMUM:F:CELLO:F:6119N:F:BILTO:F:NETKI:F:AMDEP:F:UMLER:F:RIVAK:F:KORUL:F:MAVOS:F:LEAS
    // FPN/RI:DA:KYKM:D:WENAS7.PERTT:R:09O:F:COBDI,N47072W120397:F:N47406W120509:F: ...
    // ... ROZSE,N48134W121018:F:DIABO,N48500W120562.J503.FOLDY,N49031W120427:AA:CYLW:A:PIGLU4.YDC(16O):AP:I16-Z.HUMEK
    return GARMIN_GFP;
  else
    return NONE;
}

void FlightplanIO::loadFlp(atools::fs::pln::Flightplan& plan, const QString& filename)
{
  qDebug() << Q_FUNC_INFO << filename;

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
          departure.setIdent(value);
          departure.setWaypointType(atools::fs::pln::entry::AIRPORT);
        }
        else if(key == "arptarr")
        {
          plan.destinationIdent = value;
          destination.setIdent(value);
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
              plan.append(entry);
              entry = FlightplanEntry();
            }
            wptNum = num;
          }

          if(coords.isEmpty())
            entry.setIdent(value);
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
              plan.append(entry);
              entry = FlightplanEntry();
            }
            wptNum = num;
          }

          if(fromTo.isEmpty())
            entry.setAirway(value);
          else if(fromTo.toLower() == "from")
          {
            if(plan.isEmpty() || plan.constLast().getIdent() != value)
            {
              FlightplanEntry from;
              from.setIdent(value);
              plan.append(from);
            }
          }
          else if(fromTo.toLower() == "to")
            entry.setIdent(value);
        }
        else if(!value.isEmpty())
        {
          if(key == "rwydep")
            insertPropertyIf(plan, SIDRW, value.mid(plan.departureIdent.size()));
          else if(key == "sid")
            insertPropertyIf(plan, SID, value);
          else if(key == "sid_trans" || key == "sidenrtrans") // TODO Might result in loading errors for SID transitions
          {
            if(value != "VECTORS")
              insertPropertyIf(plan, SIDTRANS, value);
          }
          else if(key == STAR)
            insertPropertyIf(plan, STAR, value);
          else if(key == "star_trans" || key == "enrstartrans") // TODO Might result in loading errors for STAR transitions
          {
            if(value != "VECTORS")
              insertPropertyIf(plan, STARTRANS, value);
          }
          else if(key == "rwyarr")
            insertPropertyIf(plan, APPROACHRW, value.mid(plan.destinationIdent.size()));
          else if(key == "rwyarrfinal")
            insertPropertyIf(plan, APPROACH_ARINC, value);
          else if(key == "appr_trans")
          {
            if(value != "VECTORS")
              insertPropertyIf(plan, TRANSITION, value);
          }
        }
      }
    }
    plan.append(entry);
    plan.prepend(departure);
    plan.append(destination);

    flpFile.close();
    plan.flightplanType = NO_TYPE;
    plan.cruiseAltitudeFt = 0.f; // Use either GUI value or calculate from airways

    plan.adjustDepartureAndDestination();
    flpFile.close();
  }
  else
    throw Exception(errorMsg.arg(filename).arg(flpFile.errorString()));
}

void FlightplanIO::loadFms(atools::fs::pln::Flightplan& plan, const QString& filename)
{
  qDebug() << Q_FUNC_INFO << filename;

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

  QFile flpFile(filename);
  int version = 0;
  bool v11Format = false;
  int minListSize = 5;
  int fieldOffset = 0;

  if(flpFile.open(QIODevice::ReadOnly))
  {
    QTextStream stream(&flpFile);
    stream.setCodec("UTF-8");
    stream.setAutoDetectUnicode(true);

    stream.readLine(); // I
    bool ok = false;
    version = stream.readLine().section(" ", 0, 0).toInt(&ok); // 3 version
    if(!ok)
      throw Exception(tr("Invalid FMS file. Cannot read version number: %1").arg(flpFile.fileName()));

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
      throw Exception(tr("Invalid FMS file. Invalid version %2: %1").arg(flpFile.fileName()).arg(version));

    float maxAlt = std::numeric_limits<float>::min();
    QString destinationRwy;

    while(!stream.atEnd())
    {
      QString line = stream.readLine().simplified();
      if(line.size() > 4)
      {
        if(line.startsWith("0 ----")) // End of file indicator
          break;

        const QList<QString> list = line.split(" ");

        QString airway;
        if(v11Format)
        {
          // Read keywords from version 11
          QString key = list.value(0);
          QString value = list.value(1);
          if(key == "CYCLE")
          {
            insertPropertyIf(plan, SIMDATA, "XP11");
            insertPropertyIf(plan, SIMDATACYCLE, value);
            continue;
          }
          else if(key == "DEPRWY")
          {
            insertPropertyIf(plan, SIDRW, value.mid(2));
            continue;
          }
          else if(key == "SID")
          {
            insertPropertyIf(plan, SID, value);
            continue;
          }
          else if(key == "SIDTRANS")
          {
            insertPropertyIf(plan, SIDTRANS, value);
            continue;
          }
          else if(key == "STAR")
          {
            insertPropertyIf(plan, STAR, value);
            continue;
          }
          else if(key == "STARTRANS")
          {
            insertPropertyIf(plan, STARTRANS, value);
            continue;
          }
          else if(key == "APP")
          {
            insertPropertyIf(plan, APPROACH_ARINC, value);
            continue;
          }
          else if(key == "APPTRANS")
          {
            insertPropertyIf(plan, TRANSITION, value);
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
          if(!col2.isEmpty() && col2 != "DRCT" && col2 != "DIRECT" &&
             col2 != "ADEP" && col2 != "DEP" && col2 != "ADES" && col2 != "DES")
            airway = col2;
        }

        if(list.size() >= minListSize)
        {
          bool altOk;
          float altitude = list.at(2 + fieldOffset).toFloat(&altOk);
          if(!altOk || altitude < atools::fs::pln::FLIGHTPLAN_ALTITUDE_FT_MIN || altitude > atools::fs::pln::FLIGHTPLAN_ALTITUDE_FT_MAX)
          {
            // Avoid excessive altitudes
            qWarning() << Q_FUNC_INFO << "Invalid altitude";
            altitude = 0.f;
          }

          bool lonOk, latOk;
          Pos position(list.at(4 + fieldOffset).toFloat(&lonOk), list.at(3 + fieldOffset).toFloat(&latOk), altitude);
          if(!position.isValidRange() || !lonOk || !latOk)
            throw Exception(tr("Invalid FMS file. Invalid coordinate in %1").arg(flpFile.fileName()));

          FlightplanEntry entry;
          const QString& ident = list.at(1);

          maxAlt = std::max(maxAlt, altitude);

          entry.setPosition(position);

          bool typeOk;
          int type = list.at(0).toInt(&typeOk);
          if(!typeOk)
            throw Exception(tr("Invalid FMS file. Cannot read waypoint type in %1").arg(flpFile.fileName()));

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
              entry.setWaypointType(atools::fs::pln::entry::WAYPOINT);
              break;

            case 28: // - Lat/Lon Position
            case 13: // - Lat/Lon Position
              entry.setWaypointType(atools::fs::pln::entry::USER);
              break;

            default:
              throw Exception(tr("Invalid FMS file. Invalid waypoint type in %1").arg(flpFile.fileName()));
          }

          entry.setIdent(ident);
          entry.setAirway(airway);

          plan.append(entry);
        }
        else
          throw Exception(tr("Invalid FMS file. Number of sections is not %2: %1").
                          arg(flpFile.fileName()).arg(minListSize));
      }
    }
    flpFile.close();

    if(!destinationRwy.isEmpty())
    {
      if(plan.properties.contains(APPROACH))
        insertPropertyIf(plan, APPROACHRW, destinationRwy);
      else if(plan.properties.contains(STAR))
        insertPropertyIf(plan, STARRW, destinationRwy);
    }

    plan.flightplanType = NO_TYPE;

    // Check if there are waypoints with zero altitude to avoid invalid maximum in broken plans
    for(int i = 1; i < plan.size() - 1; i++)
    {
      if(atools::almostEqual(plan.at(i).getAltitude(), 0.f))
      {
        maxAlt = 0.f; // Use value from GUI
        break;
      }
    }
    plan.cruiseAltitudeFt = maxAlt > atools::fs::pln::FLIGHTPLAN_ALTITUDE_FT_MIN ? maxAlt : 0.f; // Use value from GUI

    plan.adjustDepartureAndDestination();
    plan.assignAltitudeToAllEntries();

    flpFile.close();
  }
  else
    throw Exception(errorMsg.arg(filename).arg(flpFile.errorString()));
}

void FlightplanIO::loadFsc(atools::fs::pln::Flightplan& plan, const QString& filename)
{
  qDebug() << Q_FUNC_INFO << filename;
  // [FSCFP]
  // DepartAPCode=KLAS
  // DepartNum=0
  // DepartID=26R
  // DepartType=Rwy
  // DestAPCode=KSAN
  // RouteType=1
  // SID=RADYR2 (26R)
  // STAR=LUCKI1
  // Transition=
  // WP=1,Int,RUDYY,RUDYY,36.068786,-115.258975,0.000000,262.596679,11.200000,3,0,0,,0,0,0,RADYR2
  // WP=2,Int,SELLZ,SELLZ,35.992428,-115.293894,0.000000,200.305925,11.200000,3,0,0,,0,0,0,RADYR2
  // WP=3,Int,RADYR,RADYR,35.609006,-115.387875,0.000000,191.271261,11.200000,3,0,0,,0,0,0,RADYR2
  // WP=4,Int,DOYYL,DOYYL,35.119733,-115.353233,0.000000,176.685433,11.100000,3,0,0,,0,0,0,RADYR2
  // WP=5,Int,ZELMA,ZELMA,34.783331,-115.329864,0.000000,176.734506,11.000000,3,0,0,,0,0,0,RADYR2
  // WP=6,Int,JOTNU,JOTNU,34.325864,-115.529525,0.000000,199.827832,11.000000,3,0,0,,0,0,0,RADYR2
  // WP=7,Int,LVELL,LVELL,34.210383,-115.614792,0.000000,211.415245,11.000000,3,0,0,,0,0,0,RADYR2
  // WP=8,Int,LVELL,LVELL,34.210383,-115.614792,0.000000,0.000000,11.000000,2,0,0,,0,0,0
  // WP=9,Int,CHADT,CHADT,33.921803,-115.750906,0.000000,201.359438,10.900000,2,0,0,,0,0,0,LUCKI1
  // WP=10,Int,CABIC,CABIC,33.771392,-115.824642,0.000000,202.174321,10.900000,2,0,0,,0,0,0,LUCKI1
  // WP=11,Int,MOMAR,MOMAR,33.515036,-115.944483,0.000000,201.253645,10.900000,2,0,0,,0,0,0,LUCKI1
  // WP=12,Int,DSURT,DSURT,33.337500,-116.250000,0.000000,235.236049,11.000000,2,0,0,,0,0,0,LUCKI1
  // WP=13,Int,TRIXI,TRIXI,33.005556,-116.444167,0.000000,206.140245,11.000000,2,0,0,,0,0,0,LUCKI1
  // WP=14,Int,HSKER,HSKER,32.844444,-116.606667,0.000000,220.295916,11.000000,2,0,0,,0,0,0,LUCKI1
  // WP=15,Int,BARET,BARET,32.774108,-116.677464,0.000000,220.249993,11.000000,2,0,0,,0,0,0,LUCKI1
  // WP=16,Int,HNAHH,HNAHH,32.739917,-116.749694,0.000000,240.645456,11.000000,2,0,0,,0,0,0,LUCKI1
  // WP=17,Int,LUCKI,LUCKI,32.707500,-116.818056,0.000000,240.610457,11.000000,2,0,0,,0,0,0,LUCKI1
  // WP=18,Int,LYNDI,LYNDI,32.688222,-116.879861,0.000000,249.679288,11.000000,2,0,0,,0,0,0,LUCKI1

  // [FSCFP]
  // DepartAPCode=VHHH
  // DepartNum=0
  // DepartID=07R
  // DepartType=Rwy
  // DestAPCode=ZSHC
  // RouteType=1
  // SID=BEKO3A (07R)
  // STAR=ELN81A
  // Transition=
  // WP=1,Int,PORPA,PORPA,22.335861,114.021194,0.000000,74.664997,-3.300000,3,0,0,,0,0,0,BEKO3A
  // WP=2,Fix,TD,TUNG LUNG,22.247894,114.293139,116.100000,109.218089,-3.300000,3,0,0,,0,0,0,BEKO3A
  // WP=3,Int,ATENA,ATENA,22.411069,114.386633,0.000000,27.905991,-3.400000,3,0,0,,0,0,0,BEKO3A
  // WP=4,Int,BEKOL,BEKOL,22.543333,114.133333,0.000000,299.518842,-3.400000,3,0,0,,0,0,0,BEKO3A
  // WP=5,Int,IDUMA,IDUMA,22.896389,113.951667,0.000000,334.644805,-3.400000,1,0,0,,0,0,0,A461,0,0
  // ...
  // WP=18,Int,ELNEX,ELNEX,29.631667,119.490000,0.000000,47.904800,-5.800000,2,0,0,,0,0,0,A599,0,0
  // WP=19,Int,UGAGO,UGAGO,29.627500,119.650556,0.000000,91.670526,-5.800000,2,0,0,,0,0,0,ELN81A
  // WP=20,Int,MOLGU,MOLGU,29.850000,119.966667,0.000000,50.892435,-5.900000,2,0,0,,0,0,0,ELN81A
  // WP=21,Int,HC309,HC309,29.898056,120.067778,0.000000,61.247184,-5.900000,2,0,0,,0,0,0,ELN81A
  // WP=22,Int,HC307,HC307,29.996111,120.275000,0.000000,61.309367,-6.000000,2,0,0,,0,0,0,ELN81A
  // WP=23,Int,HC306,HC306,30.125833,120.566389,0.000000,62.707069,-6.100000,2,0,0,,0,0,0,ELN81A
  // WP=24,Int,HC305,HC305,30.217500,120.774167,0.000000,62.912000,-6.100000,2,0,0,,0,0,0,ELN81A
  // WP=25,Int,HC304,HC304,30.282083,120.736611,0.000000,333.337497,-6.100000,2,0,0,,0,0,0,ELN81A

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

  QFile plnFile(filename);

  if(plnFile.open(QIODevice::ReadOnly))
  {
    QTextStream stream(&plnFile);
    FlightplanEntry departure, destination;
    QString sidAndRw, starAndRw;

    while(!stream.atEnd())
    {
      QString line = stream.readLine().simplified();
      if(!line.isEmpty())
      {
        QString key = line.section('=', 0, 0).toUpper().trimmed();
        QString value = line.section('=', 1).simplified();

        if(value.isEmpty())
          continue;

        if(key == "DEPARTAPCODE")
        {
          departure.setIdent(value);
          departure.setWaypointType(atools::fs::pln::entry::AIRPORT);
        }
        else if(key == "DESTAPCODE")
        {
          destination.setIdent(value);
          destination.setWaypointType(atools::fs::pln::entry::AIRPORT);
        }
        else if(key == "SID")
          sidAndRw = value.remove('(').remove(')').simplified(); // SID=RADYR2 (26R)
        else if(key == "STAR")
          starAndRw = value.remove('(').remove(')').simplified(); // STAR=ELN81A
        else if(key == "WP")
        {
          QStringList values = value.split(",");
          QString type = values.value(1).toLower();
          QString ident = values.value(2);

          FlightplanEntry entry;
          entry.setIdent(ident);
          entry.setName(values.value(3));
          entry.setAirway(values.value(16)); // Either airway or procedure - store procedure temporarily in airway field
          entry.setPosition(Pos(values.value(5).toFloat(), values.value(4).toFloat()));

          if(type == "fix" || type == "int")
            entry.setWaypointType(atools::fs::pln::entry::WAYPOINT);
          else if(type == "uwp" || !atools::fs::util::isValidIdent(ident))
            entry.setWaypointType(atools::fs::pln::entry::USER);

          plan.append(entry);
        }
        // Ignored keys
        // else if(key == "DEPARTNUM")
        // else if(key == "DEPARTID")
        // else if(key == "DEPARTTYPE")
        // else if(key == "TRANSITION")
      }
    } // while(!stream.atEnd())

    // Now determine SID, runway and entries to delete ==========================================

    // Remember procedure entries to delete
    QVector<int> procedureEntryIndexes;
    if(!sidAndRw.isEmpty())
    {
      QString sid = sidAndRw.section(' ', 0, 0);
      QString rw = sidAndRw.section(' ', 1, 1);
      QString transWp;
      bool sidFound = false;

      // Check from start to end - entries do not contain departure airport yet
      for(int i = 0; i < plan.size(); i++)
      {
        const FlightplanEntry& entry = plan.at(i);
        if(entry.getAirway() == sid)
        {
          // Airway field matches SID name in header
          procedureEntryIndexes.append(i);
          sidFound = true;
        }
        else
        {
          // End of matching entries - get waypoint name from previous entry for probable transition
          transWp = plan.value(i - 1).getIdent();
          break;
        }
      }

      if(sidFound)
      {
        insertPropertyIf(plan, SID, sid);
        insertPropertyIf(plan, SIDRW, rw);
        insertPropertyIf(plan, SIDTRANSWP, transWp);
      }
    }

    // Now determine STAR, runway and entries to delete ==========================================
    if(!starAndRw.isEmpty())
    {
      QString star = starAndRw.section(' ', 0, 0);
      QString rw = starAndRw.section(' ', 1, 1);
      QString transWp;
      bool starFound = false;

      // Check starting from end - entries do not contain destination airport yet
      for(int i = plan.size() - 1; i >= 0; i--)
      {
        const FlightplanEntry& entry = plan.at(i);
        if(entry.getAirway() == star)
        {
          // Airway field matches STAR name in header
          procedureEntryIndexes.append(i);
          starFound = true;
        }
        else
        {
          // End of matching entries - get waypoint name for probable transition
          transWp = plan.value(i).getIdent();
          break;
        }
      }

      if(starFound)
      {
        insertPropertyIf(plan, STAR, star);
        insertPropertyIf(plan, STARRW, rw);
        insertPropertyIf(plan, STARTRANSWP, transWp);
      }
    }

    if(!procedureEntryIndexes.isEmpty())
    {
      // Sort, remove duplicates and delete entries starting from end of list
      std::sort(procedureEntryIndexes.begin(), procedureEntryIndexes.end());
      procedureEntryIndexes.erase(std::unique(procedureEntryIndexes.begin(), procedureEntryIndexes.end()), procedureEntryIndexes.end());
      for(int i = procedureEntryIndexes.size() - 1; i >= 0; i--)
        plan.removeAt(procedureEntryIndexes.at(i));
    }

    // Add departure and start airports
    plan.prepend(departure);
    plan.append(destination);

    plan.flightplanType = NO_TYPE;
    plan.cruiseAltitudeFt = 0.f; // Use either GUI value or calculate from airways
    plan.adjustDepartureAndDestination();
  }
  else
    throw Exception(errorMsg.arg(filename).arg(plnFile.errorString()));
}

void FlightplanIO::loadFs9(atools::fs::pln::Flightplan& plan, const QString& filename)
{
  qDebug() << Q_FUNC_INFO << filename;
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

        if(key == "type")
          // type=IFR
          plan.flightplanType = stringFlightplanType(value);
        else if(key == "cruising_altitude")
          plan.cruiseAltitudeFt = value.toFloat();
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
          plan.departureName = value;
        else if(key == "destination_id")
        {
          // destination_id=EISG, N54* 16.82', W008* 35.95', +000011.00
          plan.destinationIdent = value.section(',', 0, 0).trimmed();
          plan.destinationPos = Pos(value.section(',', 1, 3).trimmed());
        }
        else if(key == "destination_name")
          // destination_name=SLIGO
          plan.destinationName = value;
        else if(key.startsWith("waypoint."))
        {
          FlightplanEntry entry;

          Pos pos(value.section(',', 5, 7).trimmed(), false);
          if(pos.isValid())
          {
            // waypoint.0=   , EGPB, , EGPB, A, N59* 52.88', W001* 17.63', +000020.00,
            // waypoint.1= KK, WIK , , WIK , V, N58* 27.53', W003* 06.02', +000000.00,
            // ----------- 0   1    2  3     4  5            6             7          8
            entry.setRegion(value.section(',', 0, 0).trimmed());
            entry.setIdent(value.section(',', 1, 1).trimmed());
            // ignore airport name at 2
            // entry.setWaypointId(value.section(',', 3, 3).trimmed());
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
              entry.setIdent(value.section(',', 0, 0).trimmed());
              // ignore airport name at 2
              // entry.setWaypointId(value.section(',', 0, 0).trimmed());
              entry.setWaypointType(value.section(',', 1, 1).trimmed());
              entry.setPosition(pos);
              entry.setAirway(value.section(',', 5, 5).trimmed());
            }
            else
              throw Exception(tr("Invalid flight plan file \"%1\".").arg(filename));
          }

          plan.append(entry);
        }
        // else if(key == "alternate_name") ignore
      }
    }
    plnFile.close();
  }
  else
    throw Exception(errorMsg.arg(filename).arg(plnFile.errorString()));
}

atools::geo::Pos FlightplanIO::readPosLnm(atools::util::XmlStream& xmlStream)
{
  bool lonOk, latOk, altOk;
  QXmlStreamReader& reader = xmlStream.getReader();
  float lon = reader.attributes().value("Lon").toFloat(&lonOk);
  float lat = reader.attributes().value("Lat").toFloat(&latOk);
  float alt = reader.attributes().value("Alt").toFloat(&altOk);

  // Read only attributes
  reader.skipCurrentElement();

  if(lonOk && latOk)
  {
    atools::geo::Pos pos(lon, lat);

    if(!pos.isValid())
      throw Exception(tr("Invalid position in LNMPLN file \"%1\".").arg(xmlStream.getFilename()));

    if(!pos.isValidRange())
      throw Exception(tr("Invalid position in LNMPLN file \"%1\". Ordinates out of range: %1").
                      arg(xmlStream.getFilename()).arg(pos.toString()));

    // Set altitude if given
    if(altOk)
      pos.setAltitude(alt);
    return pos;
  }
  else
    throw Exception(tr("Invalid position in LNMPLN file \"%1\". Ordinate(s) are not numbers.").
                    arg(xmlStream.getFilename()));
}

void FlightplanIO::readPosGpx(atools::geo::Pos& pos, QString& name, QDateTime& timestamp, atools::util::XmlStream& xmlStream)
{
  bool lonOk, latOk;
  QXmlStreamReader& reader = xmlStream.getReader();
  float lon = reader.attributes().value("lon").toFloat(&lonOk);
  float lat = reader.attributes().value("lat").toFloat(&latOk);

  if(lonOk && latOk)
  {
    pos.setLonX(lon);
    pos.setLatY(lat);

    if(!pos.isValid())
    {
      qWarning() << Q_FUNC_INFO << "Invalid position in GPX. Ordinates out of range" << pos.toString();
      pos = atools::geo::EMPTY_POS;
    }
    else if(!pos.isValidRange())
    {
      qWarning() << Q_FUNC_INFO << "Invalid position in GPX. Ordinates out of range" << pos.toString();
      pos = atools::geo::EMPTY_POS;
    }
  }
  else
    throw Exception(tr("Invalid position in GPX file \"%1\".").arg(xmlStream.getFilename()));

  while(xmlStream.readNextStartElement())
  {
    if(reader.name() == "name")
      name = reader.readElementText();
    else if(reader.name() == "time")
      // Reads with or without milliseconds and returns UTC without changed hour number
      timestamp = QDateTime::fromString(reader.readElementText(), Qt::ISODate);
    else if(reader.name() == "ele")
      pos.setAltitude(atools::geo::meterToFeet(reader.readElementText().toFloat()));
    else
      xmlStream.skipCurrentElement(false /* warn */);
  }
}

void FlightplanIO::insertPropertyIf(atools::fs::pln::Flightplan& plan, const QString& key, const QString& value)
{
  if(!value.trimmed().isEmpty())
    plan.properties.insert(key, value.trimmed());
}

void FlightplanIO::readWaypointsLnm(atools::util::XmlStream& xmlStream, QList<FlightplanEntry>& entries, const QString& elementName)
{
  QXmlStreamReader& reader = xmlStream.getReader();

  while(xmlStream.readNextStartElement())
  {
    // Read waypoint element
    if(reader.name() == elementName)
    {
      // Read child elements
      FlightplanEntry entry;
      while(xmlStream.readNextStartElement())
      {
        if(reader.name() == "Name")
          entry.setName(reader.readElementText());
        else if(reader.name() == "Ident")
          entry.setIdent(reader.readElementText());
        else if(reader.name() == "Region")
          entry.setRegion(reader.readElementText());
        else if(reader.name() == "Name")
          entry.setName(reader.readElementText());
        else if(reader.name() == "Airway")
          entry.setAirway(reader.readElementText());
        else if(reader.name() == "Track")
        {
          // NAT, PACOTS or AUSOTS track
          entry.setAirway(reader.readElementText());
          entry.setFlag(atools::fs::pln::entry::TRACK);
        }
        else if(reader.name() == "Type")
          entry.setWaypointTypeFromLnm(reader.readElementText());
        else if(reader.name() == "Comment")
          entry.setComment(reader.readElementText());
        else if(reader.name() == "Pos")
          entry.setPosition(readPosLnm(xmlStream));
        else
          xmlStream.skipCurrentElement(true /* warn */);
      }
      entries.append(entry);
    }
    else
      xmlStream.skipCurrentElement(true /* warn */);
  }
}

void FlightplanIO::loadLnmStr(atools::fs::pln::Flightplan& plan, const QString& string)
{
  plan.clearAll();
  if(!string.isEmpty())
  {
    atools::util::XmlStream xmlStream(string);
    loadLnmInternal(plan, xmlStream);
  }
}

void FlightplanIO::loadLnmGz(atools::fs::pln::Flightplan& plan, const QByteArray& bytes)
{
  loadLnmStr(plan, QString(atools::zip::gzipDecompress(bytes)));
}

void FlightplanIO::loadLnm(atools::fs::pln::Flightplan& plan, const QString& filename)
{
  plan.clearAll();
  QFile xmlFile(filename);
  if(xmlFile.open(QIODevice::ReadOnly))
  {
    atools::util::XmlStream xmlStream(&xmlFile, filename);
    loadLnmInternal(plan, xmlStream);
    xmlFile.close();
  }
  else
    throw Exception(tr("Cannot open file \"%1\". Reason: %2").arg(filename).arg(xmlFile.errorString()));
}

void FlightplanIO::loadLnmInternal(atools::fs::pln::Flightplan& plan, atools::util::XmlStream& xmlStream)
{
  QXmlStreamReader& reader = xmlStream.getReader();

  QList<FlightplanEntry> alternates, waypoints;

  xmlStream.readUntilElement("LittleNavmap");
  xmlStream.readUntilElement("Flightplan");

  // ==================================================================================
  // Read all elements - order does not matter
  // Additional unknown elements are ignored and a warning is logged
  plan.cruiseAltitudeFt = 0.f;
  while(xmlStream.readNextStartElement())
  {
    // Read data from header =========================================
    if(reader.name() == "Header")
    {
      while(xmlStream.readNextStartElement())
      {
        if(reader.name() == "CreationDate" || reader.name() == "FileVersion" || reader.name() == "ProgramName" ||
           reader.name() == "ProgramVersion" || reader.name() == "Documentation")
        {
          // Skip these elements without warning
          xmlStream.skipCurrentElement();
          continue;
        }

        if(reader.name() == "FlightplanType")
          plan.flightplanType = stringFlightplanType(reader.readElementText());
        else if(reader.name() == "CruisingAltF")
          plan.cruiseAltitudeFt = reader.readElementText().toFloat(); // Always prefer more accurate value
        else if(reader.name() == "CruisingAlt")
        {
          if(atools::almostEqual(plan.cruiseAltitudeFt, 0.f))
            plan.cruiseAltitudeFt = reader.readElementText().toInt(); // Fall back to old integer representation
        }
        else if(reader.name() == "Comment")
          plan.comment = reader.readElementText();
        else
          xmlStream.skipCurrentElement(true /* warn */);
      }
    }
    // Simulator and navdata type and cycle =========================================
    else if(reader.name() == "SimData")
    {
      insertPropertyIf(plan, SIMDATACYCLE, reader.attributes().value("Cycle").toString());
      insertPropertyIf(plan, SIMDATA, reader.readElementText());
    }
    else if(reader.name() == "NavData")
    {
      insertPropertyIf(plan, NAVDATACYCLE, reader.attributes().value("Cycle").toString());
      insertPropertyIf(plan, NAVDATA, reader.readElementText());
    }
    // Used aircraft performance =========================================
    else if(reader.name() == "AircraftPerformance")
    {
      while(xmlStream.readNextStartElement())
      {
        if(reader.name() == "FilePath")
          insertPropertyIf(plan, AIRCRAFT_PERF_FILE, reader.readElementText());
        else if(reader.name() == "Type")
          insertPropertyIf(plan, AIRCRAFT_PERF_TYPE, reader.readElementText());
        else if(reader.name() == "Name")
          insertPropertyIf(plan, AIRCRAFT_PERF_NAME, reader.readElementText());
        else
          xmlStream.skipCurrentElement(true /* warn */);
      }
    }
    // Alternate airports list =========================================
    else if(reader.name() == "Alternates")
    {
      readWaypointsLnm(xmlStream, alternates, "Alternate");
      for(FlightplanEntry& entry : alternates)
      {
        entry.setWaypointType(atools::fs::pln::entry::AIRPORT);
        entry.setFlag(atools::fs::pln::entry::ALTERNATE);
      }
    }
    // Departure position (gate, etc.) =========================================
    else if(reader.name() == "Departure")
    {
      while(xmlStream.readNextStartElement())
      {
        if(reader.name() == "Start")
          plan.departureParkingName = reader.readElementText();
        else if(reader.name() == "Pos")
          plan.departureParkingPos = readPosLnm(xmlStream);
        else if(reader.name() == "Type")
          plan.setDepartureParkingType(reader.readElementText());
        else if(reader.name() == "Heading")
        {
          bool ok;
          plan.departureParkingHeading = reader.readElementText().toFloat(&ok);
          if(!ok)
            plan.departureParkingHeading = INVALID_HEADING;
        }
        else
          xmlStream.skipCurrentElement(true /* warn */);
      }
    }
    // Procedures =========================================
    else if(reader.name() == "Procedures")
    {
      while(xmlStream.readNextStartElement())
      {
        if(reader.name() == "SID")
        {
          while(xmlStream.readNextStartElement())
          {
            if(reader.name() == "Name")
              insertPropertyIf(plan, SID, reader.readElementText());
            else if(reader.name() == "Runway")
              insertPropertyIf(plan, SIDRW, reader.readElementText());
            else if(reader.name() == "Transition")
              insertPropertyIf(plan, SIDTRANS, reader.readElementText());
            else if(reader.name() == "Type")
              insertPropertyIf(plan, SIDTYPE, reader.readElementText());
            else if(reader.name() == "CustomDistance")
              insertPropertyIf(plan, DEPARTURE_CUSTOM_DISTANCE, reader.readElementText());
            else
              xmlStream.skipCurrentElement(true /* warn */);
          }
        }
        else if(reader.name() == "STAR")
        {
          while(xmlStream.readNextStartElement())
          {
            if(reader.name() == "Name")
              insertPropertyIf(plan, STAR, reader.readElementText());
            else if(reader.name() == "Runway")
              insertPropertyIf(plan, STARRW, reader.readElementText());
            else if(reader.name() == "Transition")
              insertPropertyIf(plan, STARTRANS, reader.readElementText());
            else
              xmlStream.skipCurrentElement(true /* warn */);
          }
        }
        else if(reader.name() == "Approach")
        {
          while(xmlStream.readNextStartElement())
          {
            if(reader.name() == "Name")
              insertPropertyIf(plan, APPROACH, reader.readElementText());
            else if(reader.name() == "ARINC")
              insertPropertyIf(plan, APPROACH_ARINC, reader.readElementText());
            else if(reader.name() == "Runway")
              insertPropertyIf(plan, APPROACHRW, reader.readElementText());
            else if(reader.name() == "Type")
              insertPropertyIf(plan, APPROACHTYPE, reader.readElementText());
            else if(reader.name() == "Suffix")
              insertPropertyIf(plan, APPROACHSUFFIX, reader.readElementText());
            // Transition ========================================
            else if(reader.name() == "Transition")
              insertPropertyIf(plan, TRANSITION, reader.readElementText());
            else if(reader.name() == "TransitionType")
              insertPropertyIf(plan, TRANSITIONTYPE, reader.readElementText());
            // Custom approach data ========================================
            else if(reader.name() == "CustomDistance")
              insertPropertyIf(plan, APPROACH_CUSTOM_DISTANCE, reader.readElementText());
            else if(reader.name() == "CustomAltitude")
              insertPropertyIf(plan, APPROACH_CUSTOM_ALTITUDE, reader.readElementText());
            else if(reader.name() == "CustomOffsetAngle")
              insertPropertyIf(plan, APPROACH_CUSTOM_OFFSET, reader.readElementText());
            else
              xmlStream.skipCurrentElement(true /* warn */);
          }
        }
        else
          xmlStream.skipCurrentElement(true /* warn */);
      }
    }
    else if(reader.name() == "Waypoints")
      readWaypointsLnm(xmlStream, waypoints, "Waypoint");
    else
      xmlStream.skipCurrentElement(true /* warn */);
  }

  plan.append(waypoints);
  plan.adjustDepartureAndDestination();

  if(!plan.departurePos.isValid() && !plan.isEmpty())
    plan.departurePos = plan.constFirst().getPosition();

  // Add alternates to end of list
  plan.append(alternates);

  if(plan.isEmpty())
  {
    if(xmlStream.getFilename().isEmpty())
      throw Exception(tr("Invalid LNMPLN flight plan. No waypoints found."));
    else
      throw Exception(tr("Invalid LNMPLN flight plan file \"%1\". No waypoints found.").arg(xmlStream.getFilename()));
  }
}

void FlightplanIO::loadPln(atools::fs::pln::Flightplan& plan, const QString& filename)
{
  qDebug() << Q_FUNC_INFO << filename;

  plan.clearAll();

  QFile xmlFile(filename);
  if(xmlFile.open(QIODevice::ReadOnly))
  {
    atools::util::XmlStream xmlStream(&xmlFile, filename);
    QXmlStreamReader& reader = xmlStream.getReader();

    // Read header ===========================================================
    // Skip all the useless entries until we hit the document
    xmlStream.readUntilElement("SimBase.Document");
    xmlStream.readUntilElement("Descr");
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
            insertPropertyIf(plan, prop.section("=", 0, 0).trimmed(), prop.section("=", 1, 1).trimmed());
        }
      }
      if(reader.isStartElement())
        break;
    }
    // Skip all until the flightplan is found
    xmlStream.readUntilElement("FlightPlan.FlightPlan");
    int appVersionMajor = 0, appVersionBuild = 0;

    while(xmlStream.readNextStartElement())
    {
      QStringRef name = reader.name();
      // if(name == "Title")
      // plan.title = reader.readElementText();
      // else
      if(name == "FPType")
        plan.flightplanType = stringFlightplanType(reader.readElementText());
      else if(name == "CruisingAlt")
        plan.cruiseAltitudeFt = reader.readElementText().toFloat();
      else if(name == "DepartureID")
        plan.departureIdent = reader.readElementText();
      else if(name == "DepartureLLA")
      {
        QString txt = reader.readElementText();
        if(!txt.isEmpty())
          plan.departureParkingPos = geo::Pos(txt);
      }
      else if(name == "DestinationID")
        plan.destinationIdent = reader.readElementText();
      else if(name == "DestinationLLA")
      {
        QString txt = reader.readElementText();
        if(!txt.isEmpty())
          plan.destinationPos = geo::Pos(txt);
      }
      // else if(name == "Descr")
      // plan.description = reader.readElementText();
      else if(name == "DeparturePosition")
        plan.departureParkingName = reader.readElementText();
      else if(name == "DepartureName")
        plan.departureName = reader.readElementText();
      else if(name == "DestinationName")
        plan.destinationName = reader.readElementText();
      else if(name == "AppVersion")
        readAppVersionPln(appVersionMajor, appVersionBuild, xmlStream);
      else if(name == "ATCWaypoint")
        readWaypointPln(plan, xmlStream);
      else
        reader.skipCurrentElement();
    }

    xmlFile.close();

    if(!plan.isEmpty())
    {
      // Clear airway of departure airport to avoid problems from third party tools
      // like PFPX that abuse the airway name to add approach procedures
      plan.first().setAirway(QString());

      if(plan.size() > 1)
      {
        // Clear airway to first waypoint
        plan[1].setAirway(QString());

        if(plan.constLast().getWaypointType() == entry::AIRPORT)
          // Clear airway to destination
          plan.last().setAirway(QString());
      }

      // Collect MSFS procedure information from all legs ========================================
      QString sid, sidWp, sidRunway, sidRunwayDesignator, star, starWp, starWpPrev, starRunway, starRunwayDesignator,
              approach, approachSuffix, approachRunway, approachRunwayDesignator, approachTransition;
      for(int i = 0; i < plan.size(); i++)
      {
        FlightplanEntry& entry = plan[i];

        if(!entry.getSid().isEmpty() && plan.value(i + 1).getSid().isEmpty())
          // This is the last waypoint in a SID transition
          sidWp = entry.getIdent();

        if(!entry.getStar().isEmpty() && plan.value(i - 1).getStar().isEmpty())
        {
          // First waypoint in STAR transition
          starWp = entry.getIdent();

          // Need to catch previous waypoint as well due to broken SimBrief plans or plans having SID exit equal to STAR entry
          starWpPrev = plan.value(i - 1).getIdent();
        }

        if(!entry.getSid().isEmpty())
        {
          // Leg is part of a SID ==========
          sid = entry.getSid();
          sidRunway = entry.getRunwayNumber();
          sidRunwayDesignator = entry.getRunwayDesignator();
        }
        else if(!entry.getStar().isEmpty())
        {
          // Leg is part of a STAR ==========
          star = entry.getStar();
          starRunway = entry.getRunwayNumber();
          starRunwayDesignator = entry.getRunwayDesignator();
        }
        else if(!entry.getApproach().isEmpty())
        {
          // Leg is part of an approach ==========
          approach = entry.getApproach();
          approachSuffix = entry.getApproachSuffix();
          approachTransition = entry.getApproachTransition();
          approachRunway = entry.getRunwayNumber();
          approachRunwayDesignator = entry.getRunwayDesignator();
        }

        if(i == plan.size() - 1)
        {
          // Clear procedure information in destination airport to prevent deletion further down
          entry.setApproach(QString(), QString(), QString());
          entry.setRunway(QString(), QString());
        }
      }

      // Add MSFS procedure information to properties ========================================
      insertPropertyIf(plan, SID, sid);
      insertPropertyIf(plan, SIDTRANSWP, sidWp);
      insertPropertyIf(plan, SIDRW, sidRunway + strAt(sidRunwayDesignator, 0));

      insertPropertyIf(plan, STAR, star);
      insertPropertyIf(plan, STARTRANSWP, atools::strJoin(QStringList({starWp, starWpPrev}), PROPERTY_LIST_SEP));
      insertPropertyIf(plan, STARRW, starRunway + strAt(starRunwayDesignator, 0));

      // insertPropertyIf(plan, TRANSITIONTYPE, );
      // insertPropertyIf(plan, APPROACH, approach);
      // insertPropertyIf(plan, APPROACH_ARINC, approach);
      insertPropertyIf(plan, APPROACHTYPE, msfsToApproachType(approach));
      insertPropertyIf(plan, APPROACHSUFFIX, approachSuffix);

      if(!approachRunway.isEmpty())
        insertPropertyIf(plan, APPROACHRW, approachRunway + strAt(approachRunwayDesignator, 0));

      // Remove the procedure legs ============================
      plan.erase(std::remove_if(plan.begin(), plan.end(),
                                [ = ](const FlightplanEntry& entry) -> bool {
              return !entry.getSid().isEmpty() || !entry.getStar().isEmpty() || !entry.getApproach().isEmpty();
            }), plan.end());
    }
  }
  else
    throw Exception(tr("Cannot open file \"%1\". Reason: %2").arg(filename).arg(xmlFile.errorString()));
}

// <PropertyList>
// <version type="int">2</version>
// <departure>
// <airport type="string">KOAK</airport>
// <sid type="string">(none)</sid>
// <runway type="string">29</runway>
// </departure>
// <destination>
// <airport type="string">KSJC</airport>
// <star type="string">(none)</star>
// <transition type="string"></transition>
// <runway type="string">11</runway>
// </destination>
// <route>
// <wp>
// <type type="string">runway</type>
// <departure type="bool">true</departure>
// <generated type="bool">true</generated>
// <ident type="string">29</ident>
// <icao type="string">KOAK</icao>
// </wp>
void FlightplanIO::loadFlightGear(atools::fs::pln::Flightplan& plan, const QString& filename)
{
  qDebug() << Q_FUNC_INFO << filename;

  plan.clearAll();

  QFile xmlFile(filename);
  if(xmlFile.open(QIODevice::ReadOnly))
  {
    atools::util::XmlStream xmlStream(&xmlFile, filename);
    QXmlStreamReader& reader = xmlStream.getReader();

    QString departureIcao, departureRunway, sid, sidTransition,
            destinationIcao, destinationRunway, star, starTransition;
    float maxAlt = std::numeric_limits<float>::min();

    xmlStream.readUntilElement("PropertyList");

    while(xmlStream.readNextStartElement())
    {
      QStringRef name = reader.name();
      if(name == "version")
      {
        // Skip these elements without warning
        xmlStream.skipCurrentElement();
        continue;
      }

      if(name == "departure")
      {
        // Read sub elements for departure =====================================
        while(xmlStream.readNextStartElement())
        {
          QStringRef depname = reader.name();
          if(depname == "airport")
            departureIcao = reader.readElementText();
          else if(depname == "runway")
            departureRunway = reader.readElementText();
          else if(depname == "sid")
            sid = reader.readElementText();
          else if(depname == "transition")
            sidTransition = reader.readElementText();
          else
            reader.skipCurrentElement();
        }
      }
      else if(name == "destination")
      {
        // Read sub elements for destination =====================================
        while(xmlStream.readNextStartElement())
        {
          QStringRef destname = reader.name();
          if(destname == "airport")
            destinationIcao = reader.readElementText();
          else if(destname == "runway")
            destinationRunway = reader.readElementText();
          else if(destname == "star")
            star = reader.readElementText();
          else if(destname == "transition")
            starTransition = reader.readElementText();
          else
            reader.skipCurrentElement();
        }
      }
      else if(name == "route")
      {
        // Read wp elements for route =====================================
        while(xmlStream.readNextStartElement())
        {
          FlightplanEntry entry;

          QStringRef destname = reader.name();
          if(destname == "wp")
          {
            QString wptype, wpicao, wpident, wplon, wplat, wpalt;

            while(xmlStream.readNextStartElement())
            {
              QStringRef wpname = reader.name();

              if(wpname == "type")
                wptype = reader.readElementText();
              else if(wpname == "icao")
                wpicao = reader.readElementText();
              else if(wpname == "ident")
                wpident = reader.readElementText();
              else if(wpname == "lon")
                wplon = reader.readElementText();
              else if(wpname == "lat")
                wplat = reader.readElementText();
              else if(wpname == "altitude-ft")
                wpalt = reader.readElementText();
              else
                reader.skipCurrentElement();
            }

            bool altOk;
            float altitude = wpalt.toFloat(&altOk);
            if(!altOk || altitude < atools::fs::pln::FLIGHTPLAN_ALTITUDE_FT_MIN || altitude > atools::fs::pln::FLIGHTPLAN_ALTITUDE_FT_MAX)
            {
              // Avoid excessive altitudes
              qWarning() << Q_FUNC_INFO << "Invalid altitude";
              altitude = 0.f;
            }

            maxAlt = std::max(maxAlt, altitude);

            Pos position(wplon, wplat, altitude);
            if(position.isValid())
              entry.setPosition(position);

            if(wptype == "runway")
            {
              // Runway entry for airport =================================================
              QString id = wpicao.isEmpty() ? wpident : wpicao;
              entry.setIdent(id);
              plan.append(entry);
            }
            else if(wptype == "navaid")
            {
              // Normal navaid =================================
              entry.setIdent(wpident);
              plan.append(entry);
            }
            else if(wptype == "basic")
            {
              // Basic waypoint: only lat/lon tags are meaningfull
              // (might be a custom waypoint, or navaid missing from database)
              if(position.isValid())
              {
                entry.setIdent(wpident);
                entry.setWaypointType(entry::USER);
                plan.append(entry);
              }
            }
          }
          else
            reader.skipCurrentElement();
        }
      }
      else
        reader.skipCurrentElement();
    }

    if(!plan.isEmpty())
    {
      // Correct start and destination entry types ================================================
      plan.first().setWaypointType(atools::fs::pln::entry::AIRPORT);
      plan.last().setWaypointType(atools::fs::pln::entry::AIRPORT);
    }

    plan.setDepartureIdent(departureIcao);
    plan.setDestinationIdent(destinationIcao);

    // Set departure procedure =========================================================
    if(!departureRunway.isEmpty())
      plan.getProperties().insert(SIDRW, departureRunway);
    if(!sid.isEmpty())
      plan.getProperties().insert(SID, sid);
    if(!sidTransition.isEmpty())
      plan.getProperties().insert(SIDTRANS, sidTransition);

    // Set arrival procedure =========================================================
    if(!destinationRunway.isEmpty())
      plan.getProperties().insert(STARRW, destinationRunway);
    if(!star.isEmpty())
      plan.getProperties().insert(STAR, star);
    if(!starTransition.isEmpty())
      plan.getProperties().insert(STARTRANS, starTransition);

    xmlFile.close();

    plan.flightplanType = atools::fs::pln::NO_TYPE;
    plan.cruiseAltitudeFt = maxAlt > atools::fs::pln::FLIGHTPLAN_ALTITUDE_FT_MIN ? maxAlt : 0.f; // Use value from GUI
    plan.adjustDepartureAndDestination();
    plan.assignAltitudeToAllEntries();
  }
  else
    throw Exception(tr("Cannot open FlightGear file \"%1\". Reason: %2").arg(filename).arg(xmlFile.errorString()));
}

void FlightplanIO::writeTextElementIf(QXmlStreamWriter& writer, const QString& name, const QString& value)
{
  if(!value.isEmpty())
    writer.writeTextElement(name, value);
}

void FlightplanIO::writeElementPosIf(QXmlStreamWriter& writer, const atools::geo::Pos& pos)
{
  if(pos.isValid())
  {
    writer.writeStartElement("Pos");
    writer.writeAttribute("Lon", QString::number(pos.getLonX(), 'f', 6));
    writer.writeAttribute("Lat", QString::number(pos.getLatY(), 'f', 6));
    writer.writeAttribute("Alt", QString::number(pos.getAltitude(), 'f', 2));
    writer.writeEndElement(); // Pos
  }
  else
  {
    QFileDevice *df = dynamic_cast<QFileDevice *>(writer.device());
    QString filename = df != nullptr ? df->fileName() : QString();
    qWarning() << Q_FUNC_INFO << "Position invalid" << "in file" << filename;
  }
}

void FlightplanIO::writeWaypointLnm(QXmlStreamWriter& writer, const FlightplanEntry& entry, const QString& elementName)
{
  writer.writeStartElement(elementName);
  writeTextElementIf(writer, "Name", entry.getName());
  writeTextElementIf(writer, "Ident", entry.getIdent());
  writeTextElementIf(writer, "Region", entry.getRegion());

  if(entry.isAirway())
    writeTextElementIf(writer, "Airway", entry.getAirway());
  else if(entry.isTrack())
    writeTextElementIf(writer, "Track", entry.getAirway());

  writeTextElementIf(writer, "Type", entry.getWaypointTypeAsLnmString());
  writeTextElementIf(writer, "Comment", entry.getComment());
  writeElementPosIf(writer, entry.getPosition());
  writer.writeEndElement(); // elementName
}

QByteArray FlightplanIO::saveLnmGz(const Flightplan& plan)
{
  return atools::zip::gzipCompress(saveLnmStr(plan).toUtf8());
}

QString FlightplanIO::saveLnmStr(const Flightplan& plan)
{
  QString lnmString;
  QXmlStreamWriter writer(&lnmString);
  saveLnmInternal(writer, plan);
  return lnmString;
}

void FlightplanIO::saveLnm(const Flightplan& plan, const QString& filename)
{
  QFile xmlFile(filename);
  if(xmlFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QXmlStreamWriter writer(&xmlFile);
    saveLnmInternal(writer, plan);
    xmlFile.close();
  }
  else
    throw Exception(errorMsg.arg(filename).arg(xmlFile.errorString()));
}

void FlightplanIO::saveLnmInternal(QXmlStreamWriter& writer, const Flightplan& plan)
{
  writer.setCodec("UTF-8");
  writer.setAutoFormatting(true);
  writer.setAutoFormattingIndent(2);

  writer.writeStartDocument("1.0");
  writer.writeStartElement("LittleNavmap");

  // Schema namespace and reference to XSD ======================
  writer.writeAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
  writer.writeAttribute("xsi:noNamespaceSchemaLocation", "https://www.littlenavmap.org/schema/lnmpln.xsd");

  writer.writeStartElement("Flightplan");

  // Save header and metadata =======================================================
  writer.writeStartElement("Header");
  writeTextElementIf(writer, "FlightplanType", flightplanTypeToString(plan.flightplanType));
  writeTextElementIf(writer, "CruisingAlt", QString::number(atools::roundToInt(plan.cruiseAltitudeFt)));
  writeTextElementIf(writer, "CruisingAltF", QString::number(plan.cruiseAltitudeFt, 'f', 8));
  writeTextElementIf(writer, "Comment", plan.comment);
  writeTextElementIf(writer, "CreationDate", atools::currentIsoWithOffset(false /* milliseconds */));
  writeTextElementIf(writer, "FileVersion", QString("%1.%2").arg(LNMPLN_VERSION_MAJOR).arg(LNMPLN_VERSION_MINOR));
  writeTextElementIf(writer, "ProgramName", QCoreApplication::applicationName());
  writeTextElementIf(writer, "ProgramVersion", QCoreApplication::applicationVersion());
  writeTextElementIf(writer, "Documentation", "https://www.littlenavmap.org/lnmpln.html");
  writer.writeEndElement(); // Header

  // Nav and sim metadata =======================================================
  QString cycle = plan.properties.value(SIMDATACYCLE);
  QString data = plan.properties.value(SIMDATA);
  if(!data.isEmpty())
  {
    writer.writeStartElement("SimData");
    if(!cycle.isEmpty())
      writer.writeAttribute("Cycle", plan.properties.value(SIMDATACYCLE));
    writer.writeCharacters(data);
    writer.writeEndElement(); // SimData
  }
  cycle = plan.properties.value(NAVDATACYCLE);
  data = plan.properties.value(NAVDATA);
  if(!data.isEmpty())
  {
    writer.writeStartElement("NavData");
    if(!cycle.isEmpty())
      writer.writeAttribute("Cycle", plan.properties.value(NAVDATACYCLE));
    writer.writeCharacters(data);
    writer.writeEndElement(); // NavData
  }

  // Aircraft performance =======================================================
  if(!plan.properties.value(AIRCRAFT_PERF_FILE).isEmpty() || !plan.properties.value(AIRCRAFT_PERF_TYPE).isEmpty() ||
     !plan.properties.value(AIRCRAFT_PERF_NAME).isEmpty())
  {
    writer.writeStartElement("AircraftPerformance");
    writeTextElementIf(writer, "FilePath", QFileInfo(plan.properties.value(AIRCRAFT_PERF_FILE)).fileName());
    writeTextElementIf(writer, "Type", plan.properties.value(AIRCRAFT_PERF_TYPE));
    writeTextElementIf(writer, "Name", plan.properties.value(AIRCRAFT_PERF_NAME));
    writer.writeEndElement(); // AircraftPerformance
  }

  // Departure name and position =======================================================
  if(!plan.departureParkingName.isEmpty())
  {
    writer.writeStartElement("Departure");
    writeElementPosIf(writer, plan.departureParkingPos);
    writeTextElementIf(writer, "Start", plan.departureParkingName);
    writeTextElementIf(writer, "Type", plan.getDepartureParkingTypeStr());
    if(plan.departureParkingHeading < INVALID_HEADING)
      writeTextElementIf(writer, "Heading", QString::number(plan.departureParkingHeading));
    writer.writeEndElement(); // Departure
  }

  // Procedures =======================================================
  if(!plan.properties.value(SID).isEmpty() || !plan.properties.value(STAR).isEmpty() ||
     !plan.properties.value(APPROACH).isEmpty() || !plan.properties.value(DEPARTURE_CUSTOM_DISTANCE).isEmpty())
  {
    writer.writeStartElement("Procedures");
    if(!plan.properties.value(SID).isEmpty() || !plan.properties.value(DEPARTURE_CUSTOM_DISTANCE).isEmpty())
    {
      writer.writeStartElement("SID");
      writeTextElementIf(writer, "Name", plan.properties.value(SID));
      writeTextElementIf(writer, "Runway", plan.properties.value(SIDRW));
      writeTextElementIf(writer, "Transition", plan.properties.value(SIDTRANS));
      writeTextElementIf(writer, "Type", plan.properties.value(SIDTYPE));
      writeTextElementIf(writer, "CustomDistance", plan.properties.value(DEPARTURE_CUSTOM_DISTANCE));
      writer.writeEndElement(); // SID
    }

    if(!plan.properties.value(STAR).isEmpty())
    {
      writer.writeStartElement("STAR");
      writeTextElementIf(writer, "Name", plan.properties.value(STAR));
      writeTextElementIf(writer, "Runway", plan.properties.value(STARRW));
      writeTextElementIf(writer, "Transition", plan.properties.value(STARTRANS));
      writer.writeEndElement(); // STAR
    }

    if(!plan.properties.value(APPROACH).isEmpty())
    {
      writer.writeStartElement("Approach");
      writeTextElementIf(writer, "Name", plan.properties.value(APPROACH));
      writeTextElementIf(writer, "ARINC", plan.properties.value(APPROACH_ARINC));
      writeTextElementIf(writer, "Runway", plan.properties.value(APPROACHRW));
      writeTextElementIf(writer, "Type", plan.properties.value(APPROACHTYPE));
      writeTextElementIf(writer, "Suffix", plan.properties.value(APPROACHSUFFIX));

      // Transition =============================================
      writeTextElementIf(writer, "Transition", plan.properties.value(TRANSITION));
      writeTextElementIf(writer, "TransitionType", plan.properties.value(TRANSITIONTYPE));

      // Custom approach data =============================================
      writeTextElementIf(writer, "CustomDistance", plan.properties.value(APPROACH_CUSTOM_DISTANCE));
      writeTextElementIf(writer, "CustomAltitude", plan.properties.value(APPROACH_CUSTOM_ALTITUDE));
      writeTextElementIf(writer, "CustomOffsetAngle", plan.properties.value(APPROACH_CUSTOM_OFFSET));
      writer.writeEndElement(); // Approach
    }
    writer.writeEndElement(); // Procedures
  }

  // Alternates =======================================================
  // First collect all alternates to check if there are any
  QVector<const FlightplanEntry *> alternates = plan.getAlternates();
  if(!alternates.isEmpty())
  {
    writer.writeStartElement("Alternates");

    if(!alternates.isEmpty())
    {
      // Write alternates with full information
      for(const FlightplanEntry *entry : alternates)
        writeWaypointLnm(writer, *entry, "Alternate");
    }
    writer.writeEndElement(); // Alternates
  }

  // Waypoints =======================================================
  writer.writeStartElement("Waypoints");
  for(const FlightplanEntry& entry : plan)
  {
    // Write all as waypoints except procedures and alternates
    if(!entry.isNoSave())
      writeWaypointLnm(writer, entry, "Waypoint");
  }
  writer.writeEndElement(); // Waypoints

  writer.writeEndElement(); // Flightplan
  writer.writeEndElement(); // LittleNavmap
  writer.writeEndDocument();
}

void FlightplanIO::savePln(const Flightplan& plan, const QString& file)
{
  savePlnInternal(plan, file, false /* annotated */, false /* msfs */, 10 /* userWpLength */);
}

void FlightplanIO::savePlnMsfs(const Flightplan& plan, const QString& file)
{
  savePlnInternal(plan, file, false /* annotated */, true /* msfs */, 80 /* userWpLength */);
}

void FlightplanIO::savePlnIsg(const Flightplan& plan, const QString& file)
{
  savePlnInternal(plan, file, false /* annotated */, false /* msfs */, 12 /* userWpLength */);
}

void FlightplanIO::savePlnAnnotated(const Flightplan& plan, const QString& file)
{
  savePlnInternal(plan, file, true /* annotated */, false /* msfs */, 10 /* userWpLength */);
}

void FlightplanIO::savePlnInternal(const Flightplan& plan, const QString& filename, bool annotated, bool msfs, int userWpLength)
{
  // Write XML to string first ===================
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

  if(annotated)
  {
    QStringList comment;
    for(auto it = plan.properties.constBegin(); it != plan.properties.constEnd(); ++it)
    {
      const QString& key = it.key();
      if(key == "_lnm")
        continue;

      if(!key.isEmpty())
        comment.append("\n         " + key + "=" + it.value());
    }

    std::sort(comment.begin(), comment.end());

    comment.prepend("\n         _lnm=" + atools::programFileInfo());

    writer.writeComment(" LNMDATA" + comment.join("|") + "\n");
  }

  writer.writeStartElement("FlightPlan.FlightPlan");

  writer.writeTextElement("Title", plan.getTitle());
  writer.writeTextElement("FPType", flightplanTypeToString(plan.flightplanType));

  writer.writeTextElement("RouteType", routeTypeToString(plan.getRouteType()));

  writer.writeTextElement("CruisingAlt", QString::number(atools::roundToInt(plan.cruiseAltitudeFt)));
  writer.writeTextElement("DepartureID", plan.departureIdent);

  // Use parking position
  writer.writeTextElement("DepartureLLA",
                          plan.getDepartureParkingPosition().isValid() ?
                          plan.getDepartureParkingPosition().toLongString() : QString());

  writer.writeTextElement("DestinationID", plan.destinationIdent);
  writer.writeTextElement("DestinationLLA",
                          plan.destinationPos.isValid() ? plan.destinationPos.toLongString() : QString());
  writer.writeTextElement("Descr", plan.getDescr());

  QString parking;
  if(msfs)
  {
    switch(plan.departureParkingType)
    {
      case atools::fs::pln::AIRPORT:
      case atools::fs::pln::NO_POS:
        break;

      case atools::fs::pln::HELIPAD:
      case atools::fs::pln::RUNWAY:
        parking = plan.departureParkingName;
        if(parking.startsWith('0'))
          parking = parking.mid(1);
        break;

      case atools::fs::pln::PARKING:
        parking = plan.departureParkingName;
        break;
    }
  }
  else
    parking = plan.departureParkingName;

  if(!parking.isEmpty())
    writer.writeTextElement("DeparturePosition", parking);

  writer.writeTextElement("DepartureName", plan.departNameOrIdent());
  writer.writeTextElement("DestinationName", plan.destNameOrIdent());

  writer.writeStartElement("AppVersion");
  writer.writeTextElement("AppVersionMajor", msfs ? "11" : "10");
  writer.writeTextElement("AppVersionBuild", msfs ? "282174" : "61472");
  writer.writeEndElement(); // AppVersion

  int i = 0;
  for(const FlightplanEntry& entry : plan)
  {
    if(entry.isNoSave())
      // Do not save entries like procedure points
      continue;

    writer.writeStartElement("ATCWaypoint");

    // Trim to max allowed length for FSX/P3D and remove any special chars otherwise FSX/P3D will ignore the plan
    if(msfs)
      writer.writeAttribute("id", atools::fs::util::adjustMsfsUserWpName(entry.getIdent(), userWpLength));
    else
      writer.writeAttribute("id", atools::fs::util::adjustFsxUserWpName(entry.getIdent(), userWpLength));

    writer.writeTextElement("ATCWaypointType", entry.getWaypointTypeAsFsxString());

    if(!entry.getPosition().isValid())
      throw atools::Exception("Invalid position in flightplan for id " + entry.getIdent());

    writer.writeTextElement("WorldPosition", entry.getPosition().toLongString());

    writeTextElementIf(writer, "ATCAirway", entry.getAirway());

    if(msfs && i > 0)
    {
      // Write additional procedure information for MSFS but not for departure airport
      writeTextElementIf(writer, "DepartureFP", entry.getSid());
      writeTextElementIf(writer, "ArrivalFP", entry.getStar());
      writeTextElementIf(writer, "SuffixFP", entry.getApproachSuffix());
      writeTextElementIf(writer, "ApproachTypeFP", approachToMsfs(entry.getApproach()));
      writeTextElementIf(writer, "RunwayNumberFP", entry.getRunwayNumber());
      writeTextElementIf(writer, "RunwayDesignatorFP", entry.getRunwayDesignator());
    }

    bool regionOrIdent = !entry.getRegion().isEmpty() || !entry.getIdent().isEmpty();
    bool writeIdent;
    if(msfs)
      // For MSFS also required for userpoints to avoid waypoints moved to north
      writeIdent = regionOrIdent;
    else
      // Other simulators do not need this for user waypoints
      writeIdent = entry.getWaypointType() != atools::fs::pln::entry::USER && regionOrIdent;

    if(writeIdent)
    {
      writer.writeStartElement("ICAO");

      if(!msfs || entry.getWaypointType() != atools::fs::pln::entry::AIRPORT)
        // Avoid region since it is not reliable for airports in MSFS and
        // the sim garbles the flight plan when loading
        writeTextElementIf(writer, "ICAORegion", entry.getRegion());

      writeTextElementIf(writer, "ICAOIdent", entry.getIdent());

      if(msfs)
        // Write airport for waypoint if available
        writeTextElementIf(writer, "ICAOAirport", entry.getAirport());

      writer.writeEndElement(); // ICAO
    }

    writer.writeEndElement(); // ATCWaypoint
    i++;
  } // for(const FlightplanEntry& entry : plan)

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

  // Write XML to file ===================
  QFile xmlFile(filename);
  if(xmlFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QTextStream stream(&xmlFile);
    stream.setCodec("UTF-8");
    stream.setGenerateByteOrderMark(true);
    stream << xmlString.toUtf8();
    xmlFile.close();
  }
  else
    throw Exception(errorMsg.arg(filename).arg(xmlFile.errorString()));
}

QString FlightplanIO::approachToMsfs(const QString& type)
{
  if(type == "LOC")
    return "LOCALIZER";
  else if(type == "LOCB")
    return "LOCALIZER_BACK_COURSE";
  else
    // GPS (not saved by MSFS), VOR, VORDME, RNAV, NDBDME, NDB, ILS
    // TACAN not supported
    return type;
}

QString FlightplanIO::msfsToApproachType(const QString& type)
{
  if(type == "LOCALIZER")
    return "LOC";
  else if(type == "LOCALIZER_BACK_COURSE")
    return "LOCB";
  else
    return type;
}

/*
 *  <?xml version="1.0"?>
 *  <PropertyList>
 *  <version type="int">2</version>
 *  <departure>
 *   <airport type="string">KOAK</airport>
 *   <sid type="string">(none)</sid>
 *   <runway type="string">29</runway>
 *  </departure>
 *  <destination>
 *   <airport type="string">KSJC</airport>
 *   <star type="string">(none)</star>
 *   <transition type="string"></transition>
 *   <runway type="string">11</runway>
 *  </destination>
 *  <route>
 *   <wp>
 *     <type type="string">runway</type>
 *     <departure type="bool">true</departure>
 *     <generated type="bool">true</generated>
 *     <ident type="string">29</ident>
 *     <icao type="string">KOAK</icao>
 *   </wp>
 *   <wp n="1">
 *     <type type="string">offset-navaid</type>
 *     <alt-restrict type="string">at</alt-restrict>
 *     <altitude-ft type="double">7500</altitude-ft>
 *     <ident type="string">SFO</ident>
 *     <lon type="double">-122.3738889</lon>
 *     <lat type="double">37.61947222</lat>
 *     <radial-deg type="double">88.22972768</radial-deg>
 *     <distance-nm type="double">16</distance-nm>
 *   </wp>
 *   <wp n="2">
 *     <type type="string">navaid</type>
 *     <alt-restrict type="string">at</alt-restrict>
 *     <altitude-ft type="double">10000</altitude-ft>
 *     <ident type="string">MISON</ident>
 *     <lon type="double">-121.890306</lon>
 *     <lat type="double">37.496806</lat>
 *   </wp>
 *   <wp n="3">
 *     <type type="string">runway</type>
 *     <arrival type="bool">true</arrival>
 *     <generated type="bool">true</generated>
 *     <ident type="string">11</ident>
 *     <icao type="string">KSJC</icao>
 *   </wp>
 *  </route>
 *  </PropertyList>
 */
void FlightplanIO::saveFlightGear(const Flightplan& plan, const QString& filename)
{
  QFile xmlFile(filename);

  if(xmlFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QXmlStreamWriter writer(&xmlFile);
    writer.setCodec("UTF-8");
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(2);

    writer.writeStartDocument("1.0");
    writer.writeStartElement("PropertyList");
    writePropertyInt(writer, "version", 2);

    // <source type="string">https://flightplandatabase.com/plan/1551256</source>
    writePropertyStr(writer, "source", atools::programFileInfo());

    writer.writeStartElement("departure");
    writePropertyStr(writer, "airport", plan.getDepartureIdent());

    // Writer departure procedure information ===============================================================
    if(!plan.properties.value(SIDRW).isEmpty())
      writePropertyStr(writer, "runway", plan.properties.value(SIDRW));
    if(!plan.properties.value(SID).isEmpty())
      writePropertyStr(writer, "sid", plan.properties.value(SID));
    if(!plan.properties.value(SIDTRANS).isEmpty())
      writePropertyStr(writer, "transition", plan.properties.value(SIDTRANS));

    writer.writeEndElement(); // departure

    // Writer arrival procedure information ===============================================================
    writer.writeStartElement("destination");
    writePropertyStr(writer, "airport", plan.getDestinationIdent());

    if(!plan.properties.value(STARRW).isEmpty())
      writePropertyStr(writer, "runway", plan.properties.value(STARRW));
    if(!plan.properties.value(STAR).isEmpty())
      writePropertyStr(writer, "star", plan.properties.value(STAR));
    if(!plan.properties.value(STARTRANS).isEmpty())
      writePropertyStr(writer, "transition", plan.properties.value(STARTRANS));

    writer.writeEndElement(); // destination

    // route ===================================================================================
    writer.writeStartElement("route");

    int index = 0;
    for(int i = 0; i < plan.size(); i++)
    {
      const FlightplanEntry& entry = plan.at(i);

      if(entry.isNoSave())
        // Do not save procedure points
        continue;

      writer.writeStartElement("wp");
      writer.writeAttribute("n", QString::number(index++));

      bool hasProcedure = false;
      if(i == 0)
      {
        // Departure airport ===========================================
        // <departure type="bool">true</departure>
        writePropertyBool(writer, "departure");
        if(!plan.properties.value(SIDRW).isEmpty())
        {
          // <type type="string">runway</type>
          // <ident type="string">07R</ident>
          // <icao type="string">EDDF</icao>
          writePropertyStr(writer, "type", "runway");
          writePropertyStr(writer, "ident", plan.properties.value(SIDRW));
          writePropertyStr(writer, "icao", entry.getIdent());
          hasProcedure = true;
        }
      }
      else if(i == plan.size() - 1)
      {
        // Destination airport ===========================================
        // <approach type="bool">true</approach>
        writePropertyBool(writer, "approach");

        if(!plan.properties.value(STARRW).isEmpty())
        {
          // <type type="string">runway</type>
          // <ident type="string">07</ident>
          // <icao type="string">LIRF</icao>
          writePropertyStr(writer, "type", "runway");
          writePropertyStr(writer, "ident", plan.properties.value(STARRW));
          writePropertyStr(writer, "icao", entry.getIdent());
          hasProcedure = true;
        }
      }

      if(!hasProcedure)
      {
        if(entry.getWaypointType() == entry::USER)
          writePropertyStr(writer, "type", "basic");
        else
          writePropertyStr(writer, "type", "navaid");
      }

      if(i > 0 && i < plan.size() - 1)
      {
        // Write altitude except for first and last =====================
        // <alt-restrict type="string">at</alt-restrict>
        writePropertyStr(writer, "alt-restrict", "at");

        // <altitude-ft type="double">19100</altitude-ft>
        writer.writeStartElement("altitude-ft");
        writer.writeAttribute("type", "double");
        writer.writeCharacters(QString::number(atools::roundToInt(entry.getPosition().getAltitude())));
        writer.writeEndElement();
      }

      if(!hasProcedure)
        writePropertyStr(writer, "ident", entry.getIdent());

      // <lon type="double">8.541722452</lon>
      // <lat type="double">50.03219323</lat>
      writePropertyFloat(writer, "lon", entry.getPosition().getLonX());
      writePropertyFloat(writer, "lat", entry.getPosition().getLatY());

      writer.writeEndElement(); // wp
    }

    writer.writeEndElement(); // route
    writer.writeEndElement(); // PropertyList
    writer.writeEndDocument();

    xmlFile.close();
  }
  else
    throw Exception(errorMsg.arg(filename).arg(xmlFile.errorString()));
}

void FlightplanIO::saveCrjFlp(const atools::fs::pln::Flightplan& plan, const QString& filename)
{
  saveFlpInternal(plan, filename, true /* CRJ */, false /* MSFS */);
}

void FlightplanIO::saveMsfsCrjFlp(const atools::fs::pln::Flightplan& plan, const QString& filename)
{
  saveFlpInternal(plan, filename, true /* CRJ */, true /* MSFS */);
}

void FlightplanIO::saveFlp(const atools::fs::pln::Flightplan& plan, const QString& filename)
{
  saveFlpInternal(plan, filename, false /* CRJ */, false /* MSFS */);
}

// =========================================================================
// [CoRte]
// ArptDep=EDDH
// RwyDep=EDDH33
// ArptArr=LIRF
// RwyArr=LIRF34R
// ArptAltn=LIRN
// SID=IDEK5B
// SIDEnrTrans=IDEKO
// STAR=TAQ1C
// EnrSTARTrans=TAQ
// STARApprTrans=
// APPR_Trans=VECTORS
// RwyArrFINAL=I34R
// CoRoute=
// FltNo=N275SB
// DctWpt1=ABMAL
// DctWpt1Coordinates=53.442069,10.911981
// DctWpt2=IRKIS
// DctWpt2Coordinates=53.248889,10.888611
// ...
// DctWpt38=GIKIN
// DctWpt38Coordinates=42.618333,12.048611
// DctWpt39=TAQ
// DctWpt39Coordinates=42.215056,11.732611
//
// [PerfData]
// CrzAlt=33000
// CrzAltAltn=17000
// PaxCnt=128
// PaxWeight=175
// CargoWeight=7040
// FuelWeight=21677
// WindDirClb=274
// WindSpdClb=37
// WindDirCrz=277
// WindSpdCrz=41
// WindDirDes=290
// WindSpdDes=16
// ISADev=5
// ResFuel=2710
// TaxiFuel=840
//
// [VNAVData]
// TransAlt=5000
// TransLvl=18000

// =========================================================================
// [CoRte]
// ArptDep=EDDL
// RwyDep=EDDL23L
// ArptArr=EDDM
// RwyArr=EDDM08L
// ArptAltn=
// SID=DODE8T
// SIDEnrTrans=(optional)
// STAR=ANOR3A
// EnrSTARTrans=(optional)
// STARApprTrans=(optional)
// APPR_Trans=VECTORS
// RwyArrFINAL=I08L
// CoRoute=(optional)
// FltNo=
// Airway1=Y853
// Airway1FROM=DODEN
// Airway1TO=BOMBI
// Airway2=T104
// Airway2FROM=BOMBI
// Airway2TO=LEVBU
//
// All values except the wind data default to -1 if not set. In this case, the FMS defaults will be loaded.
// [PerfData]
// CrzAlt=27000
// CrzAltAltn=-1
// PaxCnt=56
// PaxWeight=185
// CargoWeight=1089
// FuelWeight=7408
// WindDirClb=0
// WindSpdClb=0
// WindDirCrz=0
// WindSpdCrz=0
// WindDirDes=0
// WindSpdDes=0
// ISADev=0
// ResFuel=1650
// TaxiFuel=0
//
// [VNAVData]
// TransAlt=18000
// TransLvl=18000

void FlightplanIO::saveFlpKeyValue(QTextStream& stream, const atools::fs::pln::Flightplan& plan, const QString& prefix,
                                   const QString& key, const QString& property)
{
  stream << key << "=";

  if(!plan.properties.value(property).isEmpty())
  {
    if(!prefix.isEmpty())
      stream << prefix;
    stream << plan.properties.value(property);
  }
  stream << endl;
}

void FlightplanIO::saveFlpInternal(const atools::fs::pln::Flightplan& plan, const QString& filename, bool crj, bool msfs)
{
  QFile flpFile(filename);
  bool alternateFmt = crj | msfs;

  if(flpFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QTextStream stream(&flpFile);
    stream.setCodec("UTF-8");

    Flightplan pln(plan);
    pln = pln.compressedAirways();

    // CoRte ==============================================
    stream << "[CoRte]" << endl;
    stream << "ArptDep=" << pln.departureIdent << endl;
    saveFlpKeyValue(stream, pln, pln.departureIdent, "RwyDep", SIDRW);

    stream << "ArptArr=" << pln.destinationIdent << endl;
    saveFlpKeyValue(stream, pln, pln.destinationIdent, "RwyArr", APPROACHRW);

    if(alternateFmt)
    {
      stream << "ArptAltn=";

      QVector<const atools::fs::pln::FlightplanEntry *> alternates = pln.getAlternates();
      if(!alternates.isEmpty())
        stream << alternates.constFirst()->getIdent();
      stream << endl;
    }

    // Departure - SID ============================================
    saveFlpKeyValue(stream, pln, QString(), "SID", SID);
    saveFlpKeyValue(stream, pln, QString(), alternateFmt ? "SIDEnrTrans" : "SID_Trans", SIDTRANS);

    // Arrival STAR ============================================
    saveFlpKeyValue(stream, pln, QString(), "STAR", STAR);
    saveFlpKeyValue(stream, pln, QString(), alternateFmt ? "EnrSTARTrans" : "STAR_Trans", STARTRANS);
    if(alternateFmt)
      stream << "STARApprTrans=" << endl;

    // Arrival approach and transition ============================================
    saveFlpKeyValue(stream, pln, QString(), alternateFmt ? "Appr_Trans" : "APPR_Trans", TRANSITION);
    saveFlpKeyValue(stream, pln, QString(), "RwyArrFinal", APPROACH_ARINC);

    if(alternateFmt)
    {
      stream << "CoRoute=" << endl;
      stream << "FltNo=" << endl;
    }

    QString lastAirwayTo;
    int index = 1;
    for(int i = 1; i < pln.size() - 1; i++)
    {
      const FlightplanEntry& entry = pln.at(i);
      if(entry.isNoSave())
        // Do not save entries like procedure points
        continue;

      if(pln.at(i + 1).isAlternate())
        break;

      const FlightplanEntry& nextEntry = pln.at(i + 1);
      QString airway = nextEntry.getAirway();

      // .      FROM     =         TO
      // ... -> entry -> airway -> nextEntry -> ...
      if(!airway.isEmpty())
      {
        stream << "Airway" << index << "=" << airway << endl;
        stream << "Airway" << index << "FROM=" << entry.getIdent() << endl;
        stream << "Airway" << index << "TO=" << nextEntry.getIdent() << endl;
        lastAirwayTo = nextEntry.getIdent();
        index++;
      }
      else if(entry.getIdent() != lastAirwayTo)
      {
        QString coords = QString("%1,%2").
                         arg(entry.getPosition().getLatY(), 0, 'f', 6).
                         arg(entry.getPosition().getLonX(), 0, 'f', 6);

        stream << "DctWpt" << index << "=" << identOrDegMinFormat(entry) << endl;
        stream << "DctWpt" << index << "Coordinates=" << coords << endl;
        lastAirwayTo.clear();
        index++;
      }
    }

    if(alternateFmt)
    {
      // PerfData ==============================================

      // The PerfData section will only be used if “Load PERF INIT Data with Flightplan” is enabled on the
      // Options page in CRJ Manager. Otherwise it will be ignored when the route is loaded.
      // All values except the wind data default to -1 if not set. In this case, the FMS defaults will be loaded.

      stream << endl;
      stream << "[PerfData]" << endl;
      stream << "CrzAlt=" << atools::roundToInt(plan.cruiseAltitudeFt) << endl; // Cruise Altitude (in feet)
      stream << "CrzAltAltn=-1" << endl; // Cruise Altitude to Alternate Airport (in feet)
      stream << "PaxCnt=-1" << endl; // Passenger count
      stream << "PaxWeight=-1" << endl; // Average Passenger weight (lbs)
      stream << "CargoWeight=-1" << endl; // Cargo weight (lbs)
      stream << "FuelWeight=-1" << endl; // Fuel weight (lbs)
      stream << "WindDirClb=0" << endl; // Wind direction during climb (0-359 degrees)
      stream << "WindSpdClb=0" << endl; // Wind speed during climb
      stream << "WindDirCrz=0" << endl; // Wind direction during cruise (0-359 degrees)
      stream << "WindSpdCrz=0" << endl; // Wind speed during cruise
      stream << "WindDirDes=0" << endl; // Wind direction during descent (0-359 degrees)
      stream << "WindSpdDes=0" << endl; // Wind speed during descent
      stream << "ISADev=-1" << endl; // Deviation from ISA temperature (degrees Celsius)
      stream << "ResFuel=-1" << endl; // Reserve fuel weight (lbs)
      stream << "TaxiFuel=-1" << endl; // Taxi fuel weight (lbs)

      // VNAVData ==============================================
      stream << endl;
      stream << "[VNAVData]" << endl;
      stream << "TransAlt=18000" << endl;
      stream << "TransLvl=18000" << endl;
    }

    flpFile.close();
  }
  else
    throw Exception(errorMsg.arg(filename).arg(flpFile.errorString()));
}

// Feelthere PLN=EHRDLEPA01.nz2k.fpl
//
// ;Generated by "Name of Program" on 06 Jan 2018 at 15:45Z
// [Origin]
// ident=LIRN
// type=1
// latitude=40.884444
// longitude=14.290833
// [Destination]
// ident=GMMN
// type=1
// latitude=33.364167
// longitude=-7.581667
// [Route]
// gspd=377
// countOfPoints=29
// Wpt.00.ident=ISKIA
// Wpt.00.type=4
// Wpt.00.latitude=40.690833
// Wpt.00.longitude=13.890833
// Wpt.01.ident=PNZ
// Wpt.01.type=4
// Wpt.01.latitude=40.911944
// Wpt.01.longitude=12.957500
// Wpt.02.ident=ROTUN
// Wpt.02.type=4
// Wpt.02.latitude=40.883333
// Wpt.02.longitude=12.305556
// ...
// Wpt.26.ident=FES
// Wpt.26.type=4
// Wpt.26.latitude=33.927417
// Wpt.26.longitude=-5.013583
// Wpt.27.ident=BISMI
// Wpt.27.type=4
// Wpt.27.latitude=33.647208
// Wpt.27.longitude=-6.893511
// Wpt.28.ident=GMMN
// Wpt.28.type=1
// Wpt.28.latitude=33.364167
// Wpt.28.longitude=-7.581667
void FlightplanIO::saveFeelthereFpl(const atools::fs::pln::Flightplan& plan, const QString& filename, int groundSpeed)
{
  QFile flpFile(filename);

  if(flpFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QTextStream stream(&flpFile);
    stream.setCodec("UTF-8");
    stream.setRealNumberPrecision(8);

    stream << "; " << programFileInfo() << endl;
    stream << "[Origin]" << endl;
    stream << "ident=" << plan.departureIdent << endl;
    stream << "type=1" << endl;
    stream << "latitude=" << plan.constFirst().getPosition().getLatY() << endl;
    stream << "longitude=" << plan.constFirst().getPosition().getLonX() << endl;

    stream << "[Destination]" << endl;
    stream << "ident=" << plan.destinationIdent << endl;
    stream << "type=1" << endl;
    stream << "latitude=" << plan.constLast().getPosition().getLatY() << endl;
    stream << "longitude=" << plan.constLast().getPosition().getLonX() << endl;

    stream << "[Route]" << endl;
    stream << "gspd=" << groundSpeed << endl;
    stream << "countOfPoints=" << (numEntriesSave(plan) - 1) << endl;

    int index = 0;
    for(int i = 1; i < plan.size(); i++)
    {
      const FlightplanEntry& entry = plan.at(i);
      if(entry.isNoSave())
        // Do not save procedure points
        continue;

      QString prefix = QString("Wpt.%1.").arg(index, 2, 10, QChar('0'));
      stream << prefix << "ident=" << identOrDegMinFormat(entry) << endl;
      stream << prefix << "type=" << (i == plan.size() - 1 ? "1" : "4") << endl;
      stream << prefix << "latitude=" << entry.getPosition().getLatY() << endl;
      stream << prefix << "longitude=" << entry.getPosition().getLonX() << endl;

      index++;
    }

    flpFile.close();
  }
  else
    throw Exception(errorMsg.arg(filename).arg(flpFile.errorString()));
}

// LeveL-D PLN=EDDMCYVR01.rte
// H,SBEG,SBBE, ,
// W,MALPU,,30,-2.946833,-59.322333,0.000000,0.000000,0,0,0,0
// W,KULAB,UZ81,30,-2.854667,-58.497500,0.000000,0.000000,0,0,0,0
// W,DOLTI,UZ81,30,-2.783167,-57.872167,0.000000,0.000000,0,0,0,0
// W,PRI,UZ81,30,-2.667281,-56.773111,0.000000,0.000000,0,0,0,0
// W,XOLOK,UZ81,30,-2.617167,-56.426167,0.000000,0.000000,0,0,0,0
// W,OPLIP,UZ81,30,-2.598833,-56.266667,0.000000,0.000000,0,0,0,0
// W,TOBUX,UZ81,30,-2.508167,-55.478667,0.000000,0.000000,0,0,0,0
// W,STM,UZ81,30,-2.426419,-54.817564,0.000000,0.000000,0,0,0,0
// W,AMVER,UZ81,30,-2.295667,-54.013500,0.000000,0.000000,0,0,0,0
// W,RESIN,UZ81,30,-2.229833,-53.609333,0.000000,0.000000,0,0,0,0
// W,OPSUT,UZ81,30,-2.088667,-52.743000,0.000000,0.000000,0,0,0,0
// W,MASVU,UZ81,30,-2.044167,-52.468333,0.000000,0.000000,0,0,0,0
// W,ILMAN,UZ81,30,-1.493500,-49.135333,0.000000,0.000000,0,0,0,0
//
//
// End of FlightPlan
void FlightplanIO::saveLeveldRte(const atools::fs::pln::Flightplan& plan, const QString& filename)
{
  QFile rteFile(filename);

  if(rteFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QTextStream stream(&rteFile);
    stream.setCodec("UTF-8");
    stream.setRealNumberPrecision(8);

    stream << "H," << plan.departureIdent << "," << plan.destinationIdent << ", ," << endl;

    for(int i = 0; i < plan.size(); i++)
    {
      const FlightplanEntry& entry = plan.at(i);
      if(entry.isNoSave() || i == 0 || i == plan.size() - 1)
        // Do not save procedure points, neither start nor destination
        continue;

      stream << "W," << identOrDegMinFormat(entry) << "," << entry.getAirway() << ",30,"
             << QString("%1").arg(entry.getPosition().getLatY(), 0, 'f', 6, QChar('0')) << ","
             << QString("%1").arg(entry.getPosition().getLonX(), 0, 'f', 6, QChar('0'))
             << ",0.000000,0.000000,0,0,0,0" << endl;
    }

    stream << endl << endl << "End of FlightPlan";

    rteFile.close();
  }
  else
    throw Exception(errorMsg.arg(filename).arg(rteFile.errorString()));
}

// [AivlaSoft EFB Route - www.aivlasoft.com]
////Saved 2019-01-16 16:34:45z
////AIRAC cycle 1703
// Format=1
// ATS=IDEKO ABMAL Z990 HLZ M852 POVEL Z94 GALMA M736 LIZUM N503 VIC L12 LUMAV M726 GAVRA Z806 GIKIN L865 TAQ
// Generator=SimBrief
// Origin=EDDH
// Destination=LIRF
// CruiseAltitude=29000
// DepartureProcedureInfo=23|IDEK5B|IDEKO|
// ArrivalProcedureInfo=16R|TAQ1C|TAQ
// ApproachProcedureInfo=||
// Wpt=Enroute|0|IDEKO||Fix|0|ED|52.993947|9.450494|0|DCT
// Wpt=Enroute|1|ABMAL||Fix|0|ED|53.442069|10.911981|0|DCT
// Wpt=Enroute|2|IRKIS||Fix|0|ED|53.248889|10.888611|0|Z990
// Wpt=Enroute|3|DIRBO||Fix|0|ED|52.832119|10.844158|0|Z990
// Wpt=Enroute|4|HLZ|HEHLINGEN|VORDME|117.30|ED|52.363394|10.795219|0|Z990
// Wpt=Enroute|5|POVEL||Fix|0|ED|52.128378|10.827778|0|M852
// Wpt=Enroute|6|ABGUS||Fix|0|ED|51.867058|11.053431|0|Z94
// Wpt=Enroute|7|KENIG||Fix|0|ED|51.619572|11.264394|0|Z94
// Wpt=Enroute|8|GALMA||Fix|0|ED|51.398056|11.451111|0|Z94
// ...
// Wpt=Enroute|35|VERUN||Fix|0|LI|43.281389|11.921389|0|Z806
// Wpt=Enroute|36|OKBIS||Fix|0|LI|43.002222|11.975556|0|Z806
// Wpt=Enroute|37|UPONO||Fix|0|LI|42.849167|12.004722|0|Z806
// Wpt=Enroute|38|GIKIN||Fix|0|LI|42.618333|12.048611|0|Z806
// Wpt=Enroute|39|TAQ|TARQUINIA|VORDME|111.80|LI|42.215056|11.732611|0|L865
// [END]
// DepartureProcedureInfo=23|||
// ArrivalProcedureInfo=26R||
// ApproachProcedureInfo=||
void FlightplanIO::saveEfbr(const Flightplan& plan, const QString& filename, const QString& route, const QString& cycle,
                            const QString& departureRw, const QString& destinationRw)
{
  QFile efbFile(filename);

  if(efbFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QTextStream stream(&efbFile);
    stream.setCodec("UTF-8");
    stream.setRealNumberPrecision(8);

    stream << "[AivlaSoft EFB Route - www.aivlasoft.com]" << endl;
    stream << "//Saved " << QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd HH:mm:ss'z'") << endl;
    stream << "//AIRAC cycle " << cycle << endl;
    stream << "Format=1" << endl;
    stream << "ATS=" << route << endl;
    stream << "Generator=" << QCoreApplication::applicationName() << endl;
    stream << "Origin=" << plan.departureIdent << endl;
    stream << "Destination=" << plan.destinationIdent << endl;
    stream << "CruiseAltitude=" << atools::roundToInt(plan.cruiseAltitudeFt) << endl;
    stream << "DepartureProcedureInfo=" << departureRw << "|||" << endl;
    stream << "ArrivalProcedureInfo=" << destinationRw << "||" << endl;
    stream << "ApproachProcedureInfo=||" << endl;

    int num = 0;
    for(int i = 0; i < plan.size(); i++)
    {
      const FlightplanEntry& entry = plan.at(i);
      if(entry.isNoSave() || i == 0 || i == plan.size() - 1)
        // Do not save procedure points, neither start nor destination
        continue;

      // Wpt=Enroute|38|GIKIN||Fix|0|LI|42.618333|12.048611|0|Z806
      // Wpt=Enroute|39|TAQ|TARQUINIA|VORDME|111.80|LI|42.215056|11.732611|0|L865
      stream << "Wpt=Enroute|" << num << "|" << identOrDegMinFormat(entry) << "|" << entry.getName().toUpper() <<
        "|";
      entry::WaypointType waypointType = entry.getWaypointType();
      QString frequency("0");

      switch(waypointType)
      {
        case atools::fs::pln::entry::UNKNOWN:
        case atools::fs::pln::entry::AIRPORT:
        case atools::fs::pln::entry::WAYPOINT:
        case atools::fs::pln::entry::USER:
          stream << "Fix";
          break;
        case atools::fs::pln::entry::VOR:
          stream << "VORDME";
          if(entry.getFrequency() > 0)
            frequency = QString("%1").arg(entry.getFrequency() / 1000., 0, 'f', 2, QChar('0'));
          break;
        case atools::fs::pln::entry::NDB:
          stream << "NDB";
          if(entry.getFrequency() > 0)
            frequency = QString("%1").arg(entry.getFrequency() / 100., 0, 'f', 1, QChar('0'));
          break;
      }

      stream << "|" << frequency << "|" << entry.getRegion() << "|"
             << QString("%1").arg(entry.getPosition().getLatY(), 0, 'f', 6, QChar('0')) << "|"
             << QString("%1").arg(entry.getPosition().getLonX(), 0, 'f', 6,
                           QChar('0')) << "|0|" << (entry.getAirway().isEmpty() ? "DCT" : entry.getAirway()) << endl;

      num++;
    }

    stream << "[END]";

    efbFile.close();
  }
  else
    throw Exception(errorMsg.arg(filename).arg(efbFile.errorString()));
}

// [FlightPlan]
// EDDH             53.630389  9.988228  APT ---
// IDEKO            52.993947  9.450494  WPT ---
// TIMEN            52.632222  9.358889  WPT ---    Y900
// ROBEG            52.233617  9.269619  WPT ---    UL126
// PIROT            52.053431  9.236903  WPT ---    UL126
// LARBU            51.885556  9.193056  WPT ---    UL126
// WRB              51.505697  9.110914  WPT 113.70    UL126
// RANAX            51.207222  9.046111  WPT ---    UN850
// EDEGA            51.034722  9.007778  WPT ---    UN850
// AMETU            50.833056  8.964444  WPT ---    UN850
// SOGMI            50.527500  8.899167  WPT ---    UN850
// BOMBI            50.056667  8.800278  WPT ---    UN850
// OMOGI            49.655833  8.989167  WPT ---    T721
// SUNEG            49.198611  8.864167  WPT ---    T721
// TEDGO            48.618408  9.259208  WPT ---    UL607
// UTABA            48.255000  9.461667  WPT ---    UL607
// BATUB            48.019167  9.563889  WPT ---    UM738
// OSDOV            47.440000  10.183333  WPT ---    UM738
// MADEB            47.324375  10.288886  WPT ---    UM738
// TIRUL            47.057175  10.528708  WPT ---    UM738
// NATAG            46.857992  10.618750  WPT ---    UM738
// LORLO            46.724167  10.678889  WPT ---
// ADOSA            45.644444  11.026389  WPT ---
// GAVRA            43.776111  11.824722  WPT ---
// RITEB            42.698611  12.163611  WPT ---
// LIRF             41.800278  12.238889  APT ---
void FlightplanIO::saveQwRte(const Flightplan& plan, const QString& filename)
{
  QFile rteFile(filename);

  if(rteFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QTextStream stream(&rteFile);
    stream.setCodec("UTF-8");
    stream.setRealNumberPrecision(8);

    stream << "[FlightPlan]" << endl;

    for(int i = 0; i < plan.size(); i++)
    {
      const FlightplanEntry& entry = plan.at(i);
      if(entry.isNoSave())
        // Do not save procedure points
        continue;

      QString frequency("---"), type;
      entry::WaypointType waypointType = entry.getWaypointType();
      switch(waypointType)
      {
        case atools::fs::pln::entry::WAYPOINT:
        case atools::fs::pln::entry::UNKNOWN:
        case atools::fs::pln::entry::USER:
          type = "WPT";
          break;

        case atools::fs::pln::entry::AIRPORT:
          type = "APT";
          break;

        case atools::fs::pln::entry::VOR:
          type = "WPT";
          if(entry.getFrequency() > 0)
            frequency = QString("%1").arg(entry.getFrequency() / 1000., 0, 'f', 2, QChar('0'));
          break;

        case atools::fs::pln::entry::NDB:
          type = "WPT";
          if(entry.getFrequency() > 0)
            frequency = QString("%1").arg(entry.getFrequency() / 100., 0, 'f', 1, QChar('0'));
          break;
      }

      // MADEB            47.324375  10.288886  WPT ---    UM738
      stream << QString("%1").arg(identOrDegMinFormat(entry), -17)
             << QString("%1").arg(entry.getPosition().getLatY(), 0, 'f', 6, QChar('0')) << "  "
             << QString("%1").arg(entry.getPosition().getLonX(), 0, 'f', 6, QChar('0')) << "  "
             << type << " " << frequency << "    " << entry.getAirway() << endl;
    }

    rteFile.close();
  }
  else
    throw Exception(errorMsg.arg(filename).arg(rteFile.errorString()));
}

// LSZH
// EIDW
// DIRECT DITON 47.302222 8.333333
// UL613 DIDOR 49.313611 3.281944
// UT10 ALESO 50.575556 1.225556
// UT420 BIG 51.330875 0.034811
// UL9 KENET 51.520556 -1.455000
// UN14 MEDOG 51.950278 -3.549444
// UL18 LIPGO 53.063917 -5.500000
void FlightplanIO::saveMdr(const Flightplan& plan, const QString& filename)
{
  QFile mdrFile(filename);

  if(mdrFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QTextStream stream(&mdrFile);
    stream.setCodec("UTF-8");

    stream << plan.departureIdent << endl;
    stream << plan.destinationIdent << endl;

    QString lastAirway;
    for(int i = 1; i < plan.size(); i++)
    {
      const FlightplanEntry& entry = plan.at(i);

      if(entry.isNoSave())
        // Do not save procedure points
        continue;

      if(!lastAirway.isEmpty() && lastAirway == entry.getAirway())
        // Repeating the same airway - skip waypoint
        continue;

      const FlightplanEntry& prev = plan.at(i - 1);
      if(!lastAirway.isEmpty())
        // Airway has changed - print the last waypoint
        stream << lastAirway << " " << prev.getIdent() << " " << QString("%1 %2").
          arg(prev.getPosition().getLatY(), 0, 'f', 6).
          arg(prev.getPosition().getLonX(), 0, 'f', 6) << endl;

      if(entry.getAirway().isEmpty() && i != plan.size() - 1)
      {
        // Not an airway - print as is
        QString coords = QString("%1 %2").
                         arg(entry.getPosition().getLatY(), 0, 'f', 6).
                         arg(entry.getPosition().getLonX(), 0, 'f', 6);

        stream << "DIRECT" << " " << identOrDegMinFormat(entry) << " " << coords << endl;
      }
      lastAirway = entry.getAirway();
    }

    mdrFile.close();
  }
  else
    throw Exception(errorMsg.arg(filename).arg(mdrFile.errorString()));
}

/*
 *  <?xml version="1.0" encoding="UTF-8"?>
 *  <Version1>
 *  <Departure>KALB</Departure>
 *  <Arrival>KBWI</Arrival>
 *  <CostIndex>24</CostIndex>
 *  <CruiseLevel>28000</CruiseLevel>
 *  <Legs>
 *   <Leg Type="Waypoint" Ident="ATHOS" Latitude="42.2471" Longitude="- 73.8121"/>
 *   <Leg Type="Navaid" Ident="CMK" Latitude="41.2801" Longitude="-73.5813"/>
 *   <Leg Type="AirwayLeg" Level="H" Wpt2Ident="CAE" Wpt2Latitude="33.8573" Wpt2Longitude="-81.0539" Airway="J75" />
 *   <Leg Type="Airport" ICAO="KIAD" Latitude="38.9474" Longitude="-77.4599"/>
 *   <Leg Type="PilotDefinedWaypoint" Ident="PBD01" Elevation="0" Latitude="33.7197" Longitude="-84.4191" PBDPlace="KATL" PBDDist="5" PBDBearing="5" />
 *   <Leg Type="Discontinuity" />
 *  </Legs>
 *  </Version1>
 */
void FlightplanIO::saveTfdi(const Flightplan& plan, const QString& filename, const QBitArray& jetAirways)
{
  QFile xmlFile(filename);
  if(xmlFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    // Write XML to string first ===================
    QXmlStreamWriter writer(&xmlFile);
    writer.setCodec("UTF-8");
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(4);

    writer.writeStartDocument("1.0");
    writer.writeStartElement("Version1");
    writer.writeTextElement("Departure", plan.departureIdent);
    writer.writeTextElement("Arrival", plan.destinationIdent);
    writer.writeTextElement("CruiseLevel", QString::number(atools::roundToInt(plan.cruiseAltitudeFt)));
    writer.writeStartElement("Legs");

    int wpIdent = 0;
    for(int i = 1; i < plan.size() - 1; i++)
    {
      const FlightplanEntry& entry = plan.at(i);
      if(entry.isNoSave())
        // Do not save entries like procedure points
        continue;

      writer.writeStartElement("Leg");

      if(entry.getAirway().isEmpty())
      {
        switch(entry.getWaypointType())
        {
          case atools::fs::pln::entry::AIRPORT:
            writer.writeAttribute("Type", "Airport");
            writer.writeAttribute("ICAO", entry.getIdent());
            break;
          case atools::fs::pln::entry::WAYPOINT:
            writer.writeAttribute("Type", "Waypoint");
            writer.writeAttribute("Ident", entry.getIdent());
            break;
          case atools::fs::pln::entry::VOR:
          case atools::fs::pln::entry::NDB:
            writer.writeAttribute("Type", "Navaid");
            writer.writeAttribute("Ident", entry.getIdent());
            break;
          case atools::fs::pln::entry::UNKNOWN:
          case atools::fs::pln::entry::USER:
            writer.writeAttribute("Type", "PilotDefinedWaypoint");
            writer.writeAttribute("Ident", QString("WP%1").arg(wpIdent++));
            break;
        }
        writer.writeAttribute("Latitude", QString::number(entry.getPosition().getLatY(), 'f', 4));
        writer.writeAttribute("Longitude", QString::number(entry.getPosition().getLonX(), 'f', 4));
      }
      else
      {
        writer.writeAttribute("Type", "AirwayLeg");
        writer.writeAttribute("Level", jetAirways.at(i) ? "H" : "L");
        writer.writeAttribute("Wpt2Ident", entry.getIdent());
        writer.writeAttribute("Wpt2Latitude", QString::number(entry.getPosition().getLatY(), 'f', 4));
        writer.writeAttribute("Wpt2Longitude", QString::number(entry.getPosition().getLonX(), 'f', 4));
        writer.writeAttribute("Airway", entry.getAirway());
      }
      writer.writeEndElement(); // Leg
    }

    writer.writeEndElement(); // Legs
    writer.writeEndElement(); // Version1
    writer.writeEndDocument();

    xmlFile.close();
  }
  else
    throw Exception(errorMsg.arg(filename).arg(xmlFile.errorString()));
}

// [RTE]
// ORIGIN_AIRPORT=KMFR
// DEST_AIRPORT=KORD

// [RTE.0]
// RouteName=
// Name=BRUTE
// Latitude=42.407528
// Longitude=-122.722533
// CrossThisPoint=0
// Heading=0
// Speed=0
// Altitude=0
// Frequency=
// FrequencyID=

// [RTE.1]
// RouteName=V122
// Name=LANKS
// Latitude=42.365658
// Longitude=-122.612475
// CrossThisPoint=0
// Heading=0
// Speed=0
// Altitude=0
// Frequency=
// FrequencyID=
void FlightplanIO::saveIfly(const Flightplan& plan, const QString& filename)
{
  QFile routeFile(filename);
  if(routeFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QTextStream stream(&routeFile);
    stream.setCodec("UTF-8");
    stream.setRealNumberPrecision(8);

    stream << "[RTE]" << endl;
    stream << "ORIGIN_AIRPORT=" << plan.getDepartureIdent() << endl;
    stream << "DEST_AIRPORT=" << plan.getDestinationIdent() << endl;
    stream << endl;

    int index = 0;
    for(const FlightplanEntry& entry : plan)
    {
      if(entry.isNoSave())
        // Do not save procedure points
        continue;

      stream << "[RTE." << index << "]" << endl;
      stream << "RouteName=" << entry.getAirway() << endl;
      stream << "Name=" << entry.getIdent() << endl;
      stream << "Latitude=" << entry.getPosition().getLatY() << endl;
      stream << "Longitude=" << entry.getPosition().getLonX() << endl;
      stream << "CrossThisPoint=0" << endl;
      stream << "Heading=0" << endl;
      stream << "SpeedConstraint=0" << endl;
      stream << "Speed=0" << endl;
      stream << "AltitudeConstraint=0" << endl;
      stream << "Altitude=0" << endl;

      QString freqStr;
      if(entry.getWaypointType() == atools::fs::pln::entry::VOR)
        freqStr = QString::number(entry.getFrequency() / 1000.f);
      else if(entry.getWaypointType() == atools::fs::pln::entry::NDB)
        freqStr = QString::number(entry.getFrequency() / 100.f);
      stream << "Frequency=" << freqStr << endl;

      stream << "FrequencyID=" << endl;
      stream << endl;
      index++;
    }

    stream << "[CDU]" << endl;
    stream << "CRZ_ALT=" << endl;
    stream << "COST_INDEX=" << endl;
    stream << endl;

    routeFile.close();
  }
  else
    throw Exception(errorMsg.arg(filename).arg(routeFile.errorString()));
}

QString FlightplanIO::saveGpxStr(const Flightplan& plan, const QVector<geo::LineString>& tracks,
                                 const QVector<QVector<qint64> >& timestampsMs, int cruiseAltFt)
{
  QString gpxString;
  QXmlStreamWriter writer(&gpxString);
  saveGpxInternal(plan, writer, tracks, timestampsMs, cruiseAltFt);
  return gpxString;
}

QByteArray FlightplanIO::saveGpxGz(const Flightplan& plan, const QVector<geo::LineString>& tracks,
                                   const QVector<QVector<qint64> >& timestampsMs, int cruiseAltFt)
{
  QByteArray retval;
  atools::zip::gzipCompress(saveGpxStr(plan, tracks, timestampsMs, cruiseAltFt).toUtf8(), retval);
  return retval;
}

void FlightplanIO::saveGpx(const atools::fs::pln::Flightplan& plan, const QString& filename,
                           const QVector<geo::LineString>& tracks, const QVector<QVector<qint64> >& timestampsMs, int cruiseAltFt)
{
  QFile gpxFile(filename);
  if(gpxFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QXmlStreamWriter writer(&gpxFile);
    saveGpxInternal(plan, writer, tracks, timestampsMs, cruiseAltFt);
    gpxFile.close();
  }
  else
    throw Exception(errorMsg.arg(filename).arg(gpxFile.errorString()));
}

void FlightplanIO::saveGpxInternal(const atools::fs::pln::Flightplan& plan, QXmlStreamWriter& writer,
                                   const QVector<geo::LineString>& tracks, const QVector<QVector<qint64> >& timestampsMs, int cruiseAltFt)
{
  writer.setCodec("UTF-8");
  writer.setAutoFormatting(true);
  writer.setAutoFormattingIndent(2);

  // <?xml version="1.0" encoding="UTF-8"?>
  writer.writeStartDocument("1.0");

  // <gpx
  // xmlns="http://www.topografix.com/GPX/1/1"
  // version="1.1"
  // creator="Program"
  // xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  // xsi:schemaLocation="http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd">

  writer.writeStartElement("gpx");

  writer.writeDefaultNamespace("http://www.topografix.com/GPX/1/1");
  writer.writeAttribute("version", "1.1");
  writer.writeAttribute("creator", "Little Navmap");
  writer.writeNamespace("http://www.w3.org/2001/XMLSchema-instance", "xsi");
  writer.writeAttribute("xsi:schemaLocation", "http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd");

  // writer.writeComment(programFileInfo());

  // <metadata>
  // <link href="http://www.garmin.com">
  // <text>Garmin International</text>
  // </link>
  // <time>2009-10-17T22:58:43Z</time>
  // </metadata>
  writer.writeStartElement("metadata");
  writer.writeStartElement("link");
  writer.writeAttribute("href", "https://www.littlenavmap.org");
  writer.writeTextElement("text", atools::programFileInfo());
  writer.writeEndElement(); // link
  writer.writeEndElement(); // metadata

  QString descr;

  if(!plan.isEmpty())
  {
    descr = QString("%1 (%2) to %3 (%4) at %5 ft, %6").
            arg(plan.departNameOrIdent()).arg(plan.departureIdent).
            arg(plan.destNameOrIdent()).arg(plan.destinationIdent).
            arg(atools::roundToInt(plan.getCruiseAltitudeFt())).
            arg(flightplanTypeToString(plan.flightplanType));

    writer.writeStartElement("rte");
    writer.writeTextElement("name", plan.getTitle() + tr(" Flight Plan"));
    writer.writeTextElement("desc", descr);

    // Write route ========================================================
    for(int i = 0; i < plan.size(); i++)
    {
      const FlightplanEntry& entry = plan.at(i);
      // <rtept lat="52.0" lon="13.5">
      // <ele>33.0</ele>
      // <time>2011-12-13T23:59:59Z</time>
      // <name>rtept 1</name>
      // </rtept>

      if(!entry.getPosition().isValidRange())
      {
        qWarning() << Q_FUNC_INFO << "Invalid position" << entry.getPosition();
        continue;
      }

      if(i > 0)
      {
        // Remove duplicates with same name and almost same position
        const FlightplanEntry& prev = plan.at(i - 1);
        if(entry.getIdent() == prev.getIdent() &&
           entry.getRegion() == prev.getRegion() &&
           entry.getPosition().almostEqual(prev.getPosition(), Pos::POS_EPSILON_100M))
          continue;
      }

      writer.writeStartElement("rtept");
      writer.writeAttribute("lon", QString::number(entry.getPosition().getLonX(), 'f', 6));
      writer.writeAttribute("lat", QString::number(entry.getPosition().getLatY(), 'f', 6));

      if(i > 0 && i < plan.size() - 1)
        writer.writeTextElement("ele", QString::number(atools::geo::feetToMeter(cruiseAltFt)));
      else
        writer.writeTextElement("ele", QString::number(atools::geo::feetToMeter(entry.getPosition().getAltitude())));

      writer.writeTextElement("name", entry.getIdent());
      writer.writeTextElement("desc", entry.getWaypointTypeAsFsxString());

      writer.writeEndElement(); // rtept
    }

    writer.writeEndElement(); // rte
  }

  // Write track ========================================================
  if(!tracks.isEmpty())
  {
    writer.writeStartElement("trk");

    if(!plan.isEmpty())
      writer.writeTextElement("name", QCoreApplication::applicationName() + tr(" Track"));

    for(int segIdx = 0; segIdx < tracks.size(); ++segIdx)
    {
      const geo::LineString& track = tracks.at(segIdx);
      if(track.isEmpty())
        continue;

      writer.writeStartElement("trkseg");

      for(int ptIdx = 0; ptIdx < track.size(); ++ptIdx)
      {
        const Pos& pos = track.at(ptIdx);
        writer.writeStartElement("trkpt");

        writer.writeAttribute("lon", QString::number(pos.getLonX(), 'f', 6));
        writer.writeAttribute("lat", QString::number(pos.getLatY(), 'f', 6));
        writer.writeTextElement("ele", QString::number(atools::geo::feetToMeter(pos.getAltitude())));

        if(!timestampsMs.isEmpty())
        {
          // (UTC/Zulu) in ISO 8601 format: "yyyy-mm-ddThh:mm:ssZ" or "yyyy-MM-ddTHH:mm:ss.zzzZ"
          // <time>2011-01-16T23:59:01Z</time>
          // Changes time number to local if Qt::UTC is omitted
          qint64 timestampMs = timestampsMs.at(segIdx).at(ptIdx);
          writer.writeTextElement("time", QDateTime::fromMSecsSinceEpoch(timestampMs, Qt::UTC).toString(Qt::ISODateWithMs));
        }
        writer.writeEndElement(); // trkpt
      }
      writer.writeEndElement(); // trkseg
    }
    writer.writeEndElement(); // trk
  }

  writer.writeEndElement(); // gpx
  writer.writeEndDocument();
}

void FlightplanIO::loadGpxStr(atools::geo::LineString *route, QStringList *routenames,
                              QVector<atools::geo::LineString> *tracks, QVector<QVector<qint64> > *timestampsMs,
                              const QString& string)
{
  if(!string.isEmpty())
  {
    atools::util::XmlStream xmlStream(string);
    loadGpxInternal(route, routenames, tracks, timestampsMs, xmlStream);
  }
}

void FlightplanIO::loadGpxGz(atools::geo::LineString *route, QStringList *routenames,
                             QVector<atools::geo::LineString> *tracks, QVector<QVector<qint64> > *timestampsMs,
                             const QByteArray& bytes)
{
  if(!bytes.isEmpty())
    loadGpxStr(route, routenames, tracks, timestampsMs, atools::zip::gzipDecompress(bytes));
}

void FlightplanIO::loadGpx(atools::geo::LineString *route, QStringList *routenames,
                           QVector<atools::geo::LineString> *tracks, QVector<QVector<qint64> > *timestampsMs,
                           const QString& filename)
{
  QFile gpxFile(filename);
  if(gpxFile.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    atools::util::XmlStream xmlStream(&gpxFile);
    loadGpxInternal(route, routenames, tracks, timestampsMs, xmlStream);
    gpxFile.close();
  }
  else
    throw Exception(errorMsg.arg(filename).arg(gpxFile.errorString()));
}

void FlightplanIO::loadGpxInternal(atools::geo::LineString *route, QStringList *routenames,
                                   QVector<atools::geo::LineString> *tracks, QVector<QVector<qint64> > *timestampsMs,
                                   atools::util::XmlStream& xmlStream)
{
  QXmlStreamReader& reader = xmlStream.getReader();
  xmlStream.readUntilElement("gpx");
  Pos pos;
  QString name;
  QDateTime dummy;
  while(xmlStream.readNextStartElement())
  {
    // Read route elements if needed ======================================================
    if(reader.name() == "rte" && (route != nullptr || routenames != nullptr))
    {
      while(xmlStream.readNextStartElement())
      {
        if(reader.name() == "rtept")
        {
          readPosGpx(pos, name, dummy, xmlStream);
          if(pos.isValidRange())
          {
            if(route != nullptr)
              route->append(pos);
            if(routenames != nullptr)
              routenames->append(name);
          }
        }
        else
          xmlStream.skipCurrentElement(false /* warn */);
      }
    }
    // Read track elements if needed ======================================================
    else if(reader.name() == "trk" && (tracks != nullptr || timestampsMs != nullptr))
    {
      atools::geo::LineString line;
      QVector<qint64> times;
      while(xmlStream.readNextStartElement())
      {
        if(reader.name() == "trkseg")
        {
          line.clear();
          times.clear();
          while(xmlStream.readNextStartElement())
          {
            if(reader.name() == "trkpt")
            {
              QDateTime datetetime;
              readPosGpx(pos, name, datetetime, xmlStream);
              if(pos.isValidRange())
              {
                if(tracks != nullptr)
                  line.append(pos);

                if(timestampsMs != nullptr)
                  times.append(datetetime.toMSecsSinceEpoch());
              }
            }
            else
              xmlStream.skipCurrentElement(false /* warn */);
          }

          if(tracks != nullptr)
            tracks->append(line);

          if(timestampsMs != nullptr)
            timestampsMs->append(times);
        }
        else
          xmlStream.skipCurrentElement(false /* warn */);
      }
    }
    else
      xmlStream.skipCurrentElement(false /* warn */);
  }
}

void FlightplanIO::saveIniBuildsMsfs(const atools::fs::pln::Flightplan& plan, const QString& file)
{
  saveFmsInternal(plan, file, false /* version11Format */, true /* iniBuildsFormat */);
}

// I
// 3 version
// 1
// 8
// 28 WP1 0.000000 53.421391 -6.270000
// 28 WP2 0.000000 53.434834 -6.238197
// 28 WP3 0.000000 53.437164 -6.280617
// 28 WP4 0.000000 53.503860 -6.442389
// 28 WP5 0.000000 53.552917 -6.601222
// 28 WP6 0.000000 53.597584 -7.391278
// 28 WP7 0.000000 49.500000 -52.000000
// 28 WP8 0.000000 48.731529 -54.328449
// 28 WP9 0.000000 48.692711 -54.633060
void FlightplanIO::saveCivaFms(atools::fs::pln::Flightplan plan, const QString& file)
{
  // Create a copy and erase all consecutive waypoints having the same position
  plan.erase(std::unique(plan.begin(), plan.end(), [ = ](FlightplanEntry& entry1, FlightplanEntry& entry2) -> bool {
          return entry1.getPosition().almostEqual(entry2.getPosition(), atools::geo::Pos::POS_EPSILON_10M);
        }), plan.end());

  int fileNumber = 0;
  Flightplan splitPlan;
  for(int i = 0; i < plan.size(); i++)
  {
    // Prepare entry for copying
    FlightplanEntry entry = plan.at(i);
    entry.setIdent(QString("WP%1").arg(splitPlan.size() + 1)); // Change ident
    entry.setPosition(entry.getPosition().alt(0.f)); // Set altitude to zero
    entry.setWaypointType(atools::fs::pln::entry::USER); // Change to user type to export with prefix 28
    entry.setFlags(atools::fs::pln::entry::NONE); // No flags - all points exported - missed approach removed
    splitPlan.append(entry);

    // Check if current file is full or the last one was added
    if(splitPlan.size() == 9 || (i == plan.size() - 1 && !splitPlan.isEmpty()))
    {
      QFileInfo fi(file);
      QString filename;
      if(fileNumber == 0)
        // Omit number for first file
        filename = fi.path() + QDir::separator() + fi.baseName() + "." + fi.completeSuffix();
      else
        // Add number for all other files
        filename = fi.path() + QDir::separator() + fi.baseName() + QString::number(fileNumber) + "." + fi.completeSuffix();

      splitPlan.adjustDepartureAndDestination(true);
      saveFmsInternal(splitPlan, filename, false /* version11Format */, false /* iniBuildsFormat */);

      // Clear and add last one for overlap
      splitPlan.clearAll();
      entry.setIdent("WP1");
      splitPlan.append(entry);
      fileNumber++;
    }
  }
}

void FlightplanIO::saveFms3(const atools::fs::pln::Flightplan& plan, const QString& file)
{
  saveFmsInternal(plan, file, false /* version11Format */, false /* iniBuildsFormat */);
}

void FlightplanIO::saveFms11(const atools::fs::pln::Flightplan& plan, const QString& file)
{
  saveFmsInternal(plan, file, true /* version11Format */, false /* iniBuildsFormat */);
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
void FlightplanIO::saveFmsInternal(const atools::fs::pln::Flightplan& plan, const QString& filename, bool version11Format,
                                   bool iniBuildsFormat)
{
  QFile fmsFile(filename);

  if(fmsFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    int numEntries = numEntriesSave(plan);
    QTextStream stream(&fmsFile);
    stream.setCodec("UTF-8");

    if(!iniBuildsFormat)
      // OS
      stream << "I" << endl;

    // File version
    QString departureIdent = plan.getDepartureIdent().left(8);
    QString destinationIdent = plan.getDestinationIdent().left(8);
    QString cycle = plan.properties.value(NAVDATACYCLE);
    if(cycle.isEmpty())
      cycle = plan.properties.value(SIMDATACYCLE);
    if(cycle.isEmpty())
      // Fake a cycle by using current year and month
      cycle = QLocale(QLocale::C).toString(QDateTime::currentDateTime(), "yyMM");

    if(version11Format)
    {
      // FMS 11 ======================================
      stream << "1100 Version" << endl;

      stream << "CYCLE " << cycle << endl;

      // Departure ==============================
      if(plan.constFirst().getWaypointType() == entry::AIRPORT && !plan.properties.contains(AIRPORT_DEPARTURE_NO_AIRPORT))
        // Departure is normal airport id or there is a SID
        stream << "ADEP " << departureIdent << endl;
      else
        // Use any point
        stream << "DEP " << departureIdent << endl;

      // Departure - SID
      if(!plan.properties.value(SIDRW).isEmpty())
        stream << "DEPRWY RW" << plan.properties.value(SIDRW) << endl;

      if(!plan.properties.value(SID).isEmpty())
        stream << "SID " << plan.properties.value(SID) << endl;

      if(!plan.properties.value(SIDTRANS).isEmpty())
        stream << "SIDTRANS " << plan.properties.value(SIDTRANS) << endl;

      // Destination =============================
      if(plan.constLast().getWaypointType() == entry::AIRPORT && !plan.properties.contains(AIRPORT_DESTINATION_NO_AIRPORT))
        // Destination is normal airport id or there is a STAR or an approach
        stream << "ADES " << destinationIdent << endl;
      else
        // Use any point
        stream << "DES " << destinationIdent << endl;

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
    else if(iniBuildsFormat)
    {
      // MSFS Inibuilds ======================================
      // AIRLINE
      // FLTID N314SB
      // CYCLE 2211
      // ADEP KDEN
      // ADES KDFW
      // ALTN KIAH
      // CRUISE 37000
      // TRIP 6024
      // CONT 1061
      // ALTNFUEL 2821
      // FINRES 1845
      // MINTO 11752
      // EXTRA 0
      // TAXI 399
      // BLOCK 12151
      // ZFW 110280
      // TOW 122032
      // LAW 116008
      // CRUISEWIND 239/082
      // PAXNBR 221
      // CARGO 5513
      // PAYLOAD 23056
      // 1 KDEN ADEP 5434.000000 39.861667 -104.673167
      // 3 HGO DRCT 35900.000000 38.817500 -103.621389
      // 3 LAA DRCT 37000.000000 38.197092 -102.687533
      // 3 SPS DRCT 37000.000000 33.987286 -98.593472
      // 3 FUZ DRCT 3100.000000 32.889450 -97.179425
      // 1 KDFW ADES 3100.000000 32.897233 -97.037694
      stream << "ADEP " << departureIdent << endl;
      stream << "ADES " << destinationIdent << endl;
      stream << "CYCLE " << cycle << endl;
      stream << "CRUISE " << atools::roundToInt(plan.getCruiseAltitudeFt()) << endl;
    }
    else
    {
      // FMS 3 ======================================
      stream << "3 version" << endl;
      stream << "1" << endl;
      stream << (numEntries - 1) << endl; // Number of waypoints minus one for FMS 3
    }

    // Waypoints ======================================
    int index = 0;
    for(int i = 0; i < plan.size(); i++)
    {
      const FlightplanEntry& entry = plan.at(i);

      if(entry.isNoSave())
        continue;

      // 1 - Airport ICAO
      // 2 - NDB
      // 3 - VOR
      // 11 - Fix
      // 28 - Lat/Lon Position
      QString ident = entry.getIdent();
      if(entry.getWaypointType() == atools::fs::pln::entry::USER || entry.getWaypointType() == atools::fs::pln::entry::UNKNOWN)
      {
        // Type column user ================
        stream << "28 ";

        if(ident.isEmpty())
          // No ident - can appear if using export options
          stream << atools::fs::util::toDegMinFormat(entry.getPosition()) << " ";
        else
          // Replace spaces
          stream << ident.replace(QRegularExpression("[\\s]"), "_") << " ";

        // Disabled user waypoints as coordinates
        // +12.345_+009.459 Correct for a waypoint at 12.345°/0.459°.
        // -28.478_-056.370 Correct for a waypoint at -28.478°/-56.370°.
      }
      else
      {
        // Type column ================
        if(entry.getWaypointType() == atools::fs::pln::entry::AIRPORT)
          stream << "1 " << ident.left(8) << " ";
        else if(entry.getWaypointType() == atools::fs::pln::entry::WAYPOINT)
          stream << "11 " << ident << " ";
        else if(entry.getWaypointType() == atools::fs::pln::entry::VOR)
          stream << "3 " << ident << " ";
        else if(entry.getWaypointType() == atools::fs::pln::entry::NDB)
          stream << "2 " << ident << " ";
      }

      // Keywords column ================
      if(version11Format || iniBuildsFormat)
      {
        if(index == 0)
          stream << (entry.getWaypointType() == entry::AIRPORT ? "ADEP " : "DEP ");
        else if(index == numEntries - 1)
          stream << (entry.getWaypointType() == entry::AIRPORT ? "ADES " : "DES ");
        else
          stream << (entry.getAirway().isEmpty() ? "DRCT " : entry.getAirway() + " ");
      }

      // Coordinates for all formats ================
      stream << QString::number(entry.getPosition().getAltitude(), 'f', 6)
             << " "
             << QString::number(entry.getPosition().getLatY(), 'f', 6)
             << " "
             << QString::number(entry.getPosition().getLonX(), 'f', 6)
             << endl;

      index++;
    }

    fmsFile.close();
  }
  else
    throw Exception(errorMsg.arg(filename).arg(fmsFile.errorString()));
}

void FlightplanIO::saveRte(const atools::fs::pln::Flightplan& plan, const QString& filename)
{
  namespace ple = atools::fs::pln::entry;
  int userWaypointNum = 1;

  const int NO_DATA_NUM = -1000000;
  const QString NO_DATA_STR("-");
  enum
  {
    RTE_AIRPORT = 1, RTE_OTHER = 2, RTE_WAYPOINT = 5
  };

  enum
  {
    RTE_NO_PHASE = 0, RTE_CLIMB = 1, RTE_CRUISE = 2, RTE_DESCENT = 3
  };

  QFile rteFile(filename);

  if(rteFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QString rteString;
    QTextStream stream(&rteString);
    stream.setCodec("UTF-8");

    stream << tr("PMDG RTE Created by %1 Version %2 (revision %3) on %4 ").
      arg(QCoreApplication::applicationName()).
      arg(QCoreApplication::applicationVersion()).
      arg(atools::gitRevision()).
      arg(QDateTime::currentDateTime().toString(Qt::ISODate)).
      replace("-", " ") << endl << endl;

    stream << numEntriesSave(plan) << endl << endl;

    stream << plan.departureIdent << endl << RTE_AIRPORT << endl << "DIRECT" << endl;
    posToRte(stream, plan.constFirst().getPosition(), true);
    stream << endl << NO_DATA_STR << endl
           << 1 /* Departure*/ << endl << 0 /* Runway position */ << endl << endl;

    stream << RTE_CLIMB << endl; // Restriction phase climb
    stream << atools::roundToInt(plan.constFirst().getPosition().getAltitude()); // Restriction altitude, if restricted

    // Restriction type, altitude and speed
    stream << endl << NO_DATA_STR << endl << NO_DATA_NUM << endl << NO_DATA_NUM << endl << endl;

    for(int i = 1; i < plan.size() - 1; i++)
    {
      const FlightplanEntry& entry = plan.at(i);

      if(entry.isNoSave())
        // Do not save entries like procedure points
        continue;

      if(entry.getWaypointType() == ple::USER)
      {
        stream << "WPT" << userWaypointNum++ << endl;
        stream << RTE_OTHER << endl;
      }
      else
      {
        stream << (entry.getIdent().isEmpty() ? NO_DATA_STR : entry.getIdent()) << endl;
        stream << (entry.getWaypointType() == ple::AIRPORT ? RTE_AIRPORT : RTE_WAYPOINT) << endl;
      }

      QString nextAirway = plan.at(i + 1).getAirway();
      stream << (nextAirway.isEmpty() ? "DIRECT" : nextAirway) << endl;

      posToRte(stream, entry.getPosition(), false);
      stream << endl << 0 << endl << 0 << endl << 0 << endl << endl; // Restriction fields
    }

    stream << plan.destinationIdent << endl << RTE_AIRPORT << endl << NO_DATA_STR << endl;
    posToRte(stream, plan.destinationPos, true);
    stream << endl << NO_DATA_STR << endl
           << 0 /* no departure*/ << endl << 0 /* Runway position */ << endl << endl;

    stream << RTE_CLIMB << endl; // Restriction phase
    stream << atools::roundToInt(plan.destinationPos.getAltitude()) << endl; // Restriction altitude, if restricted
    // Restriction type, altitude and speed
    stream << NO_DATA_STR << endl << NO_DATA_NUM << endl << NO_DATA_NUM << endl;

#ifndef Q_OS_WIN32
    // Convert EOL always to Windows (0x0a -> 0x0d0a)
    rteString.replace("\n", "\r\n");
#endif

    QByteArray utf8 = rteString.toUtf8();
    rteFile.write(utf8.constData(), utf8.size());
    rteFile.close();
  }
  else
    throw Exception(errorMsg.arg(filename).arg(rteFile.errorString()));
}

void FlightplanIO::saveFpr(const atools::fs::pln::Flightplan& plan, const QString& filename)
{
  QFile fprFile(filename);

  // Create base hash from 0 to 32768 - use qt functions which also compile on mac
  int hashSeed = QRandomGenerator::global()->bounded(0, std::numeric_limits<qint16>::max());

  if(fprFile.open(QIODevice::WriteOnly))
  {
    QDataStream ds(&fprFile);

    fpr::FprPlan fprplan;
    memset(&fprplan, 0, sizeof(fprplan));

    int legIdx = 0;
    for(int i = 0; i < plan.size(); ++i)
    {
      const FlightplanEntry& e = plan.at(i);

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

        case atools::fs::pln::entry::WAYPOINT:
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
      writeBinaryString(leg->waypoint.designator, e.getIdent(), sizeof(leg->waypoint.designator));
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

// ERROR ===========================
// DIRECT,3,FJR,0,  43.578362 003.974722,0,0,091.00000,0,0,1,-1,0.000,0,-1000,-1000,-1,-1,-1,0,0,000.00000,0,0,,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1000,0,
// ,2,NIZ,0,  43.770638 007.254389,0,0,083.00000,0,0,1,-1,0.000,0,-1000,-1000,-1,-1,-1,0,0,000.00000,0,0,,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1000,0,
// ,2,AMGEL,0,  43.832054 007.374639,0,0,052.00000,0,0,1,-1,0.000,0,-1000,-1000,-1,-1,-1,0,0,000.00000,0,0,,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1000,0,

// OK ===========================
// DIRECT,3,MONTPELLIER MEDITERRANEE,FJR,+43.578360+003.974722,0,0,090.819150,0,0,-1,-1,0,-1,-1,-1,-1,0,0,0,0,0,0,0,-1000,-1,-1,17,0,-1000,-1,-1,17,0,-1000,-1,-1,17,0,-1000,-1,-1,17,0,-1000,-1000,0,
// DIRECT,3,NICE COTE DAZUR,NIZ,+43.770640+007.254389,0,0,084.234570,0,0,-1,-1,0,-1,-1,-1,-1,0,0,0,0,0,0,0,-1000,-1,-1,17,0,-1000,-1,-1,17,0,-1000,-1,-1,17,0,-1000,-1,-1,17,0,-1000,-1000,0,
// DIRECT,3,AMGEL,AMGEL,+43.832050+007.374639,0,0,054.674560,0,0,-1,-1,0,-1,-1,-1,-1,0,0,0,0,0,0,0,-1000,-1,-1,17,0,-1000,-1,-1,17,0,-1000,-1,-1,17,0,-1000,-1,-1,

void FlightplanIO::saveFltplan(const Flightplan& plan, const QString& filename)
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
    stream << atools::roundToInt(plan.getCruiseAltitudeFt()) << "," << endl;

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
    stream << "-1," << endl << "," << endl << "," << endl << "," << endl << "," << endl << "0," << endl;

    for(int i = 0; i < plan.size(); i++)
    {
      if(i == 0 || i == plan.size() - 1)
        // Start or destination
        continue;

      const FlightplanEntry& entry = plan.at(i);
      if(entry.isNoSave())
        continue;

      // DIRECT,3,WOL,0,-34.558056 150.791111,0,0,195.40055,0,0,1,321,0.000,0,18763,-1000,13468,457,-1,0,0,000.00000,0,0,,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1000,0,
      // H65,2,RAZZI,0,-35.054166 149.960277,0,0,220.43300,0,0,0,0,0.797,0,28908,-1000,12935,859,-1,0,0,000.00000,0,0,,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1,-1,-1000,0,-1000,-1000,0,
      if(entry.getAirway().isEmpty())
        stream << "DIRECT,3,";
      else
        stream << entry.getAirway() << ",2,";

      float heading = plan.at(i - 1).getPosition().angleDegTo(entry.getPosition());
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

      stream << identOrDegMinFormat(entry) << ",0,";

      stream << latY << lonX;
      stream << ",0,0," << QString("%1").arg(atools::roundToInt(heading), 3, 10, QChar('0')) << ".00000";

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
    fltplanFile.write(utf8.constData(), utf8.size());
    fltplanFile.close();
  }
  else
    throw Exception(errorMsg.arg(filename).arg(fltplanFile.errorString()));
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

void FlightplanIO::saveBbsPln(const Flightplan& plan, const QString& filename)
{
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
    stream << "routetype=3" << endl;

    // cruising_altitude=29000
    // departure_id=EDDH, N53* 37.82', E009* 59.29', +000053.00
    // destination_id=LIRF, N41* 48.02', E012* 14.33', +000014.00
    // departure_name=HAMBURG
    // destination_name=FIUMICINO
    stream << "cruising_altitude=" << atools::roundToInt(plan.getCruiseAltitudeFt()) << endl;
    stream << "departure_id=" << plan.getDepartureIdent() << ", " << coordStringFs9(plan.getDeparturePosition()) << endl;
    stream << "destination_id=" << plan.getDestinationIdent() << ", " << coordStringFs9(plan.getDestinationPosition()) << endl;
    stream << "departure_name=" << plan.departNameOrIdent().toUpper() << endl;
    stream << "destination_name=" << plan.destNameOrIdent().toUpper() << endl;

    // waypoint.0=EDDH, A, N53* 37.82', E009* 59.29', +000053.00,
    // waypoint.1=AMLUH, I, N53* 25.74', E010* 19.35', +000000.00,
    // ...
    // waypoint.19=RITEB, I, N42* 41.92', E012* 9.82', +000000.00, T369
    // waypoint.20=LIRF, A, N41* 48.02', E012* 14.33', +000014.00,
    int idx = 0;
    for(const FlightplanEntry& entry : plan)
    {
      if(entry.isNoSave())
        // Do not save procedure points
        continue;

      stream << "waypoint." << idx << "=" << identOrDegMinFormat(entry) << ", ";
      stream << entry.getWaypointTypeAsStringShort() << ", ";
      stream << coordStringFs9(entry.getPosition()) << ", ";
      stream << entry.getAirway();

      stream << endl;
      idx++;
    }

    fltplanFile.close();
  }
  else
    throw Exception(errorMsg.arg(filename).arg(fltplanFile.errorString()));
}

void FlightplanIO::saveGarminFpl(atools::fs::pln::Flightplan plan, const QString& filename, bool saveAsUserWaypoints)
{
  // Create a copy so we can easily change all waypoints to user defined is this is desired
  if(saveAsUserWaypoints)
  {
    // Convert all waypoints to user defined waypoints keeping the names
    int i = 0;
    for(FlightplanEntry& entry : plan)
    {
      if(entry.isNoSave())
        // Do not save procedure points
        continue;

      if(i > 0 && i < plan.size() - 1)
      {
        entry.setAirway(QString());
        entry.setRegion(QString());
        entry.setIdent(entry.getIdent());
        entry.setWaypointType(entry::USER);
      }

      i++;
    }
  }

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

    for(const FlightplanEntry& entry : plan)
    {
      if(entry.isNoSave())
        // Do not save procedure points
        continue;

      // Adjust name of user waypoints
      QString ident;
      if(entry.getWaypointType() == entry::USER || entry.getWaypointType() == entry::UNKNOWN)
      {
        ident = entry.getIdent();

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
        ident = entry.getIdent();

      QStringList wptDat({ident, gnsType(entry), entry.getRegion()});

      if(waypointList.contains(wptDat))
      {
        // Waypoint already in index
        const Pos pos = waypointList.value(wptDat);

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
    for(auto it = waypointList.constBegin(); it != waypointList.constEnd(); ++it)
    {
      QStringList key = it.key();
      writer.writeStartElement("waypoint");

      writer.writeTextElement("identifier", key.value(0).toUpper());
      writer.writeTextElement("type", key.value(1));

      QString region = key.value(2).toUpper();
      if(region.isEmpty() && key.value(1) != "USER WAYPOINT")
        qDebug() << Q_FUNC_INFO << "Region is empty for" << key << "in" << filename;

      writer.writeTextElement("country-code", region);

      const Pos pos = waypointList.value(key);
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
    for(const FlightplanEntry& entry : plan)
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
        writer.writeTextElement("waypoint-identifier", entry.getIdent().toUpper());
        writer.writeTextElement("waypoint-type", gnsType(entry));
      }

      writer.writeTextElement("waypoint-country-code", entry.getRegion().toUpper());

      writer.writeEndElement(); // route-point
      curIdx++;
    }

    writer.writeEndElement(); // route
    writer.writeEndElement(); // flight-plan
    writer.writeEndDocument();
    xmlFile.close();
  }
  else
    throw Exception(errorMsg.arg(filename).arg(xmlFile.errorString()));
}

void FlightplanIO::loadGarminFpl(atools::fs::pln::Flightplan& plan, const QString& filename)
{
  plan.clearAll();
  QFile xmlFile(filename);
  if(xmlFile.open(QIODevice::ReadOnly))
  {
    atools::util::XmlStream xmlStream(&xmlFile, filename);
    loadGarminFplInternal(plan, xmlStream);
    xmlFile.close();
  }
  else
    throw Exception(tr("Cannot open file \"%1\". Reason: %2").arg(filename).arg(xmlFile.errorString()));
}

// FPN/RI:F:BIKF:F:RIMUM:F:CELLO:F:6119N:F:BILTO:F:NETKI:F:AMDEP:F:UMLER:F:RIVAK:F:KORUL:F:MAVOS:F:LEAS
// FPN/RI:F:ENGM:F:N58208E006148:F:N57296E006526:F:N57000E007136:F:N49119E008518:F:N48371E009156:F:N48153E009277:F:EDDM
// FPN/RI:F:EDDF:F:WUR,N49431E009568:AA:EDDN:A:BISB1E(10O)
// FPN/RI:DA:KMEM:D:ELVIS3.WTWOO:AA:KLIT:AP:I04R.MOMTE
// FPN/RI:F:SPZO:F:ILMOX.V11.JUL.A304.PAZ.W11.VIR.A556.VAS.A311.COSTA:F:SGES
// FPN/RI:DA:KYKM:D:WENAS7.PERTT:R:09O:F:COBDI,N47072W120397:F:N47406W120509:F:ROZSE,N48134W121018:F:DIABO,N48500W120562.J503.FOLDY,N49031W120427:
// AA:CYLW:A:PIGLU4.YDC(16O):AP:I16-Z.HUMEK
void FlightplanIO::loadGarminGfp(atools::fs::pln::Flightplan& plan, const QString& filename)
{
  qDebug() << Q_FUNC_INFO << filename;

  QFile gfpFile(filename);

  if(gfpFile.open(QIODevice::ReadOnly))
  {
    QTextStream stream(&gfpFile);
    stream.setCodec("UTF-8");
    stream.setAutoDetectUnicode(true);

    // Read line and remove header
    QString line = stream.readLine();
    line.replace("FPN/RI:", "");

    // Loop through keys and values
    QString lastKey;
    QStringList valueList = line.split(':');
    for(int i = 0; i < valueList.size(); i++)
    {
      const QString& value = valueList.at(i);

      if(lastKey.isEmpty() && atools::contains(value, {"F" /* direct */,
                                                       "DA" /* departure airport */, "D" /* SID */, "R" /* runway */,
                                                       "AA" /* arrival airport */, "A" /* STAR */, "AP" /* Approach */}))
        // Known key in colons - remember and continue ============
        lastKey = value;
      else
      {
        // Value - decide by last key ====================
        if(lastKey == "F")
        {
          // Direct ==================================================
          if(value.contains('.'))
          {
            // Airways ...DIABO.J503.FOLDY:...
            QStringList waypointAirwayList = value.split('.');
            QString lastAirway;
            for(int j = 0; j < waypointAirwayList.size(); j++)
            {
              const QString& waypointAirway = waypointAirwayList.at(j);
              if((j % 2) == 0)
              {
                // Waypoint =================
                FlightplanEntry entry;
                entry.setIdent(waypointAirway.section(',', 0, 0));
                entry.setWaypointType(atools::fs::pln::entry::WAYPOINT);
                entry.setAirway(lastAirway);
                plan.append(entry);
                lastAirway.clear();
              }
              else
                // Airway to next waypoint in list ============
                lastAirway = waypointAirway;
            }
          }
          else
          {
            // Waypoint ==================================================
            FlightplanEntry entry;

            // ...COBDI,N47072W120397...
            entry.setIdent(value.section(',', 0, 0));

            if(i == 0 || i == valueList.size())
              entry.setWaypointType(atools::fs::pln::entry::AIRPORT);
            else
              entry.setWaypointType(atools::fs::pln::entry::WAYPOINT);
            plan.append(entry);
          }
        }
        else if(lastKey == "DA")
        {
          // Departure airport ...KYKM... ====================
          FlightplanEntry entry;
          entry.setIdent(value);
          entry.setWaypointType(atools::fs::pln::entry::AIRPORT);
          plan.append(entry);
        }
        else if(lastKey == "D")
        {
          // SID ...WENAS7.PERTT... ====================
          QString sid = value.section('(', 0, 0);
          insertPropertyIf(plan, SID, sid.section('.', 0, 0));
          insertPropertyIf(plan, SIDTRANS, sid.section('.', 1, 1));
        }
        else if(lastKey == "R")
        {
          // Runway ...09O... ====================
          QString rw = value;
          rw.remove('O');
          insertPropertyIf(plan, SIDRW, rw);
        }
        else if(lastKey == "AA")
        {
          // arrival airport ...CYLW... ====================
          FlightplanEntry entry;
          entry.setIdent(value);
          entry.setWaypointType(atools::fs::pln::entry::AIRPORT);
          plan.append(entry);
        }
        else if(lastKey == "A")
        {
          // STAR ...PIGLU4.YDC(16O)... ====================
          QString star = value.section('(', 0, 0);
          insertPropertyIf(plan, STAR, star.section('.', 0, 0));
          insertPropertyIf(plan, STARTRANS, star.section('.', 1, 1));
          QString rw = value.section('(', 1, 1).section(')', 0, 0);
          rw.remove('O');
          insertPropertyIf(plan, STARRW, rw);
        }
        else if(lastKey == "AP")
        {
          // Approach ...I16-Z.HUMEK ====================
          insertPropertyIf(plan, APPROACH_ARINC, value.section('.', 0, 0));
          insertPropertyIf(plan, TRANSITION, value.section('.', 1, 1));
        }
        else
          qWarning() << Q_FUNC_INFO << "Unknown key in file" << gfpFile.fileName() << lastKey;

        lastKey.clear();
      } // if(atools::contains(value,  ... else
    } // for(int i = 0; i < valueList.size(); i++)

    plan.flightplanType = NO_TYPE;
    plan.cruiseAltitudeFt = 0.f;
    plan.adjustDepartureAndDestination();
    plan.assignAltitudeToAllEntries();

    gfpFile.close();
  }
  else
    throw Exception(errorMsg.arg(filename).arg(gfpFile.errorString()));
}

void FlightplanIO::loadGarminFplGz(atools::fs::pln::Flightplan& plan, const QByteArray& bytes)
{
  loadGarminFplStr(plan, QString(atools::zip::gzipDecompress(bytes)));
}

void FlightplanIO::loadGarminFplStr(atools::fs::pln::Flightplan& plan, const QString& string)
{
  plan.clearAll();

  if(!string.isEmpty())
  {
    atools::util::XmlStream xmlStream(string);
    loadGarminFplInternal(plan, xmlStream);
  }
}

void FlightplanIO::loadGarminFplInternal(atools::fs::pln::Flightplan& plan, atools::util::XmlStream& xmlStream)
{
  QXmlStreamReader& reader = xmlStream.getReader();

  // ==================================================================================
  // Read all elements - order does not matter
  // Additional unknown elements are ignored and a warning is logged

  // Waypoint key is list of ident,region and type
  QHash<QStringList, FlightplanEntry> waypointIndex;

  xmlStream.readUntilElement("flight-plan");
  while(xmlStream.readNextStartElement())
  {
    if(reader.name() == "waypoint-table")
    {
      // Fill waypoint hash map =========================================================
      while(xmlStream.readNextStartElement())
      {
        if(reader.name() == "waypoint")
        {
          FlightplanEntry entry;
          QString type;
          Pos pos;
          while(xmlStream.readNextStartElement())
          {
            // . <waypoint>
            // .   <identifier>CYYZ</identifier>
            // .   <type>AIRPORT</type>
            // .   <country-code>CY</country-code>
            // .   <lat>43.677222</lat>
            // .   <lon>-79.6305555</lon>
            // .   <comment></comment>
            // .   <elevation>173.4312</elevation>
            // . </waypoint>

            if(reader.name() == "identifier")
              entry.setIdent(reader.readElementText());
            else if(reader.name() == "type")
              type = reader.readElementText();
            else if(reader.name() == "country-code")
              entry.setRegion(reader.readElementText());
            else if(reader.name() == "lat")
              pos.setLatY(reader.readElementText().toFloat());
            else if(reader.name() == "lon")
              pos.setLonX(reader.readElementText().toFloat());
            else if(reader.name() == "comment")
              entry.setComment(reader.readElementText());
            else if(reader.name() == "elevation")
              pos.setAltitude(reader.readElementText().toFloat());
            else
              xmlStream.skipCurrentElement(false /* warn */);
          }
          entry.setPosition(pos);
          entry.setWaypointType(garminToWaypointType(type));

          QStringList key = {entry.getIdent(), entry.getRegion(), type};

          if(waypointIndex.contains(key))
            qWarning() << Q_FUNC_INFO << "Duplicate key in waypoint index" << key;

          waypointIndex.insert(key, entry);
        }
        else
          xmlStream.skipCurrentElement(false /* warn */);
      }
    }
    else if(reader.name() == "route")
    {
      while(xmlStream.readNextStartElement())
      {
        // Read route points =========================================================
        if(reader.name() == "route-point")
        {
          // . <route-point>
          // .   <waypoint-identifier>CYYZ</waypoint-identifier>
          // .   <waypoint-type>AIRPORT</waypoint-type>
          // .   <waypoint-country-code>CY</waypoint-country-code>
          // . </route-point>

          QString ident, type, region;
          while(xmlStream.readNextStartElement())
          {
            if(reader.name() == "waypoint-identifier")
              ident = reader.readElementText();
            else if(reader.name() == "waypoint-type")
              type = reader.readElementText();
            else if(reader.name() == "waypoint-country-code")
              region = reader.readElementText();
            else
              xmlStream.skipCurrentElement(false /* warn */);
          }
          QStringList key = {ident, region, type};
          if(waypointIndex.contains(key))
            plan.append(waypointIndex.value(key));
          else
            qWarning() << Q_FUNC_INFO << "Key not found in waypoint index" << key;
        }
        else
          xmlStream.skipCurrentElement(false /* warn */);
      }
    }
    else
      xmlStream.skipCurrentElement(false /* warn */);
  }

  plan.flightplanType = NO_TYPE;
  plan.cruiseAltitudeFt = 0.f; // Use altitude as set in GUI
  plan.assignAltitudeToAllEntries();
  plan.adjustDepartureAndDestination();
}

atools::fs::pln::entry::WaypointType FlightplanIO::garminToWaypointType(const QString& typeStr) const
{
  if(typeStr == "AIRPORT")
    return atools::fs::pln::entry::AIRPORT;
  else if(typeStr == "VOR")
    return atools::fs::pln::entry::VOR;
  else if(typeStr == "NDB")
    return atools::fs::pln::entry::NDB;
  else if(typeStr == "INT")
    return atools::fs::pln::entry::WAYPOINT;

  return atools::fs::pln::entry::USER;
}

QString FlightplanIO::flightplanTypeToString(atools::fs::pln::FlightplanType type)
{
  switch(type)
  {
    case atools::fs::pln::NO_TYPE:
      return "NONE";

    case atools::fs::pln::IFR:
      return "IFR";

    case atools::fs::pln::VFR:
      return "VFR";
  }
  return QString();
}

FlightplanType FlightplanIO::stringFlightplanType(const QString& str)
{
  if(str.compare("IFR", Qt::CaseInsensitive) == 0)
    return IFR;
  else if(str.compare("VFR", Qt::CaseInsensitive) == 0)
    return VFR;

  return NO_TYPE;
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

int FlightplanIO::numEntriesSave(const atools::fs::pln::Flightplan& plan)
{
  int num = 0;
  for(const FlightplanEntry& entry : plan)
  {
    if(!entry.isNoSave())
      // Do not save entries like procedure points
      num++;
  }
  return num;
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

    case atools::fs::pln::entry::WAYPOINT:
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

  QByteArray bytes = str.toLatin1();
  const char *data = bytes.constData();
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

/*
 *  MSFS
 *  <AppVersion>
 *      <AppVersionMajor>11</AppVersionMajor>
 *      <AppVersionBuild>282174</AppVersionBuild>
 *  </AppVersion>
 *  FSX
 *  <AppVersion>
 *      <AppVersionMajor>10</AppVersionMajor>
 *      <AppVersionBuild>61472</AppVersionBuild>
 *  </AppVersion>
 */
void FlightplanIO::readAppVersionPln(int& appVersionMajor, int& appVersionBuild, atools::util::XmlStream& xmlStream)
{
  while(xmlStream.readNextStartElement())
  {
    QStringRef aName = xmlStream.getReader().name();
    if(aName == "AppVersionMajor")
      appVersionMajor = xmlStream.getReader().readElementText().toInt();
    else if(aName == "xmlStream")
      appVersionBuild = xmlStream.getReader().readElementText().toInt();
    else
      xmlStream.skipCurrentElement();
  }
}

/*
 * <ATCWaypoint id="DH111">
 *     <ATCWaypointType>Intersection</ATCWaypointType>
 *     <WorldPosition>N53° 34' 31.74",E9° 52' 32.92",+001100.00</WorldPosition>
 *     <DepartureFP>AMLU1B</DepartureFP>
 *     <RunwayNumberFP>23</RunwayNumberFP>
 *     <ICAO>
 *         <ICAORegion>ED</ICAORegion>
 *         <ICAOIdent>DH111</ICAOIdent>
 *         <ICAOAirport>EDDH</ICAOAirport>
 *     </ICAO>
 * </ATCWaypoint>
 */
void FlightplanIO::readWaypointPln(atools::fs::pln::Flightplan& plan, atools::util::XmlStream& xmlStream)
{
  FlightplanEntry entry;
  QXmlStreamReader& reader = xmlStream.getReader();

  entry.setIdent(reader.attributes().value("id").toString());
  QString runway, designator, approach, approachTransition, suffix;

  while(xmlStream.readNextStartElement())
  {
    QString name = reader.name().toString();
    if(name == "ATCWaypointType")
      entry.setWaypointType(reader.readElementText());
    else if(name == "WorldPosition")
      entry.setPosition(geo::Pos(reader.readElementText()));
    else if(name == "ATCAirway")
      entry.setAirway(reader.readElementText());

    // MSFS
    else if(name == "RunwayNumberFP")
      runway = reader.readElementText();
    else if(name == "RunwayDesignatorFP")
      designator = reader.readElementText();
    else if(name == "DepartureFP")
      entry.setSid(reader.readElementText());
    else if(name == "ArrivalFP")
      entry.setStar(reader.readElementText());
    else if(name == "ApproachTypeFP")
      approach = reader.readElementText();
    else if(name == "SuffixFP")
      suffix = reader.readElementText();

    else if(name == "ICAO")
    {
      while(xmlStream.readNextStartElement())
      {
        QStringRef iName = reader.name();
        if(iName == "ICAORegion")
          entry.setRegion(reader.readElementText());
        else if(iName == "ICAOIdent")
          entry.setIdent(reader.readElementText());
        else if(iName == "ICAOAirport") // MSFS
          entry.setAirport(reader.readElementText());
        else
          reader.skipCurrentElement();
      }
    }
    else
      reader.skipCurrentElement();
  }
  entry.setRunway(runway, designator);
  entry.setApproach(approach, suffix, approachTransition);

  plan.append(entry);
}

void FlightplanIO::writePropertyFloat(QXmlStreamWriter& writer, const QString& name, float value)
{
  writer.writeStartElement(name);
  writer.writeAttribute("type", "double");
  writer.writeCharacters(QString::number(value, 'f', 8));
  writer.writeEndElement();
}

void FlightplanIO::writePropertyInt(QXmlStreamWriter& writer, const QString& name, int value)
{
  writer.writeStartElement(name);
  writer.writeAttribute("type", "int");
  writer.writeCharacters(QString::number(value));
  writer.writeEndElement();
}

void FlightplanIO::writePropertyBool(QXmlStreamWriter& writer, const QString& name, bool value)
{
  writer.writeStartElement(name);
  writer.writeAttribute("type", "bool");
  writer.writeCharacters(value ? "true" : "false");
  writer.writeEndElement();
}

void FlightplanIO::writePropertyStr(QXmlStreamWriter& writer, const QString& name, const QString& value)
{
  writer.writeStartElement(name);
  writer.writeAttribute("type", "string");
  writer.writeCharacters(value);
  writer.writeEndElement();
}

QString FlightplanIO::identOrDegMinFormat(const atools::fs::pln::FlightplanEntry& entry)
{
  if(entry.getWaypointType() == atools::fs::pln::entry::USER ||
     entry.getWaypointType() == atools::fs::pln::entry::UNKNOWN)
    return atools::fs::util::toDegMinFormat(entry.getPosition());
  else
    return entry.getIdent();
}

} // namespace pln
} // namespace fs
} // namespace atools
