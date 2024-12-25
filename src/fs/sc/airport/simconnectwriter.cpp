/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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

#include "fs/sc/airport/simconnectwriter.h"

#include "atools.h"
#include "exception.h"
#include "fs/bgl/ap/airport.h"
#include "fs/bgl/ap/approachleg.h"
#include "fs/bgl/ap/approachtypes.h"
#include "fs/bgl/ap/com.h"
#include "fs/bgl/ap/helipad.h"
#include "fs/bgl/ap/parking.h"
#include "fs/bgl/ap/rw/runwayapplights.h"
#include "fs/bgl/ap/rw/runwayvasi.h"
#include "fs/bgl/ap/start.h"
#include "fs/bgl/ap/taxipath.h"
#include "fs/bgl/ap/taxipoint.h"
#include "fs/bgl/ap/transition.h"
#include "fs/bgl/converter.h"
#include "fs/bgl/util.h"
#include "fs/db/runwayindex.h"
#include "fs/sc/airport/simconnectfacilities.h"
#include "fs/util/fsutil.h"
#include "geo/calculations.h"
#include "geo/rect.h"
#include "sql/sqlquery.h"
#include "sql/sqltransaction.h"
#include "sql/sqlutil.h"

#include <QString>
#include <QStringBuilder>
#include <QDebug>
#include <QCoreApplication>
#include <QThread>

