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

#include "fs/xp/xpnavreader.h"

#include "fs/common/airportindex.h"
#include "fs/util/fsutil.h"
#include "fs/util/tacanfrequencies.h"
#include "fs/progresshandler.h"
#include "fs/navdatabaseoptions.h"
#include "fs/common/magdecreader.h"

#include "geo/pos.h"
#include "geo/calculations.h"
#include "sql/sqlutil.h"
#include "sql/sqlquery.h"

using atools::sql::SqlQuery;
using atools::sql::SqlUtil;
using atools::geo::Pos;

namespace atools {
namespace fs {
namespace xp {

// 4  36.692211111    3.217516667      131    11030    25    232.655   AG DAAG DA 23 ILS-cat-III
// 6  36.710150000    3.249166667      131    11030    25 300232.655   AG DAAG DA 23 GS
// 12  36.710150000    3.249166667      131    11030    25      0.000   AG DAAG DA HOUARI BOUMEDIENE DME-ILS
enum FieldIndex
{
  ROWCODE = 0,
  LATY = 1,
  LONX = 2,
  ALT = 3,
  FREQ = 4, // Or SBAS/GBAS channel
  RANGE = 5, // Or length Offset in meters, from stop end of runway to FPAP or path point threshold crossing height, feet
  HDG = 6,
  MAGVAR = HDG,
  IDENT = 7,
  AIRPORT = 8,
  REGION = 9,
  RW = 10,
  NAME = 11
};

XpNavReader::XpNavReader(atools::sql::SqlDatabase& sqlDb, atools::fs::common::AirportIndex *airportIndexParam,
                         const NavDatabaseOptions& opts, ProgressHandler *progressHandler,
                         atools::fs::NavDatabaseErrors *navdatabaseErrors)
  : XpReader(sqlDb, opts, progressHandler, navdatabaseErrors), airportIndex(airportIndexParam)
{
  initQueries();
}

atools::fs::xp::XpNavReader::~XpNavReader()
{
  deInitQueries();
}

void XpNavReader::writeVor(const QStringList& line, int curFileId, bool dmeOnly)
{
  // X-Plane 12 definition
  // 25 = terminal, 40 = low altitude, 130 = high altitude, 125 =
  // unspecified but likely high power VOR. Uses the higher of 5.35
  // class and 5.149 figure of merit.

  int range = at(line, RANGE).toInt();
  QString type;
  QString rangeType;

  if(range == 0)
  {
    rangeType = QStringLiteral("H");
    insertVorQuery->bindNullInt(QStringLiteral(":range"));
  }
  else if(range == 125)
  {
    rangeType = QStringLiteral("H");
    // likely high power VOR
    insertVorQuery->bindValue(QStringLiteral(":range"), 130);
  }
  else
  {
    if(range < 30)
      rangeType = QStringLiteral("T");
    else if(range < 50)
      rangeType = QStringLiteral("L");
    else if(range < 140)
      rangeType = QStringLiteral("H");
    insertVorQuery->bindValue(QStringLiteral(":range"), range);
  }

  QString suffix = line.constLast().toUpper();
  if(suffix == QStringLiteral("VOR") || suffix == QStringLiteral("DME") || suffix == QStringLiteral("VOR-DME") ||
     suffix == QStringLiteral("VOR/DME"))
    type = rangeType;
  else if(suffix == QStringLiteral("VORTAC"))
    type = QStringLiteral("VT") + rangeType;
  else if(suffix == QStringLiteral("TACAN"))
    type = QStringLiteral("TC");

  bool hasDme = suffix == QStringLiteral("DME") || suffix == QStringLiteral("VORTAC") || suffix == QStringLiteral("VOR-DME") ||
                suffix == QStringLiteral("VOR/DME");
  int frequency = at(line, FREQ).toInt();

  insertVorQuery->bindValue(QStringLiteral(":vor_id"), ++curVorId);
  insertVorQuery->bindValue(QStringLiteral(":file_id"), curFileId);
  insertVorQuery->bindValue(QStringLiteral(":ident"), at(line, IDENT));
  insertVorQuery->bindValue(QStringLiteral(":name"), line.mid(RW, line.size() - 11).join(QStringLiteral(" ")));
  insertVorQuery->bindValue(QStringLiteral(":region"), at(line, REGION));
  insertVorQuery->bindValue(QStringLiteral(":type"), type);
  insertVorQuery->bindValue(QStringLiteral(":frequency"), frequency * 10);
  insertVorQuery->bindValue(QStringLiteral(":mag_var"), at(line, MAGVAR).toFloat());
  insertVorQuery->bindValue(QStringLiteral(":dme_only"), dmeOnly);
  insertVorQuery->bindValue(QStringLiteral(":airport_id"), airportIndex->getAirportIdVar(at(line, AIRPORT), false /* allIdents */));
  insertVorQuery->bindValue(QStringLiteral(":airport_ident"), atAirportIdent(line, AIRPORT));

  if(suffix == QStringLiteral("TACAN") || suffix == QStringLiteral("VORTAC"))
    insertVorQuery->bindValue(QStringLiteral(":channel"), atools::fs::util::tacanChannelForFrequency(frequency));
  else
    insertVorQuery->bindNullStr(QStringLiteral(":channel"));

  if(hasDme)
  {
    insertVorQuery->bindValue(QStringLiteral(":dme_altitude"), at(line, ALT).toInt());
    insertVorQuery->bindValue(QStringLiteral(":dme_lonx"), at(line, LONX).toFloat());
    insertVorQuery->bindValue(QStringLiteral(":dme_laty"), at(line, LATY).toFloat());
    insertVorQuery->bindValue(QStringLiteral(":altitude"), at(line, ALT).toInt());
  }
  else
  {
    insertVorQuery->bindNullInt(QStringLiteral(":dme_altitude"));
    insertVorQuery->bindNullFloat(QStringLiteral(":dme_lonx"));
    insertVorQuery->bindNullFloat(QStringLiteral(":dme_laty"));

    // VOR only - unlikely to have an elevation
    if(at(line, ALT).toInt() != 0)
      insertVorQuery->bindValue(QStringLiteral(":altitude"), at(line, ALT).toInt());
    else
      insertVorQuery->bindNullInt(QStringLiteral(":altitude"));
  }

  insertVorQuery->bindValue(QStringLiteral(":lonx"), at(line, LONX).toFloat());
  insertVorQuery->bindValue(QStringLiteral(":laty"), at(line, LATY).toFloat());

  insertVorQuery->exec();

  progress->incNumVors();
}

void XpNavReader::writeNdb(const QStringList& line, int curFileId, const XpReaderContext& context)
{
  int range = at(line, RANGE).toInt();

  QString type;
  if(range < 24)
    type = QStringLiteral("CP");
  else if(range < 40)
    type = QStringLiteral("MH");
  else if(range < 70)
    type = QStringLiteral("H");
  else
    type = QStringLiteral("HH");

  Pos pos(at(line, LONX).toFloat(), at(line, LATY).toFloat());

  insertNdbQuery->bindValue(QStringLiteral(":ndb_id"), ++curNdbId);
  insertNdbQuery->bindValue(QStringLiteral(":file_id"), curFileId);
  insertNdbQuery->bindValue(QStringLiteral(":ident"), at(line, IDENT));
  insertNdbQuery->bindValue(QStringLiteral(":name"), line.mid(RW, line.size() - 11).join(QStringLiteral(" ")));
  insertNdbQuery->bindValue(QStringLiteral(":region"), at(line, REGION));
  insertNdbQuery->bindValue(QStringLiteral(":type"), type);
  insertNdbQuery->bindValue(QStringLiteral(":frequency"), at(line, FREQ).toInt() * 100);
  insertNdbQuery->bindValue(QStringLiteral(":range"), range);
  insertNdbQuery->bindValue(QStringLiteral(":airport_id"), airportIndex->getAirportIdVar(at(line, AIRPORT), false /* allIdents */));
  insertNdbQuery->bindValue(QStringLiteral(":airport_ident"), atAirportIdent(line, AIRPORT));

  // NDBs never have an altitude
  if(at(line, ALT).toInt() != 0)
    insertNdbQuery->bindValue(QStringLiteral(":altitude"), at(line, ALT).toInt());
  else
    insertNdbQuery->bindNullInt(QStringLiteral(":altitude"));
  insertNdbQuery->bindValue(QStringLiteral(":mag_var"), context.magDecReader->getMagVar(pos));
  insertNdbQuery->bindValue(QStringLiteral(":lonx"), pos.getLonX());
  insertNdbQuery->bindValue(QStringLiteral(":laty"), pos.getLatY());

  insertNdbQuery->exec();

  progress->incNumNdbs();
}

void XpNavReader::writeMarker(const QStringList& line, int curFileId, NavRowCode rowCode)
{
  QString type;
  if(rowCode == OM)
    type = QStringLiteral("OUTER");
  else if(rowCode == MM)
    type = QStringLiteral("MIDDLE");
  else if(rowCode == IM)
    type = QStringLiteral("INNER");

  insertMarkerQuery->bindValue(QStringLiteral(":marker_id"), ++curMarkerId);
  insertMarkerQuery->bindValue(QStringLiteral(":file_id"), curFileId);
  insertMarkerQuery->bindValue(QStringLiteral(":region"), at(line, REGION));
  insertMarkerQuery->bindValue(QStringLiteral(":type"), type);
  insertMarkerQuery->bindValue(QStringLiteral(":ident"), at(line, IDENT));
  insertMarkerQuery->bindValue(QStringLiteral(":heading"), at(line, HDG).toFloat());
  insertMarkerQuery->bindValue(QStringLiteral(":altitude"), at(line, ALT).toInt());
  insertMarkerQuery->bindValue(QStringLiteral(":lonx"), at(line, LONX).toFloat());
  insertMarkerQuery->bindValue(QStringLiteral(":laty"), at(line, LATY).toFloat());

  insertMarkerQuery->exec();

  progress->incNumMarker();
}

QChar XpNavReader::ilsType(const QString& name, bool glideslope)
{
  // ILS Localizer only, no glideslope   0
  // ILS Localizer/MLS/GLS Unknown cat   U
  // ILS Localizer/MLS/GLS Cat I         1
  // ILS Localizer/MLS/GLS Cat II        2
  // ILS Localizer/MLS/GLS Cat III       3
  // IGS Facility                        I
  // LDA Facility with glideslope        L
  // LDA Facility no glideslope          A
  // SDF Facility with glideslope        S
  // SDF Facility no glideslope          F

  QChar type('\0');

  // Read type from name
  if(name == QStringLiteral("LOC"))
    type = '0';
  else if(name == QStringLiteral("SDF"))
    type = glideslope ? 'S' : 'F';
  else if(name == QStringLiteral("LDA"))
    type = glideslope ? 'L' : 'A';
  else if(name == QStringLiteral("IGS"))
    type = 'I';
  else
  {
    if(glideslope)
    {
      if(name.contains(QStringLiteral("CAT-III")) || name.contains(QStringLiteral("CAT III")) || name.contains(QStringLiteral("CATIII")))
        type = '3';
      else if(name.contains(QStringLiteral("CAT-II")) || name.contains(QStringLiteral("CAT II")) || name.contains(QStringLiteral("CATII")))
        type = '2';
      else if(name.contains(QStringLiteral("CAT-I")) || name.contains(QStringLiteral("CAT I")) || name.contains(QStringLiteral("CATI")))
        type = '1';
      else
        type = 'U'; // ILS of unknown category
    }
    else
      type = '0'; // Localizer
  }

  return type;
}

void XpNavReader::updateSbasGbasThreshold(const QStringList& line)
{
  /*  SBAS_GBAS_THRESHOLD 16 Landing threshold point or fictitious threshold point of an SBAS/GBAS approach */
  const QString& airportIdent = at(line, AIRPORT);
  const QString& airportRegion = at(line, REGION);
  const QString& ilsIdent = at(line, IDENT);

  if(airportIndex->hasSkippedAirportIls(airportIdent, airportRegion, ilsIdent))
    // Already skipped in this file
    return;

  int ilsId = airportIndex->getAirportIlsId(airportIdent, airportRegion, ilsIdent);

  Pos pos(at(line, LONX).toFloat(), at(line, LATY).toFloat());

  // pos = airportIndex->getRunwayEndPos(airportIdent, runwayName);

  float hdg = at(line, HDG).toFloat();
  float heading = atools::geo::normalizeCourse(std::fmod(hdg, 1000.f));
  float pitch = std::floor(hdg / 1000.f) / 100.f;

  updateSbasGbasThresholdQuery->bindValue(QStringLiteral(":id"), ilsId);
  updateSbasGbasThresholdQuery->bindValue(QStringLiteral(":gs_altitude"), at(line, ALT).toInt());
  updateSbasGbasThresholdQuery->bindValue(QStringLiteral(":gs_lonx"), pos.getLonX());
  updateSbasGbasThresholdQuery->bindValue(QStringLiteral(":gs_laty"), pos.getLatY());
  updateSbasGbasThresholdQuery->bindValue(QStringLiteral(":gs_pitch"), pitch);
  updateSbasGbasThresholdQuery->bindValue(QStringLiteral(":loc_heading"), heading);
  updateSbasGbasThresholdQuery->bindValue(QStringLiteral(":type"), QStringLiteral("T"));
  updateSbasGbasThresholdQuery->bindValue(QStringLiteral(":provider"), at(line, NAME));
  updateSbasGbasThresholdQuery->bindValue(QStringLiteral(":lonx"), pos.getLonX());
  updateSbasGbasThresholdQuery->bindValue(QStringLiteral(":laty"), pos.getLatY());

  assignIlsGeometry(updateSbasGbasThresholdQuery, pos, heading, ILS_FEATHER_WIDTH_DEG * 2.f);

  // updateSbasGbasThresholdQuery->bindValue(QStringLiteral(":gs_range"), atools::geo::meterToFeet(at(line, RANGE).toInt()));
  // updateSbasGbasThresholdQuery->bindValue(QStringLiteral(":range"), at(line, RANGE).toInt());

  updateSbasGbasThresholdQuery->exec();
  updateSbasGbasThresholdQuery->clearBoundValues();
}

void XpNavReader::writeIlsSbasGbas(const QStringList& line, NavRowCode rowCode, const XpReaderContext& context)
{
  const QString& airportIdent = at(line, AIRPORT);
  const QString& airportRegion = at(line, REGION);
  const QString& ilsIdent = at(line, IDENT);

  if(airportIndex->getAirportIlsId(airportIdent, airportRegion, ilsIdent) != -1)
  {
    // Remember what was skipped in this file so DME and GS update functions can skip too
    airportIndex->addSkippedAirportIls(airportIdent, airportRegion, ilsIdent);
    return;
  }

  airportIndex->addAirportIls(airportIdent, airportRegion, ilsIdent, ++curIlsId);

  const QString& runwayName = at(line, RW);
  Pos pos(at(line, LONX).toFloat(), at(line, LATY).toFloat());

  insertIlsQuery->bindValue(QStringLiteral(":ils_id"), curIlsId);

  ilsName = line.mid(NAME).join(QStringLiteral(" ")).simplified().toUpper();
  float heading = at(line, HDG).toFloat();
  float width = 0.f;

  if(rowCode == SBAS_GBAS_FINAL)
  {
    /*  14 Final approach path alignment point of an SBAS or GBAS approach path */
    insertIlsQuery->bindValue(QStringLiteral(":perf_indicator"), at(line, NAME));
    insertIlsQuery->bindValue(QStringLiteral(":frequency"), at(line, FREQ).toInt());
    width = ILS_FEATHER_WIDTH_DEG * 2.f;
  }
  else if(rowCode == GBAS)
  {
    /*  15 GBAS differential ground station of a GLS */
    Pos rwpos = airportIndex->getRunwayEndPos(airportIdent, runwayName, false /* allAirportIdents */);
    if(rwpos.isValid())
      pos = rwpos;
    // else Use station position as a fall back

    insertIlsQuery->bindValue(QStringLiteral(":frequency"), at(line, FREQ).toInt());
    insertIlsQuery->bindValue(QStringLiteral(":type"), QStringLiteral("G"));
    insertIlsQuery->bindValue(QStringLiteral(":gs_pitch"), std::floor(at(line, HDG).toFloat() / 1000.f) / 100.f);
    heading = atools::geo::normalizeCourse(std::fmod(heading, 1000.f));
    width = RNV_FEATHER_WIDTH_DEG;
  }
  else
  {
    // Normal ILS, SDF, LOC, etc.
    insertIlsQuery->bindValue(QStringLiteral(":frequency"), at(line, FREQ).toInt() * 10);

    // Is probably updated later in updateIlsGlideslope()
    insertIlsQuery->bindValue(QStringLiteral(":type"), ilsType(ilsName, false /* glideslope */));
    insertIlsQuery->bindValue(QStringLiteral(":range"), at(line, RANGE).toInt());
    heading = atools::geo::normalizeCourse(heading);
    width = ILS_FEATHER_WIDTH_DEG;
  }

  insertIlsQuery->bindValue(QStringLiteral(":loc_heading"), heading);
  insertIlsQuery->bindValue(QStringLiteral(":ident"), ilsIdent);
  insertIlsQuery->bindValue(QStringLiteral(":loc_airport_ident"), airportIdent);
  insertIlsQuery->bindValue(QStringLiteral(":region"), at(line, REGION));
  insertIlsQuery->bindValue(QStringLiteral(":loc_runway_name"), runwayName);
  insertIlsQuery->bindValue(QStringLiteral(":name"), ilsName);
  insertIlsQuery->bindValue(QStringLiteral(":loc_runway_end_id"),
                            airportIndex->getRunwayEndIdVar(airportIdent, runwayName, false /* allAirportIdents */));
  insertIlsQuery->bindValue(QStringLiteral(":altitude"), at(line, ALT).toInt());
  insertIlsQuery->bindValue(QStringLiteral(":lonx"), pos.getLonX());
  insertIlsQuery->bindValue(QStringLiteral(":laty"), pos.getLatY());

  insertIlsQuery->bindValue(QStringLiteral(":mag_var"), context.magDecReader->getMagVar(pos));
  insertIlsQuery->bindNullFloat(QStringLiteral(":loc_width"));
  insertIlsQuery->bindValue(QStringLiteral(":has_backcourse"), 0);

  if(rowCode != SBAS_GBAS_FINAL)
    assignIlsGeometry(insertIlsQuery, pos, heading, width);

  insertIlsQuery->exec();
  insertIlsQuery->clearBoundValues();
  progress->incNumIls();
}

void XpNavReader::assignIlsGeometry(atools::sql::SqlQuery *query, const atools::geo::Pos& pos, float heading, float width)
{
  Pos p1, p2, pmid;
  atools::fs::util::calculateIlsGeometry(pos, heading, width, atools::fs::util::DEFAULT_FEATHER_LEN_NM, p1, p2, pmid);

  query->bindValue(QStringLiteral(":end1_lonx"), p1.getLonX());
  query->bindValue(QStringLiteral(":end1_laty"), p1.getLatY());
  query->bindValue(QStringLiteral(":end_mid_lonx"), pmid.getLonX());
  query->bindValue(QStringLiteral(":end_mid_laty"), pmid.getLatY());
  query->bindValue(QStringLiteral(":end2_lonx"), p2.getLonX());
  query->bindValue(QStringLiteral(":end2_laty"), p2.getLatY());
}

void XpNavReader::updateIlsGlideslope(const QStringList& line)
{
  const QString& airportIdent = at(line, AIRPORT);
  const QString& airportRegion = at(line, REGION);
  const QString& ilsIdent = at(line, IDENT);

  if(airportIndex->hasSkippedAirportIls(airportIdent, airportRegion, ilsIdent))
    // Already skipped in this file
    return;

  int ilsId = airportIndex->getAirportIlsId(airportIdent, airportRegion, ilsIdent);

  updateIlsGsTypeQuery->bindValue(QStringLiteral(":id"), ilsId);
  updateIlsGsTypeQuery->bindValue(QStringLiteral(":gs_pitch"), std::floor(at(line, HDG).toFloat() / 1000.f) / 100.f);
  updateIlsGsTypeQuery->bindValue(QStringLiteral(":gs_range"), at(line, RANGE).toInt());
  updateIlsGsTypeQuery->bindValue(QStringLiteral(":gs_altitude"), at(line, ALT).toInt());
  updateIlsGsTypeQuery->bindValue(QStringLiteral(":gs_lonx"), at(line, LONX).toFloat());
  updateIlsGsTypeQuery->bindValue(QStringLiteral(":gs_laty"), at(line, LATY).toFloat());
  updateIlsGsTypeQuery->bindValue(QStringLiteral(":type"), ilsType(ilsName, true /* glideslope */)); // Update cat since gs is now available
  updateIlsGsTypeQuery->exec();
  updateIlsGsTypeQuery->clearBoundValues();
}

void XpNavReader::updateIlsDme(const QStringList& line)
{
  const QString& airportIdent = at(line, AIRPORT);
  const QString& airportRegion = at(line, REGION);
  const QString& ilsIdent = at(line, IDENT);

  if(airportIndex->hasSkippedAirportIls(airportIdent, airportRegion, ilsIdent))
    // Already skipped in this file
    return;

  int ilsId = airportIndex->getAirportIlsId(airportIdent, airportRegion, ilsIdent);

  updateIlsDmeQuery->bindValue(QStringLiteral(":id"), ilsId);
  updateIlsDmeQuery->bindValue(QStringLiteral(":dme_range"), at(line, RANGE).toInt());
  updateIlsDmeQuery->bindValue(QStringLiteral(":dme_altitude"), at(line, ALT).toInt());
  updateIlsDmeQuery->bindValue(QStringLiteral(":dme_lonx"), at(line, LONX).toFloat());
  updateIlsDmeQuery->bindValue(QStringLiteral(":dme_laty"), at(line, LATY).toFloat());
  updateIlsDmeQuery->exec();
  updateIlsDmeQuery->clearBoundValues();
}

void XpNavReader::read(const QStringList& line, const XpReaderContext& context)
{
  ctx = &context;

  // lat lon
  // ("28.000708333", "-83.423330556", "KNOST", "ENRT", "K7")
  // if(at(line, IDENT) == "OEV")
  // qDebug() << "OEV";

  NavRowCode rowCode = static_cast<NavRowCode>(at(line, ROWCODE).toInt());

  // Glideslope records must come later in the file than their associated localizer
  // LTP/FTP records must come later in the file than their associated FPAP
  // Paired DME records must come later in the file than their associated VOR or TACAN
  switch(rowCode)
  {
    // 2 NDB (Non-Directional Beacon) Includes NDB component of Locator Outer Markers (LOM)
    case NDB:
      if(options.isIncludedNavDbObject(atools::fs::type::NDB))
        writeNdb(line, context.curFileId, context);
      break;

    // 3 VOR (including VOR-DME and VORTACs) Includes VORs, VOR-DMEs, TACANs and VORTACs
    case VOR:
      if(options.isIncludedNavDbObject(atools::fs::type::VOR))
        writeVor(line, context.curFileId, false);
      break;

    case LOC: // 4 Localizer component of an ILS (Instrument Landing System)
    case LOC_ONLY: // 5 Localizer component of a localizer-only approach Includes for LDAs and SDFs
    case GBAS: // 15 GBAS differential ground station of a GLS
    case SBAS_GBAS_FINAL: // 14 Final approach path alignment point of an SBAS or GBAS approach path
      if(options.isIncludedNavDbObject(atools::fs::type::ILS))
        writeIlsSbasGbas(line, rowCode, context);
      break;

    case SBAS_GBAS_THRESHOLD: // 16 Landing threshold point or fictitious threshold point of an SBAS/GBAS approach
      // Always follows on 14
      updateSbasGbasThreshold(line);
      break;

    // 6 Glideslope component of an ILS Frequency shown is paired frequency, notthe DME channel
    case GS:
      updateIlsGlideslope(line);
      break;

    // 7 Outer markers (OM) for an ILS Includes outer maker component of LOMs
    case OM:
    // 8 Middle markers (MM) for an ILS
    case MM:
    // 9 Inner markers (IM) for an ILS
    case IM:
      if(options.isIncludedNavDbObject(atools::fs::type::MARKER))
        writeMarker(line, context.curFileId, rowCode);
      break;

    // 12 DME, including the DME component of an ILS, VORTAC or VOR-DME Paired frequency display suppressed on X-Plane’s charts
    case DME:
      if(options.isIncludedNavDbObject(atools::fs::type::ILS))
      {
        if(line.constLast() == QStringLiteral("DME-ILS"))
          updateIlsDme(line);
      }
      break;

    // 13 Stand-alone DME, or the DME component of an NDB-DME Paired frequency will be displayed on X-Plane’s charts
    case DME_ONLY:
      if(options.isIncludedNavDbObject(atools::fs::type::ILS))
      {
        if(line.constLast() == QStringLiteral("DME-ILS"))
          updateIlsDme(line);
        else
          writeVor(line, true, context.curFileId);
      }
      break;
  }
}

void XpNavReader::finish(const XpReaderContext& context)
{
  Q_UNUSED(context)
}

void XpNavReader::reset()
{
  airportIndex->clearSkippedIls();
}

void XpNavReader::initQueries()
{
  deInitQueries();

  atools::sql::SqlUtil util(&db);

  insertVorQuery = new SqlQuery(db);
  insertVorQuery->prepare(util.buildInsertStatement("vor"));

  insertNdbQuery = new SqlQuery(db);
  insertNdbQuery->prepare(util.buildInsertStatement("ndb"));

  insertMarkerQuery = new SqlQuery(db);
  insertMarkerQuery->prepare(util.buildInsertStatement("marker"));

  insertIlsQuery = new SqlQuery(db);
  insertIlsQuery->prepare(util.buildInsertStatement("ils"));

  updateIlsDmeQuery = new SqlQuery(db);
  updateIlsDmeQuery->prepare("update ils set dme_range = :dme_range, dme_altitude = :dme_altitude, "
                             "dme_lonx = :dme_lonx, dme_laty = :dme_laty where ils_id = :id");

  updateIlsGsTypeQuery = new SqlQuery(db);
  updateIlsGsTypeQuery->prepare("update ils set type = :type, "
                                "gs_range = :gs_range, gs_pitch = :gs_pitch, gs_altitude = :gs_altitude, "
                                "gs_lonx = :gs_lonx, gs_laty = :gs_laty where ils_id = :id");

  updateSbasGbasThresholdQuery = new SqlQuery(db);
  updateSbasGbasThresholdQuery->prepare("update ils set "
                                        "gs_altitude = :gs_altitude, gs_lonx = :gs_lonx, gs_laty = :gs_laty, "
                                        "gs_pitch = :gs_pitch, "
                                        "loc_heading = :loc_heading, type = :type, "
                                        "provider = :provider, lonx = :lonx, laty = :laty, "
                                        "end1_lonx = :end1_lonx, end1_laty = :end1_laty, "
                                        "end_mid_lonx = :end_mid_lonx, end_mid_laty = :end_mid_laty, "
                                        "end2_lonx = :end2_lonx, end2_laty = :end2_laty "
                                        "where ils_id = :id");
}

void XpNavReader::deInitQueries()
{
  delete insertVorQuery;
  insertVorQuery = nullptr;

  delete insertNdbQuery;
  insertNdbQuery = nullptr;

  delete insertMarkerQuery;
  insertMarkerQuery = nullptr;

  delete insertIlsQuery;
  insertIlsQuery = nullptr;

  delete updateIlsDmeQuery;
  updateIlsDmeQuery = nullptr;

  delete updateIlsGsTypeQuery;
  updateIlsGsTypeQuery = nullptr;

  delete updateSbasGbasThresholdQuery;
  updateSbasGbasThresholdQuery = nullptr;
}

} // namespace xp
} // namespace fs
} // namespace atools