namespace atools {
namespace fs {
namespace sc {
namespace airport {

using atools::sql::SqlQuery;
using atools::sql::SqlUtil;
using atools::geo::Rect;
using atools::geo::Pos;
using atools::geo::meterToFeet;
using atools::geo::meterToNm;
using atools::fs::bgl::util::enumToStr;

// Binding inline helper functions ===================================================
inline void bindPos(atools::sql::SqlQuery *query, const geo::Pos& pos, const QString& prefix = QString())
{
  if(pos.isValid() && !pos.isNull())
  {
    query->bindValue(":" % prefix % "lonx", pos.getLonX());
    query->bindValue(":" % prefix % "laty", pos.getLatY());
  }
  else
  {
    query->bindNullFloat(":" % prefix % "lonx");
    query->bindNullFloat(":" % prefix % "laty");
  }
}

inline void bindPosAlt(atools::sql::SqlQuery *query, const atools::geo::Pos& pos, const QString& prefix = QString())
{
  bindPos(query, pos, prefix);

  if(pos.isValid() && !pos.isNull())
    query->bindValue(":" % prefix % "altitude", pos.getAltitude());
  else
    query->bindNullFloat(":" % prefix % "altitude");
}

inline void bindPos(atools::sql::SqlQuery *query, float lonX, float latY, const QString& prefix = QString())
{
  bindPos(query, Pos(lonX, latY), prefix);
}

inline void bindPos(atools::sql::SqlQuery *query, double lonX, double latY, const QString& prefix = QString())
{
  bindPos(query, Pos(lonX, latY), prefix);
}

inline atools::geo::Pos calcPosFromBias(const atools::geo::Pos& airportPos, float biasXMeter, float biasYMeter)
{
  // Calculate offset position from bias in meter
  atools::geo::PosD pos(airportPos);
  pos.setLonX(pos.endpoint(biasXMeter, 90.).getLonX());
  pos.setLatY(pos.getLatY() + meterToNm(static_cast<double>(biasYMeter)) / 60.);
  return pos.asPos();
}

// ====================================================================================================

SimConnectWriter::SimConnectWriter(sql::SqlDatabase& sqlDb, bool verboseParam)
  : db(sqlDb), verbose(verboseParam)
{
}

SimConnectWriter::~SimConnectWriter()
{
  deInitQueries();
}

void SimConnectWriter::initQueries()
{
  deInitQueries();

  SqlUtil util(db);

  // create insert statement for table airport and exclude columns
  airportStmt = new SqlQuery(db);
  airportStmt->prepare(util.buildInsertStatement("airport", QString(),
                                                 {"icao", "iata", "faa", "local", "city", "state", "country", "flatten", "type",
                                                  "has_tower_object", "scenery_local_path", "bgl_filename"}));

  airportFileStmt = new SqlQuery(db);
  airportFileStmt->prepare(util.buildInsertStatement("airport_file"));

  runwayStmt = new SqlQuery(db);
  runwayStmt->prepare(util.buildInsertStatement("runway", QString(), {"smoothness", "shoulder"}));

  runwayEndStmt = new SqlQuery(db);
  runwayEndStmt->prepare(util.buildInsertStatement("runway_end", QString(), {"num_strobes"}));

  startStmt = new SqlQuery(db);
  startStmt->prepare(util.buildInsertStatement("start"));

  frequencyStmt = new SqlQuery(db);
  frequencyStmt->prepare(util.buildInsertStatement("com"));

  helipadStmt = new SqlQuery(db);
  helipadStmt->prepare(util.buildInsertStatement("helipad"));

  taxiPathStmt = new SqlQuery(db);
  taxiPathStmt->prepare(util.buildInsertStatement("taxi_path"));

  parkingStmt = new SqlQuery(db);
  parkingStmt->prepare(util.buildInsertStatement("parking", QString(), {"pushback", "airline_codes"}));

  approachStmt = new SqlQuery(db);
  approachStmt->prepare(util.buildInsertStatement("approach"));

  transitionStmt = new SqlQuery(db);
  transitionStmt->prepare(util.buildInsertStatement("transition"));

  approachLegStmt = new SqlQuery(db);
  approachLegStmt->prepare(util.buildInsertStatement("approach_leg", QString(),
                                                     {"fix_lonx", "fix_laty", "recommended_fix_fix_lonx", "recommended_fix_fix_laty"}));

  transitionLegStmt = new SqlQuery(db);
  transitionLegStmt->prepare(util.buildInsertStatement("transition_leg", QString(),
                                                       {"fix_airport_ident", "fix_lonx", "fix_laty", "recommended_fix_fix_lonx",
                                                        "recommended_fix_fix_laty"}));
}

void SimConnectWriter::clearAllBoundValues()
{
  airportStmt->clearBoundValues();
  airportFileStmt->clearBoundValues();
  runwayStmt->clearBoundValues();
  runwayEndStmt->clearBoundValues();
  startStmt->clearBoundValues();
  frequencyStmt->clearBoundValues();
  helipadStmt->clearBoundValues();
  taxiPathStmt->clearBoundValues();
  parkingStmt->clearBoundValues();
  approachStmt->clearBoundValues();
  transitionStmt->clearBoundValues();
  approachLegStmt->clearBoundValues();
  transitionLegStmt->clearBoundValues();
}

void SimConnectWriter::deInitQueries()
{
  ATOOLS_DELETE(airportStmt);
  ATOOLS_DELETE(airportFileStmt);
  ATOOLS_DELETE(runwayStmt);
  ATOOLS_DELETE(runwayEndStmt);
  ATOOLS_DELETE(startStmt);
  ATOOLS_DELETE(frequencyStmt);
  ATOOLS_DELETE(helipadStmt);
  ATOOLS_DELETE(taxiPathStmt);
  ATOOLS_DELETE(parkingStmt);
  ATOOLS_DELETE(approachStmt);
  ATOOLS_DELETE(transitionStmt);
  ATOOLS_DELETE(approachLegStmt);
  ATOOLS_DELETE(transitionLegStmt);
}

void SimConnectWriter::writeAirportsToDatabase(const QMap<unsigned long, atools::fs::sc::airport::Airport>& airports, int fileId)
{
  errors.clear();
  atools::sql::SqlTransaction transaction(db);

  if(!airports.isEmpty())
    qDebug() << Q_FUNC_INFO << "Batch size" << airports.size() << "from"
             << airports.first().getAirportFacility().icao << "..." << airports.last().getAirportFacility().icao;
  else
    qDebug() << Q_FUNC_INFO << "Batch is empty";

  for(auto it = airports.begin(); it != airports.end(); ++it)
  {
    const Airport& airport = *it;
    const AirportFacility& airportFacility = airport.getAirportFacility();
    QString icao = airportFacility.icao;

    try
    {
      // Write airport =========================================================================================
      Pos airportPos(airportFacility.longitude, airportFacility.latitude, meterToFeet(airportFacility.altitude));
      Rect bounding(airportPos);

      if(verbose)
        qDebug() << Q_FUNC_INFO << icao << airportFacility.longitude << airportFacility.latitude;

      airportStmt->bindValue(":airport_id", ++airportId);
      airportStmt->bindValue(":file_id", fileId);
      airportStmt->bindValue(":ident", icao);
      airportStmt->bindValue(":name", airportFacility.name);
      airportStmt->bindValue(":region", airportFacility.region);
      airportStmt->bindValue(":tower_frequency", airport.getTowerFrequency());
      airportStmt->bindValue(":atis_frequency", airport.getAtisFrequency());
      airportStmt->bindValue(":awos_frequency", airport.getAwosFrequency());
      airportStmt->bindValue(":asos_frequency", airport.getAsosFrequency());
      airportStmt->bindValue(":unicom_frequency", airport.getUnicomFrequency());
      airportStmt->bindValue(":is_closed", 0);
      airportStmt->bindValue(":is_military", util::isNameMilitary(airportFacility.name));
      airportStmt->bindValue(":is_addon", 0);
      airportStmt->bindValue(":num_com", airport.getFrequencyFacilities().size());
      airportStmt->bindValue(":num_parking_gate", airport.getNumParkingGate());
      airportStmt->bindValue(":num_parking_ga_ramp", airport.getNumParkingGaRamp());
      airportStmt->bindValue(":num_parking_cargo", airport.getNumParkingCargo());
      airportStmt->bindValue(":num_parking_mil_cargo", airport.getNumParkingMilCargo());
      airportStmt->bindValue(":num_parking_mil_combat", airport.getNumParkingMilCombat());
      airportStmt->bindValue(":num_runway_hard", airport.getNumRunwayHard());
      airportStmt->bindValue(":num_runway_soft", airport.getNumRunwaySoft());
      airportStmt->bindValue(":num_runway_water", airport.getNumRunwayWater());
      airportStmt->bindValue(":num_runway_end_closed", 0); // Not available
      airportStmt->bindValue(":num_runway_end_vasi", airport.getNumRunwayEndVasi());
      airportStmt->bindValue(":num_runway_end_als", airport.getNumRunwayEndAls());
      airportStmt->bindValue(":num_runway_end_ils", airport.getNumRunwayEndIls());
      airportStmt->bindValue(":num_apron", 0);
      airportStmt->bindValue(":num_taxi_path", airport.getTaxiPathFacilities().size());
      airportStmt->bindValue(":num_helipad", airport.getHelipadFacilities().size());
      airportStmt->bindValue(":num_jetway", airport.getJetwayFacilities().size());
      airportStmt->bindValue(":num_starts", airport.getStartFacilities().size());

      // Longest runway ==================================
      int longestIndex = airport.getLongestRunwayIndex();
      if(longestIndex >= 0)
      {
        const RunwayFacility& runway = airport.getRunways().at(longestIndex).getFacility();
        airportStmt->bindValue(":longest_runway_length", meterToFeet(runway.length));
        airportStmt->bindValue(":longest_runway_width", meterToFeet(runway.width));
        airportStmt->bindValue(":longest_runway_heading", runway.heading);
        airportStmt->bindValue(":longest_runway_surface", surfaceToDb(static_cast<Surface>(runway.surface)));
      }
      else
      {
        airportStmt->bindValue(":longest_runway_length", 0);
        airportStmt->bindValue(":longest_runway_width", 0);
        airportStmt->bindValue(":longest_runway_heading", 0.f);
        airportStmt->bindValue(":longest_runway_surface", 0);
      }

      airportStmt->bindValue(":num_runways", airport.getRunways().size());
      airportStmt->bindValue(":largest_parking_ramp", airport.getLargestParkingRamp());
      airportStmt->bindValue(":largest_parking_gate", airport.getLargestParkingGate());

      airportStmt->bindValue(":rating",
                             util::calculateAirportRating(false /* isAddon */, !airport.getTowerFrequency().isNull(), true /* msfs */,
                                                          airport.getTaxiPathFacilities().size(), airport.getTaxiParkingFacilities().size(),
                                                          0 /* numAprons */));

      airportStmt->bindValue(":is_3d", 0);
      airportStmt->bindValue(":mag_var", bgl::converter::adjustMagvar(airportFacility.magvar));

      Pos towerPos(airportFacility.towerLongitude, airportFacility.towerLatitude, meterToFeet(airportFacility.towerAltitude));
      bindPosAlt(airportStmt, towerPos, "tower_");
      bounding.extend(towerPos);

      airportStmt->bindValue(":transition_altitude", meterToFeet(airportFacility.transitionAltitude));
      airportStmt->bindValue(":transition_level", meterToFeet(airportFacility.transitionLevel));
      bindPosAlt(airportStmt, airportPos);

      // Runways ===========================================================================================
      // Index stores all runway end ids for this airport
      atools::fs::db::RunwayIndex runwayIndex;
      int numLightedRunways = 0;
      for(const Runway& runway : airport.getRunways())
      {
        // Check ranges and skip if invalid ====================
        // Primary threshold // Primary blastpad // Primary overrun
        // Secondary threshold // Secondary blastpad // Secondary overrun
        if(runway.getPavementFacilities().size() != 6)
          throw Exception(tr("Wrong pavements size %1 in airport %2").
                          arg(runway.getPavementFacilities().size()).arg(icao));

        // Primary approach lights // Secondary approach lights
        if(runway.getApproachLightFacilities().size() != 2)
          throw Exception(tr("Wrong approach lights size %1 in airport %2").
                          arg(runway.getApproachLightFacilities().size()).arg(icao));

        // Primary left vasi // Primary right vasi // Secondary left vasi // Secondary right vasi
        if(runway.getVasiFacilities().size() != 4)
          throw Exception(tr("Wrong VASI lights size %1 in airport %2").
                          arg(runway.getVasiFacilities().size()).arg(icao));

        // Runway ===========================================================================================
        const RunwayFacility& runwayFacility = runway.getFacility();
        const ApproachLightFacility& primaryApproachLights = runway.getApproachLightFacilities().at(0);
        const ApproachLightFacility& secondaryApproachLights = runway.getApproachLightFacilities().at(1);

        runwayStmt->bindValue(":runway_id", ++runwayId);
        runwayStmt->bindValue(":airport_id", airportId);

        int primaryEndId = ++runwayEndId;
        int secondaryEndId = ++runwayEndId;
        runwayStmt->bindValue(":primary_end_id", primaryEndId);
        runwayStmt->bindValue(":secondary_end_id", secondaryEndId);
        runwayStmt->bindValue(":surface", surfaceToDb(static_cast<Surface>(runwayFacility.surface)));
        runwayStmt->bindValue(":length", meterToFeet(runwayFacility.length));
        runwayStmt->bindValue(":width", meterToFeet(runwayFacility.width));
        runwayStmt->bindValue(":heading", runwayFacility.heading);
        runwayStmt->bindValue(":pattern_altitude", atools::roundToInt(meterToFeet(runwayFacility.patternAltitude)));
        runwayStmt->bindValue(":marking_flags", 0);

        // Look for lighted runway taxipaths which overlap with this primary or secondary runway number =========
        // Not reliable if taxiway is missing
        bool pathLights = false;
        for(const TaxiPathFacility& path : airport.getTaxiPathFacilities())
        {
          // Any edge or center light makes the runway lighted
          if(path.type == bgl::taxipath::RUNWAY &&
             ((runwayFacility.primaryNumber == path.runwayNumber && runwayFacility.primaryDesignator == path.runwayDesignator) ||
              (runwayFacility.secondaryNumber == path.runwayNumber && runwayFacility.secondaryDesignator == path.runwayDesignator)))
            pathLights |= path.leftEdgeLighted | path.rightEdgeLighted | path.centerLineLighted;
        }

        // Assume that the runway is lighted if it has ILS or touchdown or any other approach lights ================
        bool runwayLighted = pathLights |
                             primaryApproachLights.hasTouchdownLights | secondaryApproachLights.hasTouchdownLights |
                             primaryApproachLights.hasEndLights | secondaryApproachLights.hasEndLights |
                             primaryApproachLights.hasReilLights | secondaryApproachLights.hasReilLights |
                             (primaryApproachLights.system > 0) | (secondaryApproachLights.system > 0) |
                             (strlen(runwayFacility.primaryIlsIcao) > 0) | (strlen(runwayFacility.secondaryIlsIcao) > 0);

        numLightedRunways += runwayLighted;

        runwayStmt->bindValue(":edge_light",
                              bgl::util::enumToStr(bgl::Runway::lightToStr, runwayLighted ? bgl::rw::HIGH : bgl::rw::NO_LIGHT));
        runwayStmt->bindNullStr(":center_light");
        runwayStmt->bindValue(":has_center_red", 0);

        // Calculate runway end positions for drawing ======================
        Pos runwayPos(runwayFacility.longitude, runwayFacility.latitude, meterToFeet(runwayFacility.altitude));
        bounding.extend(runwayPos);

        Pos primaryPos = runwayPos.endpoint(runwayFacility.length / 2.f, atools::geo::opposedCourseDeg(runwayFacility.heading));
        primaryPos.setAltitude(runwayPos.getAltitude());
        bounding.extend(primaryPos);

        Pos secondaryPos = runwayPos.endpoint(runwayFacility.length / 2.f, runwayFacility.heading);
        secondaryPos.setAltitude(runwayPos.getAltitude());
        bounding.extend(secondaryPos);

        bindPos(runwayStmt, primaryPos, "primary_");
        bindPos(runwayStmt, secondaryPos, "secondary_");
        bindPosAlt(runwayStmt, runwayPos);
        runwayStmt->execAndClearBounds();

        // Primary runway end ===================================================
        runwayEndStmt->bindValue(":runway_end_id", primaryEndId);
        QString name = bgl::converter::runwayToStr(runwayFacility.primaryNumber, runwayFacility.primaryDesignator);
        runwayEndStmt->bindValue(":name", name);
        runwayEndStmt->bindValue(":end_type", "P");
        runwayEndStmt->bindValue(":offset_threshold", meterToFeet(runway.getPavementFacilities().at(0).length));
        runwayEndStmt->bindValue(":blast_pad", meterToFeet(runway.getPavementFacilities().at(1).length));
        runwayEndStmt->bindValue(":overrun", meterToFeet(runway.getPavementFacilities().at(2).length));
        bindVasi(runwayEndStmt, runway, true /* primary */);
        runwayEndStmt->bindValue(":has_closed_markings", 0);
        runwayEndStmt->bindValue(":has_stol_markings", 0);
        runwayEndStmt->bindValue(":is_takeoff", 1);
        runwayEndStmt->bindValue(":is_landing", 1);
        runwayEndStmt->bindValue(":is_pattern", "N");
        runwayEndStmt->bindValue(":app_light_system_type",
                                 enumToStr(bgl::RunwayApproachLights::appLightSystemToStr,
                                           static_cast<bgl::rw::ApproachLightSystem>(primaryApproachLights.system)));
        runwayEndStmt->bindValue(":has_end_lights", primaryApproachLights.hasEndLights);
        runwayEndStmt->bindValue(":has_reils", primaryApproachLights.hasReilLights);
        runwayEndStmt->bindValue(":has_touchdown_lights", primaryApproachLights.hasTouchdownLights);

        runwayEndStmt->bindValue(":ils_ident", runwayFacility.primaryIlsIcao);
        runwayEndStmt->bindValue(":heading", runwayFacility.heading);
        bindPosAlt(runwayEndStmt, primaryPos);
        runwayEndStmt->execAndClearBounds();
        runwayIndex.add(icao, name, primaryEndId);

        // Secondary runway end ===================================================
        runwayEndStmt->bindValue(":runway_end_id", secondaryEndId);
        name = bgl::converter::runwayToStr(runwayFacility.secondaryNumber, runwayFacility.secondaryDesignator);
        runwayEndStmt->bindValue(":name", name);
        runwayEndStmt->bindValue(":end_type", "S");
        runwayEndStmt->bindValue(":offset_threshold", meterToFeet(runway.getPavementFacilities().at(3).length));
        runwayEndStmt->bindValue(":blast_pad", meterToFeet(runway.getPavementFacilities().at(4).length));
        runwayEndStmt->bindValue(":overrun", meterToFeet(runway.getPavementFacilities().at(5).length));
        bindVasi(runwayEndStmt, runway, false /* primary */);
        runwayEndStmt->bindValue(":has_closed_markings", 0);
        runwayEndStmt->bindValue(":has_stol_markings", 0);
        runwayEndStmt->bindValue(":is_takeoff", 1);
        runwayEndStmt->bindValue(":is_landing", 1);
        runwayEndStmt->bindValue(":is_pattern", "N");
        runwayEndStmt->bindValue(":app_light_system_type",
                                 enumToStr(bgl::RunwayApproachLights::appLightSystemToStr,
                                           static_cast<bgl::rw::ApproachLightSystem>(secondaryApproachLights.system)));
        runwayEndStmt->bindValue(":has_end_lights", secondaryApproachLights.hasEndLights);
        runwayEndStmt->bindValue(":has_reils", secondaryApproachLights.hasReilLights);
        runwayEndStmt->bindValue(":has_touchdown_lights", secondaryApproachLights.hasTouchdownLights);
        runwayEndStmt->bindValue(":ils_ident", runwayFacility.secondaryIlsIcao);
        runwayEndStmt->bindValue(":heading", atools::geo::opposedCourseDeg(runwayFacility.heading));
        bindPosAlt(runwayEndStmt, secondaryPos);
        runwayEndStmt->execAndClearBounds();
        runwayIndex.add(icao, name, secondaryEndId);
      } // for(const Runway& runway : airport.getRunways())

      // Starts ===========================================================================================
      QVector<std::pair<Pos, int> > startIndex;
      for(const StartFacility& start : airport.getStartFacilities())
      {
        Pos startPos(start.longitude, start.latitude, meterToFeet(start.altitude));

        startStmt->bindValue(":start_id", ++startId);
        startStmt->bindValue(":airport_id", airportId);

        // Look for runway ends in index if start is of type runway =======================
        bgl::start::StartType startType = static_cast<bgl::start::StartType>(start.type);
        if(startType == bgl::start::RUNWAY)
        {
          QString name = bgl::converter::runwayToStr(start.number, start.designator);
          int endId = runwayIndex.getRunwayEndId(icao, name, "SimConnect - start " % startPos.toString());

          if(endId > 0)
            startStmt->bindValue(":runway_end_id", endId);
          else
            startStmt->bindNullStr(":runway_end_id");

          startStmt->bindValue(":runway_name", name);
        }
        else
          startStmt->bindNullStr(":runway_name");

        startStmt->bindValue(":type", enumToStr(bgl::Start::startTypeToStr, startType));
        startStmt->bindValue(":heading", start.heading);
        startStmt->bindValue(":number", start.number);
        bindPosAlt(startStmt, startPos);
        startStmt->execAndClearBounds();
        bounding.extend(startPos);
        startIndex.append(std::make_pair(startPos, startId));
      }

      // Frequencies / COM ===========================================================================================
      for(const FrequencyFacility& frequency : airport.getFrequencyFacilities())
      {
        frequencyStmt->bindValue(":com_id", ++frequencyId);
        frequencyStmt->bindValue(":airport_id", airportId);
        frequencyStmt->bindValue(":type", enumToStr(bgl::Com::comTypeToStr, static_cast<bgl::com::ComType>(frequency.type)));
        frequencyStmt->bindValue(":frequency", frequency.frequency);
        frequencyStmt->bindValue(":name", frequency.name);
        frequencyStmt->execAndClearBounds();
      }

      // Helipads ===========================================================================================
      for(const HelipadFacility& helipad : airport.getHelipadFacilities())
      {
        // Look for a start position by coordinates for the helipad =========
        Pos helipadPos(helipad.longitude, helipad.latitude, meterToFeet(helipad.altitude));
        int helipadStartId = -1;
        for(const std::pair<Pos, int>& start : startIndex)
        {
          if(start.first.almostEqual(helipadPos, atools::geo::Pos::POS_EPSILON_5M))
          {
            helipadStartId = start.second;
            break;
          }
        }

        helipadStmt->bindValue(":helipad_id", ++helipadId);
        helipadStmt->bindValue(":airport_id", airportId);

        if(helipadStartId > 0)
          helipadStmt->bindValue(":start_id", helipadStartId);
        else
          helipadStmt->bindNullInt(":start_id");

        helipadStmt->bindValue(":surface", surfaceToDb(static_cast<Surface>(helipad.surface)));
        helipadStmt->bindValue(":type", enumToStr(bgl::Helipad::helipadTypeToStr, static_cast<bgl::helipad::HelipadType>(helipad.type)));

        helipadStmt->bindValue(":length", meterToFeet(helipad.length));
        helipadStmt->bindValue(":width", meterToFeet(helipad.width));
        helipadStmt->bindValue(":heading", helipad.heading);
        helipadStmt->bindValue(":is_transparent", 0);
        helipadStmt->bindValue(":is_closed", 0);
        bindPosAlt(helipadStmt, helipadPos);
        helipadStmt->execAndClearBounds();
        bounding.extend(helipadPos);
      }

      // Approaches ===========================================================================================
      int numProcedures = 0;
      for(const Approach& approach : airport.getApproaches())
      {
        numProcedures++;
        const ApproachFacility& approachFacility = approach.getApproachFacility();
        approachStmt->bindValue(":approach_id", ++approachId);
        approachStmt->bindValue(":airport_id", airportId);

        // Look for runway ends in index  =======================
        QString name = bgl::converter::runwayToStr(approachFacility.runwayNumber, approachFacility.runwayDesignator);
        int endId = runwayIndex.getRunwayEndId(icao, name,
                                               "SimConnect - approach " % QString(approach.getApproachFacility().fafIcao));

        if(endId > 0)
          approachStmt->bindValue(":runway_end_id", endId);
        else
          approachStmt->bindNullInt(":runway_end_id");

        // Build ARINC name
        approachStmt->bindValue(":arinc_name",
                                bgl::ap::arincNameAppr(static_cast<bgl::ap::ApproachType>(approachFacility.type), name,
                                                       static_cast<char>(approachFacility.suffix), false));

        approachStmt->bindValue(":airport_ident", icao);
        approachStmt->bindValue(":runway_name", name);
        approachStmt->bindValue(":type", enumToStr(bgl::ap::approachTypeToStr, static_cast<bgl::ap::ApproachType>(approachFacility.type)));

        if(approachFacility.suffix > '0')
          approachStmt->bindValue(":suffix", QChar(approachFacility.suffix));
        else
          approachStmt->bindNullStr(":suffix");

        approachStmt->bindValue(":has_gps_overlay", 0);
        approachStmt->bindValue(":has_rnp", approachFacility.rnpAr);
        approachStmt->bindValue(":fix_type", QChar(approachFacility.fafType));
        approachStmt->bindValue(":fix_ident", approachFacility.fafIcao);
        approachStmt->bindValue(":fix_region", approachFacility.fafRegion);
        approachStmt->bindNullStr(":fix_airport_ident");
        approachStmt->bindNullStr(":aircraft_category");
        approachStmt->bindValue(":altitude", meterToFeet(approachFacility.fafAltitude));
        approachStmt->bindNullFloat(":heading");
        approachStmt->bindValue(":missed_altitude", meterToFeet(approachFacility.missedAltitude));

        // Transitions ==============================
        for(const ApproachTransition& transition : approach.getApproachTransitions())
        {
          const ApproachTransitionFacility& transitionFacility = transition.getApproachTransitionFacility();

          transitionStmt->bindValue(":transition_id", ++transitionId);
          transitionStmt->bindValue(":type", enumToStr(bgl::Transition::transitionTypeToStr,
                                                       static_cast<bgl::ap::TransitionType>(transitionFacility.type)));
          transitionStmt->bindValue(":approach_id", approachId);
          transitionStmt->bindValue(":fix_type", QChar(transitionFacility.iafType));
          transitionStmt->bindValue(":fix_ident", transitionFacility.iafIcao);
          transitionStmt->bindValue(":fix_region", transitionFacility.iafRegion);
          transitionStmt->bindNullStr(":fix_airport_ident");
          transitionStmt->bindNullStr(":aircraft_category");
          transitionStmt->bindValue(":altitude", meterToFeet(transitionFacility.iafAltitude));

          transitionStmt->bindValue(":dme_ident", transitionFacility.dmeArcIcao);
          transitionStmt->bindValue(":dme_region", transitionFacility.dmeArcRegion);
          transitionStmt->bindNullStr(":dme_airport_ident");
          transitionStmt->bindValue(":dme_radial", transitionFacility.dmeArcRadial);
          transitionStmt->bindValue(":dme_distance", meterToNm(approachFacility.fafAltitude));

          // Transition legs ==============================
          for(const LegFacility& leg : transition.getApproachTransitionLegFacilities())
            writeLeg(transitionLegStmt, leg, TRANS);

          transitionStmt->execAndClearBounds();
        }
        // Approach legs =========
        bool verticalAngleFound = false; // Check for vertical path angle in legs
        for(const LegFacility& leg : approach.getFinalApproachLegFacilities())
          writeLeg(approachLegStmt, leg, APPROACH, &verticalAngleFound);

        // Missed approach legs =========
        for(const LegFacility& leg : approach.getMissedApproachLegFacilities())
          writeLeg(approachLegStmt, leg, MISSED);

        approachStmt->bindValue(":has_vertical_angle", verticalAngleFound);
        approachStmt->execAndClearBounds();
      }

      // STAR / Arrivals ===========================================================================================
      // Legs are saved in flying order from transition entry to STAR exit
      for(const Arrival& arrival : airport.getArrivals())
      {
        for(const RunwayTransition& runwayTransition : arrival.getRunwayTransitions())
        {
          const RunwayTransitionFacility& transitionFacility = runwayTransition.getTransitionFacility();
          approachStmt->bindValue(":approach_id", ++approachId);
          approachStmt->bindValue(":airport_id", airportId);
          bindRunway(approachStmt, runwayIndex, icao, transitionFacility.runwayNumber, transitionFacility.runwayDesignator,
                     "SimConnect - arrival " % QString(arrival.getArrivalFacility().name));
          approachStmt->bindValue(":airport_ident", icao);
          approachStmt->bindValue(":type", "GPS");
          approachStmt->bindValue(":suffix", "A");
          approachStmt->bindValue(":has_gps_overlay", 1);
          approachStmt->bindValue(":has_rnp", 0);
          approachStmt->bindValue(":fix_type", QChar(runwayTransition.getLegFacilities().constFirst().fixType));
          approachStmt->bindValue(":fix_ident", arrival.getArrivalFacility().name);
          approachStmt->bindValue(":fix_region", runwayTransition.getLegFacilities().constFirst().fixRegion);
          approachStmt->bindNullStr(":fix_airport_ident");
          approachStmt->bindNullStr(":aircraft_category");
          approachStmt->bindNullFloat(":altitude");
          approachStmt->bindNullFloat(":heading");
          approachStmt->bindNullFloat(":missed_altitude");
          approachStmt->execAndClearBounds();

          // Common route if available ===================================
          for(const LegFacility& leg : arrival.getApproachLegFacilities())
            writeLeg(approachLegStmt, leg, STAR);

          // Runway transition legs - always present  ===================================
          for(const LegFacility& leg : runwayTransition.getLegFacilities())
          {
            // Skip overlapping IF legs between common and runway transition
            if(!arrival.getApproachLegFacilities().isEmpty())
            {
              const LegFacility& lastCommon = arrival.getApproachLegFacilities().constLast();
              if(strcmp(lastCommon.fixIcao, leg.fixIcao) == 0 && strcmp(lastCommon.fixRegion, leg.fixRegion) == 0)
                continue;
            }

            // Write transtion leg
            writeLeg(approachLegStmt, leg, STAR);
          }

          // Enroute transition is saved separately as transition ===================================
          for(const EnrouteTransition& enrouteTransition : arrival.getEnrouteTransitions())
          {
            transitionStmt->bindValue(":transition_id", ++transitionId);
            transitionStmt->bindValue(":approach_id", approachId);
            transitionStmt->bindValue(":type", enumToStr(bgl::Transition::transitionTypeToStr, bgl::ap::FULL));
            transitionStmt->bindValue(":fix_type", QChar(enrouteTransition.getLegFacilities().constFirst().fixType));
            transitionStmt->bindValue(":fix_ident", enrouteTransition.getTransitionFacility().name);
            transitionStmt->bindValue(":fix_region", enrouteTransition.getLegFacilities().constFirst().fixRegion);
            transitionStmt->bindNullStr(":fix_airport_ident");
            transitionStmt->bindNullStr(":aircraft_category");
            transitionStmt->bindNullFloat(":altitude");
            transitionStmt->bindNullStr(":dme_ident");
            transitionStmt->bindNullStr(":dme_region");
            transitionStmt->bindNullStr(":dme_airport_ident");
            transitionStmt->bindNullFloat(":dme_radial");
            transitionStmt->bindNullFloat(":dme_distance");
            transitionStmt->execAndClearBounds();

            for(const LegFacility& leg : enrouteTransition.getLegFacilities())
              writeLeg(transitionLegStmt, leg, STARTRANS);
          }
          numProcedures++;
        }
      } // for(const Arrival& arrival : airport.getArrivals())

      // SID / Departures ===========================================================================================
      // Legs are saved in flying order from STAR entry/runway to transition exit
      for(const Departure& departure : airport.getDepartures())
      {
        for(const RunwayTransition& runwayTransition : departure.getRunwayTransitions())
        {
          const RunwayTransitionFacility& transitionFacility = runwayTransition.getTransitionFacility();
          approachStmt->bindValue(":approach_id", ++approachId);
          approachStmt->bindValue(":airport_id", airportId);
          bindRunway(approachStmt, runwayIndex, icao, transitionFacility.runwayNumber, transitionFacility.runwayDesignator,
                     "SimConnect - departure " % QString(departure.getDepartureFacility().name));
          approachStmt->bindValue(":airport_ident", icao);
          approachStmt->bindValue(":type", "GPS");
          approachStmt->bindValue(":suffix", "D");
          approachStmt->bindValue(":has_gps_overlay", 1);
          approachStmt->bindValue(":has_rnp", 0);
          approachStmt->bindValue(":fix_type", QChar(runwayTransition.getLegFacilities().constLast().fixType));
          approachStmt->bindValue(":fix_ident", departure.getDepartureFacility().name);
          approachStmt->bindValue(":fix_region", runwayTransition.getLegFacilities().constLast().fixRegion);
          approachStmt->bindNullStr(":fix_airport_ident");
          approachStmt->bindNullStr(":aircraft_category");
          approachStmt->bindNullFloat(":altitude");
          approachStmt->bindNullFloat(":heading");
          approachStmt->bindNullFloat(":missed_altitude");
          approachStmt->execAndClearBounds();

          // Common route if available ===================================
          for(const LegFacility& leg : runwayTransition.getLegFacilities())
            writeLeg(approachLegStmt, leg, SID);

          // Runway transition legs - always present  ===================================
          for(const LegFacility& leg : departure.getApproachLegFacilities())
          {
            // Skip overlapping IF legs between common and runway transition
            if(!runwayTransition.getLegFacilities().isEmpty())
            {
              const LegFacility& firstCommon = departure.getApproachLegFacilities().constFirst();
              if(strcmp(firstCommon.fixIcao, leg.fixIcao) == 0 && strcmp(firstCommon.fixRegion, leg.fixRegion) == 0)
                continue;
            }

            // Write transtion leg
            writeLeg(approachLegStmt, leg, SID);
          }

          // Enroute transition is saved separately as transition ===================================
          for(const EnrouteTransition& enrouteTransition : departure.getEnrouteTransitions())
          {
            transitionStmt->bindValue(":transition_id", ++transitionId);
            transitionStmt->bindValue(":approach_id", approachId);
            transitionStmt->bindValue(":type", enumToStr(bgl::Transition::transitionTypeToStr, bgl::ap::FULL));
            transitionStmt->bindValue(":fix_type", QChar(enrouteTransition.getLegFacilities().constLast().fixType));
            transitionStmt->bindValue(":fix_ident", enrouteTransition.getTransitionFacility().name);
            transitionStmt->bindValue(":fix_region", enrouteTransition.getLegFacilities().constLast().fixRegion);
            transitionStmt->bindNullStr(":fix_airport_ident");
            transitionStmt->bindNullStr(":aircraft_category");
            transitionStmt->bindNullFloat(":altitude");
            transitionStmt->bindNullStr(":dme_ident");
            transitionStmt->bindNullStr(":dme_region");
            transitionStmt->bindNullStr(":dme_airport_ident");
            transitionStmt->bindNullFloat(":dme_radial");
            transitionStmt->bindNullFloat(":dme_distance");
            transitionStmt->execAndClearBounds();

            for(const LegFacility& leg : enrouteTransition.getLegFacilities())
              writeLeg(transitionLegStmt, leg, SIDTRANS);
          }
          numProcedures++;
        }
      } // for(const Departure& departure : airport.getDepartures())

      // Jetways ===========================================================================================
      // Save in index only
      QSet<std::pair<qint32, qint32> > jetwayIndexes;
      for(const JetwayFacility& jetway : airport.getJetwayFacilities())
        jetwayIndexes.insert(std::make_pair(jetway.parkingGate, jetway.parkingSuffix));

      // Parking ===========================================================================================
      // Maps index to parking facility - allows to omit vehicle parking spots
      QHash<int, const TaxiParkingFacility *> parkingIndexes;
      bool airportHasFuel = false;
      int index = 0;
      for(const TaxiParkingFacility& parking : airport.getTaxiParkingFacilities())
      {
        bgl::ap::ParkingType parkingType = static_cast<bgl::ap::ParkingType>(parking.type);
        QString parkingTypeStr = enumToStr(bgl::Parking::parkingTypeToStr, parkingType);

        // Add only valid and omit vehicle parking
        if(parkingType != bgl::ap::VEHICLES && !parkingTypeStr.isEmpty())
        {
          // Detect fuel availability for airport
          if(parking.type == bgl::ap::FUEL)
            airportHasFuel = true;

          parkingStmt->bindValue(":parking_id", ++parkingId);
          parkingStmt->bindValue(":airport_id", airportId);
          parkingStmt->bindValue(":type", parkingTypeStr);
          parkingStmt->bindValue(":name", enumToStr(bgl::Parking::parkingNameToStr,
                                                    static_cast<bgl::ap::ParkingName>(parking.name)));
          parkingStmt->bindValue(":number", parking.number);
          parkingStmt->bindValue(":suffix", enumToStr(bgl::Parking::parkingSuffixToStr,
                                                      static_cast<bgl::ap::ParkingNameSuffix>(parking.suffix)));
          parkingStmt->bindValue(":radius", meterToFeet(parking.radius));
          parkingStmt->bindValue(":heading", parking.heading);
          parkingStmt->bindValue(":has_jetway", jetwayIndexes.contains(std::make_pair(parking.name, parking.suffix)));

          Pos parkingPos(calcPosFromBias(airportPos, parking.biasX, parking.biasY));
          bindPos(parkingStmt, parkingPos);
          parkingStmt->execAndClearBounds();
          bounding.extend(parkingPos);
        }
        parkingIndexes.insert(index, &parking);

        index++;
      }

      // Taxi path ===========================================================================================
      for(const TaxiPathFacility& path : airport.getTaxiPathFacilities())
      {
        if(path.type == bgl::taxipath::PATH || path.type == bgl::taxipath::TAXI || path.type == bgl::taxipath::PARKING)
        {
          if(!atools::inRange(airport.getTaxiPointFacilities(), path.start))
            throw Exception(tr("Start path index %1 out of range in airport %2").arg(path.start).arg(icao));

          if(path.type == bgl::taxipath::PARKING)
          {
            if(!parkingIndexes.contains(path.end))
              throw Exception(tr("End parking path index %1 out of range in airport %2").arg(path.start).arg(icao));
          }
          else if(!atools::inRange(airport.getTaxiPointFacilities(), path.end))
            throw Exception(tr("End path index %1 out of range in airport %2").arg(path.start).arg(icao));

          taxiPathStmt->bindValue(":taxi_path_id", ++taxiPathId);
          taxiPathStmt->bindValue(":airport_id", airportId);
          taxiPathStmt->bindValue(":type", enumToStr(bgl::TaxiPath::pathTypeToString, static_cast<bgl::taxipath::Type>(path.type)));
          taxiPathStmt->bindValue(":width", meterToFeet(path.width));
          taxiPathStmt->bindValue(":surface", surfaceToDb(TAXIPATH));

          // Bind name if index is valid
          if(atools::inRange(airport.getTaxiNameFacilities(), static_cast<int>(path.nameIndex)))
            taxiPathStmt->bindValue(":name", airport.getTaxiNameFacilities().at(static_cast<int>(path.nameIndex)).name);
          else
            qWarning() << Q_FUNC_INFO << icao << "path.name out of range. Is" << path.nameIndex << path.type
                       << "from 0 to" << airport.getTaxiNameFacilities().size();

          taxiPathStmt->bindValue(":is_draw_surface", 1);
          taxiPathStmt->bindValue(":is_draw_detail", 1);

          // Start is always a node ===============================================================
          const TaxiPointFacility& startPoint = airport.getTaxiPointFacilities().at(path.start);
          taxiPathStmt->bindValue(":start_type", enumToStr(bgl::TaxiPoint::pointTypeToString,
                                                           static_cast<bgl::taxipoint::PointType>(startPoint.type)));
          taxiPathStmt->bindValue(":start_dir", enumToStr(bgl::TaxiPoint::dirToString,
                                                          static_cast<bgl::taxipoint::PointDir>(startPoint.orientation)));

          Pos startPos(calcPosFromBias(airportPos, startPoint.biasX, startPoint.biasY));
          bindPos(taxiPathStmt, startPos, "start_");

          // End is either node or parking depending on type ======================================================
          Pos endPos;
          QString endType, endDir;
          if(path.type == bgl::taxipath::PARKING)
          {
            // End is parking spot ===============================
            const TaxiParkingFacility *endParking = parkingIndexes.value(path.end, nullptr);
            if(endParking != nullptr && static_cast<bgl::ap::ParkingType>(endParking->type) != bgl::ap::VEHICLES)
            {
              endPos = calcPosFromBias(airportPos, endParking->biasX, endParking->biasY);
              endType = enumToStr(bgl::TaxiPoint::pointTypeToString, bgl::taxipoint::PARKING);
              endDir = enumToStr(bgl::TaxiPoint::dirToString, bgl::taxipoint::UNKNOWN_DIR);
            }
          }
          else
          {
            // End is normal point ===============================
            const TaxiPointFacility *endPoint = &airport.getTaxiPointFacilities().at(path.end);
            if(endPoint != nullptr)
            {
              endPos = calcPosFromBias(airportPos, endPoint->biasX, endPoint->biasY);
              endType = enumToStr(bgl::TaxiPoint::pointTypeToString, static_cast<bgl::taxipoint::PointType>(endPoint->type));
              endDir = enumToStr(bgl::TaxiPoint::dirToString, static_cast<bgl::taxipoint::PointDir>(endPoint->orientation));
            }
          }

          taxiPathStmt->bindValue(":end_type", endType);
          taxiPathStmt->bindValue(":end_dir", endDir);
          bindPos(taxiPathStmt, endPos, "end_");
          taxiPathStmt->execAndClearBounds();

          bounding.extend(startPos);
          bounding.extend(endPos);
        }
      }

      // Finalize airport =================================
      airportStmt->bindValue(":fuel_flags", airportHasFuel ? bgl::ap::MSFS_DEFAULT_FUEL : bgl::ap::NO_FUEL_FLAGS);
      airportStmt->bindValue(":has_avgas", airportHasFuel);
      airportStmt->bindValue(":has_jetfuel", airportHasFuel);
      airportStmt->bindValue(":num_runway_light", numLightedRunways);
      airportStmt->bindValue(":num_approach", numProcedures);
      airportStmt->bindValue(":left_lonx", bounding.getTopLeft().getLonX());
      airportStmt->bindValue(":top_laty", bounding.getTopLeft().getLatY());
      airportStmt->bindValue(":right_lonx", bounding.getBottomRight().getLonX());
      airportStmt->bindValue(":bottom_laty", bounding.getBottomRight().getLatY());
      airportStmt->execAndClearBounds();

      airportFileStmt->bindValue(":airport_file_id", ++airportFileId);
      airportFileStmt->bindValue(":file_id", fileId);
      airportFileStmt->bindValue(":ident", icao);
      airportFileStmt->execAndClearBounds();

    } // try
    catch(Exception& e)
    {
      errors.append(tr("Caught exception writing airport %1. Error: %2").arg(icao).arg(e.what()));
      qWarning() << Q_FUNC_INFO << errors.constLast();

      clearAllBoundValues();
    }
    catch(...)
    {
      errors.append(tr("Caught unknown exception writing airport %1.").arg(icao));
      qWarning() << Q_FUNC_INFO << errors.constLast();

      clearAllBoundValues();
    }

    if(errors.size() > MAX_ERRORS)
    {
      errors.append(tr("Too many errors writing airport data. Stopping."));
      break;
    }
  } // for(auto it = airports.begin(); it != airports.end(); ++it)

  transaction.commit();
}

void SimConnectWriter::bindRunway(atools::sql::SqlQuery *query, const atools::fs::db::RunwayIndex& runwayIndex, const QString& airportIcao,
                                  int runwayNumber, int runwayDesignator, const QString& sourceObject) const
{
  // Get runway name and end id from index
  QString rwName = bgl::converter::runwayToStr(runwayNumber, runwayDesignator);
  int endId = runwayIndex.getRunwayEndId(airportIcao, rwName, sourceObject);

  if(endId > 0)
    query->bindValue(":runway_end_id", endId);
  else
    query->bindNullInt(":runway_end_id");

  if(!rwName.isEmpty())
    query->bindValue(":arinc_name", QString("RW") + rwName);
  else
    query->bindNullInt(":arinc_name");

  query->bindValue(":runway_name", rwName);
}

void SimConnectWriter::bindVasi(atools::sql::SqlQuery *query, const Runway& runway, bool primary) const
{
  // VASI facility indexes
  enum {VASI_PRIMARY_LEFT, VASI_PRIMARY_RIGHT, VASI_SECONDARY_LEFT, VASI_SECONDARY_RIGHT};

  // Left VASI ======================================================
  int index = primary ? VASI_PRIMARY_LEFT : VASI_SECONDARY_LEFT;
  bgl::rw::VasiType type = static_cast<bgl::rw::VasiType>(runway.getVasiFacilities().at(index).type);
  if(type != bgl::rw::NONE)
  {
    query->bindValue(":left_vasi_type", enumToStr(bgl::RunwayVasi::vasiTypeToStr, type));
    query->bindValue(":left_vasi_pitch", runway.getVasiFacilities().at(index).angle);
  }
  else
  {
    query->bindNullStr(":left_vasi_type");
    query->bindNullFloat(":left_vasi_pitch");
  }

  // Right VASI ======================================================
  index = primary ? VASI_PRIMARY_RIGHT : VASI_SECONDARY_RIGHT;
  type = static_cast<bgl::rw::VasiType>(runway.getVasiFacilities().at(index).type);
  if(type != bgl::rw::NONE)
  {
    query->bindValue(":right_vasi_type", enumToStr(bgl::RunwayVasi::vasiTypeToStr, type));
    query->bindValue(":right_vasi_pitch", runway.getVasiFacilities().at(index).angle);
  }
  else
  {
    query->bindNullStr(":right_vasi_type");
    query->bindNullFloat(":right_vasi_pitch");
  }
}

void SimConnectWriter::writeLeg(atools::sql::SqlQuery *query, const LegFacility& leg, LegType type, bool *verticalAngleFound)
{
  // Bind ids ======================
  switch(type)
  {
    case APPROACH:
    case MISSED:
    case SID:
    case STAR:
      query->bindValue(":approach_leg_id", ++approachLegId);
      query->bindValue(":approach_id", approachId);
      query->bindValue(":is_missed", type == MISSED);
      break;

    case TRANS:
    case SIDTRANS:
    case STARTRANS:
      query->bindValue(":transition_leg_id", ++transitionLegId);
      query->bindValue(":transition_id", transitionId);
      break;
  }

  query->bindValue(":type", enumToStr(bgl::ApproachLeg::legTypeToString, static_cast<bgl::leg::Type>(leg.type)));

  // ARINC 424 5.17
  // Initial Approach Fix "A"
  // Intermediate Approach Fix "B"
  // Initial Approach Fix with Holding "C"
  // Initial Approach Fix with Final Approach Course Fix "D"
  // Final End Point Fix "E"
  // Published Final Approach Fix or Database Final Approach Fix "F"
  // Holding Fix Enroute SID, STAR, "H"
  // Final Approach Course Fix "I"
  // Published Missed Approach Point Fix "M"
  QString apprFixType;
  if(leg.isIaf)
    apprFixType = "A"; // Initial Approach Fix
  else if(leg.isIf)
    apprFixType = "B"; // Intermediate Approach Fix
  else if(leg.isFaf)
    apprFixType = "F"; // Final Approach Fix
  else if(leg.isMap)
    apprFixType = "M";
  query->bindValue(":arinc_descr_code", QString("   %1").arg(apprFixType));
  query->bindValue(":approach_fix_type", apprFixType);

  query->bindValue(":alt_descriptor", enumToStr(bgl::ApproachLeg::altDescriptorToString,
                                                static_cast<bgl::leg::AltDescriptor>(leg.approachAltDesc)));
  query->bindValue(":turn_direction", enumToStr(bgl::ApproachLeg::turnDirToString,
                                                static_cast<bgl::leg::TurnDirection>(leg.turnDirection)));
  query->bindValue(":rnp", meterToNm(leg.requiredNavigationPerformance));
  query->bindValue(":fix_type", QChar(leg.fixType));
  query->bindValue(":fix_ident", leg.fixIcao);
  query->bindValue(":fix_region", leg.fixRegion);

  // Omit coordinates since they seem to be not accurate - let Little Navmap resolve the fixes by name
  // bindPos(query, leg.fixLongitude, leg.fixLatitude, "fix_");

  if(strlen(leg.arcCenterFixIcao) > 0)
  {
    query->bindValue(":recommended_fix_type", QChar(leg.arcCenterFixType));
    query->bindValue(":recommended_fix_ident", leg.arcCenterFixIcao);
    query->bindValue(":recommended_fix_region", leg.arcCenterFixRegion);
    // bindPos(query, leg.arcCenterFixLongitude, leg.arcCenterFixLatitude, "recommended_fix_");
  }
  else
  {
    query->bindValue(":recommended_fix_type", QChar(leg.originType));
    query->bindValue(":recommended_fix_ident", leg.originIcao);
    query->bindValue(":recommended_fix_region", leg.originRegion);
    // bindPos(query, leg.originLongitude, leg.originLatitude, "recommended_fix_");
  }
  query->bindValue(":is_flyover", leg.flyOver);
  query->bindValue(":is_true_course", leg.trueDegree);
  query->bindValue(":course", leg.course);

  if(leg.distanceMinute)
  {
    query->bindNullFloat(":distance");
    query->bindValue(":time", leg.routeDistance);
  }
  else
  {
    query->bindValue(":distance", meterToNm(leg.routeDistance));
    query->bindNullFloat(":time");
  }

  query->bindValue(":theta", leg.theta);
  query->bindValue(":rho", meterToNm(leg.rho));

  query->bindValue(":altitude1", meterToFeet(leg.altitude1));
  query->bindValue(":altitude2", meterToFeet(leg.altitude2));

  query->bindValue(":speed_limit_type", enumToStr(bgl::ApproachLeg::speedDescriptorToString, bgl::leg::AT_OR_BELOW));

  if(leg.speedLimit > 0.f)
    query->bindValue(":speed_limit", leg.speedLimit);
  else
    query->bindNullFloat(":speed_limit");

  if(leg.verticalAngle > 0)
  {
    query->bindValue(":vertical_angle", 360.f - leg.verticalAngle);
    if(verticalAngleFound != nullptr)
      *verticalAngleFound |= true;
  }
  else
    query->bindNullFloat(":vertical_angle");
  query->execAndClearBounds();
}

} // namespace airport
} // namespace sc
} // namespace fs
} // namespace atools
