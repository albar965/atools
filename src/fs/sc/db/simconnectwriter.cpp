/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

#include "fs/sc/db/simconnectwriter.h"

#include "atools.h"
#include "exception.h"
#include "fs/bgl/ap/airport.h"
#include "fs/bgl/converter.h"
#include "fs/bgl/nav/ndb.h"
#include "fs/bgl/util.h"
#include "fs/common/magdecreader.h"
#include "fs/db/runwayindex.h"
#include "fs/sc/db/simconnectairport.h"
#include "fs/sc/db/simconnectid.h"
#include "fs/sc/db/simconnectnav.h"
#include "fs/util/fsutil.h"
#include "fs/util/tacanfrequencies.h"
#include "geo/calculations.h"
#include "geo/rect.h"
#include "sql/sqlquery.h"
#include "sql/sqltransaction.h"
#include "sql/sqlutil.h"

#include <QString>
#include <QStringBuilder>
#include <QCoreApplication>
#include <QThread>

namespace atools {
namespace fs {
namespace sc {
namespace db {

using atools::sql::SqlQuery;
using atools::sql::SqlUtil;
using atools::geo::Rect;
using atools::geo::Pos;
using atools::fs::bgl::util::enumToStr;

// Do not extend bounding rectangle beyond this distance from center point
const static float MAX_DISTANCE_TO_BOUNDING_NM = 20.f;

const static int UPDATE_RATE_MS = 2000;

float mToNm(float meter)
{
  return atools::geo::meterToNm(meter);
}

float mToFt(float meter, int precision = 0)
{
  return atools::roundToPrecision(atools::geo::meterToFeet(meter), precision);
}

// Binding inline helper functions ===================================================
inline bool valid(const geo::Pos& pos)
{
  return pos.isValid() && !pos.isNull();
}

/* Binds position to query if valid or null if not */
inline void bindPos(atools::sql::SqlQuery *query, const geo::Pos& pos, const QString& prefix = QString())
{
  if(valid(pos))
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

/* Binds position and altitude in meter to query  */
inline void bindPosAlt(atools::sql::SqlQuery *query, const atools::geo::Pos& pos, const QString& prefix = QString())
{
  bindPos(query, pos, prefix);

  if(valid(pos))
    query->bindValue(":" % prefix % "altitude", atools::geo::meterToFeet(pos.getAltitude()));
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

inline void bindPosAlt(atools::sql::SqlQuery *query, float lonX, float latY, float altitude, const QString& prefix = QString())
{
  bindPosAlt(query, Pos(lonX, latY, altitude), prefix);
}

inline void bindPosAlt(atools::sql::SqlQuery *query, double lonX, double latY, double altitude, const QString& prefix = QString())
{
  bindPosAlt(query, Pos(lonX, latY, altitude), prefix);
}

inline void bindPosNull(atools::sql::SqlQuery *query, const QString& prefix = QString())
{
  query->bindNullFloat(":" % prefix % "lonx");
  query->bindNullFloat(":" % prefix % "laty");
}

inline void bindPosAltNull(atools::sql::SqlQuery *query, const QString& prefix = QString())
{
  bindPosNull(query, prefix);
  query->bindNullFloat(":" % prefix % "altitude");
}

inline atools::geo::Pos calcPosFromBias(const atools::geo::Pos& airportPos, float biasXMeter, float biasYMeter)
{
  // Calculate offset position from bias in meter
  atools::geo::PosD pos(airportPos);
  pos.setLonX(pos.endpoint(biasXMeter, 90.).getLonX()); // Do precise calculation
  pos.setLatY(pos.getLatY() + atools::geo::meterToNm(static_cast<double>(biasYMeter)) / 60.); // 1 deg minute = 1 NM
  return pos.asPos();
}

void extend(atools::geo::Rect& rect, const atools::geo::Pos& pos)
{
  if(pos.isValid() && !pos.isNull() &&
     (!rect.isValid() || pos.distanceMeterTo(rect.getCenter()) < atools::geo::nmToMeter(MAX_DISTANCE_TO_BOUNDING_NM)))
    rect.extend(pos);
}

// ====================================================================================================

SimConnectWriter::SimConnectWriter(sql::SqlDatabase& sqlDb, bool verboseParam)
  : db(sqlDb), verbose(verboseParam)
{
  magDecReader = new atools::fs::common::MagDecReader;
  magDecReader->readFromWmm();
}

SimConnectWriter::~SimConnectWriter()
{
  deInitQueries();
  ATOOLS_DELETE_LOG(magDecReader);
}

bool SimConnectWriter::callProgressUpdate()
{
  return callProgress(QString(), false /* incProgress */);
}

QVector<RunwayTransition> SimConnectWriter::groupProcedures(const QVector<RunwayTransition>& runwayTransitions,
                                                            const QHash<int, QVector<const Runway *> > runwaysByNumber,
                                                            int airportNumRunwayEnds) const
{
  // KSEA {{ISBRG1/NESOE/34C}, {ISBRG1/CUSBU/16L}, {ISBRG1/DODVE/16R}}
  // KSEA {{MONTN2/PEAKK/16C, MONTN2/PEAKK/16L, MONTN2/PEAKK/16R}, {MONTN2/NEZUG/34C, MONTN2/NEZUG/34L, MONTN2/NEZUG/34R}}
  // EDDL {{BIKM1A/DUS/05L, BIKM1A/DUS/05R}, {BIKM1A/DUS/23L, BIKM1A/DUS/23R}}
  QVector<RunwayTransition> resultTransitions;

  // Detect "ALL" =======================================================================
  // Make unique hash by leg information only as key ignoring runways completely
  // Value is related list of transition pointers
  QHash<RunwayTransition, QVector<const RunwayTransition *> > transitionByName;
  for(const RunwayTransition& runwayTransPtr : runwayTransitions)
  {
    // Reset runway and designator to exclude in hashing and compare
    RunwayTransition runwayTransKey(runwayTransPtr);
    runwayTransKey.getTransitionFacility().runwayNumber = 0;
    runwayTransKey.getTransitionFacility().runwayDesignator = 0;

    if(transitionByName.contains(runwayTransKey))
      transitionByName[runwayTransKey].append(&runwayTransPtr);
    else
      transitionByName.insert(runwayTransKey, {&runwayTransPtr});
  }

  // List of remaining transtitions after removing ALL
  QVector<const RunwayTransition *> runwayTransitionsFiltered;
  for(auto it = transitionByName.begin(); it != transitionByName.end(); ++it)
  {
    // EDDL BIKM1A/DUS -> {{BIKM1A/DUS/05L, BIKM1A/DUS/05R}, {BIKM1A/DUS/23L, BIKM1A/DUS/23R}}
    QVector<const RunwayTransition *> transitionPtrs = transitionByName.value(it.key());

    // Number of referenced runways is equal to airport runways - set to "ALL"
    if(transitionPtrs.size() == airportNumRunwayEnds)
    {
      // If the set is the same size as number of airport runway ends it covers all
      // Add a modified copy to the result
      RunwayTransition resultTransition(*transitionPtrs.constFirst());
      resultTransition.setRunwayGroup("ALL");
      resultTransition.getTransitionFacility().runwayNumber = -1;
      resultTransition.getTransitionFacility().runwayDesignator = -1;
      resultTransitions.append(resultTransition);

      if(verbose)
      {
        qDebug() << Q_FUNC_INFO << "Convertingto ALL";
        for(const RunwayTransition *runwayTransPtr : transitionPtrs)
          qDebug() << Q_FUNC_INFO << *runwayTransPtr;
      }
    }
    else
    {
      // Add pointers to list for futher processing if runway list is not "ALL"
      for(const RunwayTransition *runwayTransition : transitionPtrs)
        runwayTransitionsFiltered.append(runwayTransition);
    }
  }

  // Detect parallel runways =======================================================================
  // Make unique hash by leg information and runway number as key ignoring runways completely
  // Value is related list of transition pointers
  QHash<RunwayTransition, QVector<const RunwayTransition *> > transitionByNameAndRwNum;
  for(const RunwayTransition *runwayTransPtr : runwayTransitionsFiltered)
  {
    // Reset designator to exclude in hashing and compare
    RunwayTransition runwayTransKey(*runwayTransPtr);
    runwayTransKey.getTransitionFacility().runwayDesignator = 0;
    if(transitionByNameAndRwNum.contains(runwayTransKey))
      transitionByNameAndRwNum[runwayTransKey].append(runwayTransPtr);
    else
      transitionByNameAndRwNum.insert(runwayTransKey, {runwayTransPtr});
  }

  for(auto it = transitionByNameAndRwNum.begin(); it != transitionByNameAndRwNum.end(); ++it)
  {
    // KSEA MONTN2/PEAKK/16 -> {MONTN2/PEAKK/16C, MONTN2/PEAKK/16L, MONTN2/PEAKK/16R}
    QVector<const RunwayTransition *> transitionPtrs = transitionByNameAndRwNum.value(it.key());
    if(transitionPtrs.size() == 1)
    {
      // References only a single runway - add copy to result
      resultTransitions.append(*transitionPtrs.constFirst());
      if(verbose)
        qDebug() << Q_FUNC_INFO << "Copying" << *transitionPtrs.constFirst();
    }
    else if(!transitionPtrs.isEmpty())
    {
      // More than one runway in the group which is grouped by runway number without designator
      const RunwayTransition *firstTrans = transitionPtrs.constFirst();
      int parallelRunwaysSize = runwaysByNumber.value(firstTrans->getTransitionFacility().runwayNumber).size();
      if(parallelRunwaysSize == transitionPtrs.size())
      {
        // GROUP - add first transition with group code
        RunwayTransition resultTransition(*firstTrans);

        resultTransition.getTransitionFacility().runwayDesignator = -1;
        resultTransition.setRunwayGroup(
          "RW" % util::runwayNamePrefixZero(bgl::converter::runwayToStr(resultTransition.getTransitionFacility().runwayNumber, 0)) % 'B');
        resultTransitions.append(resultTransition);

        if(verbose)
        {
          qDebug() << Q_FUNC_INFO << "Converting to" << resultTransition.getRunwayGroup();
          for(const RunwayTransition *runwayTransPtr : runwayTransitionsFiltered)
            qDebug() << Q_FUNC_INFO << *runwayTransPtr;
        }
      }
      else
      {
        for(const RunwayTransition *transition : transitionPtrs)
          resultTransitions.append(*transition);
      }
    }
  }

  return resultTransitions;
}

void SimConnectWriter::groupProcedures(Airport& airport) const
{
  // Runways grouped by number
  // runwayNumber, runways
  QHash<int, QVector<const Runway *> > runwaysByNumber;
  for(const Runway& runway : airport.getRunways())
  {
    if(runwaysByNumber.contains(runway.getFacility().primaryNumber))
      runwaysByNumber[runway.getFacility().primaryNumber].append(&runway);
    else
      runwaysByNumber.insert(runway.getFacility().primaryNumber, {&runway});

    if(runwaysByNumber.contains(runway.getFacility().secondaryNumber))
      runwaysByNumber[runway.getFacility().secondaryNumber].append(&runway);
    else
      runwaysByNumber.insert(runway.getFacility().secondaryNumber, {&runway});
  }

  int airportNumRunwayEnds = airport.getRunways().size() * 2;

  for(Arrival& arrival : airport.getArrivals())
  {
    if(verbose)
      qDebug() << Q_FUNC_INFO << "============ Arrival" << arrival.getArrivalFacility().name;
    arrival.setRunwayTransitions(groupProcedures(arrival.getRunwayTransitions(), runwaysByNumber, airportNumRunwayEnds));
  }

  for(Departure& departure : airport.getDepartures())
  {
    if(verbose)
      qDebug() << Q_FUNC_INFO << "============ Departure" << departure.getDepartureFacility().name;
    departure.setRunwayTransitions(groupProcedures(departure.getRunwayTransitions(), runwaysByNumber, airportNumRunwayEnds));
  }
}

bool SimConnectWriter::callProgress(const QString& message, bool incProgress)
{
  bool aborted = false;

  // Reset timers used in progress callback in thread context
  if(progressCallback)
  {
    if(!message.isEmpty())
      lastMessage = message;

    // Call only every UPDATE_RATE_MS
    if((timer.elapsed() - progressTimerElapsed) > UPDATE_RATE_MS || incProgress)
    {
      progressTimerElapsed = timer.elapsed();

      // Add dot animation
      QString dots;
      if(message.isEmpty())
        dots = tr(" ") % tr(".").repeated((progressCounter++) % 10);

      aborted = progressCallback(lastMessage % dots, incProgress);
    }
  }
  return aborted;
}

void SimConnectWriter::initQueries()
{
  deInitQueries();

  SqlUtil util(db);

  // create insert statement for table airport and exclude unneeded columns
  airportStmt = new SqlQuery(db);
  airportStmt->prepare(util.buildInsertStatement("airport", QString(),
                                                 {"icao", "iata", "faa", "local", "city", "state", "country", "flatten", "type",
                                                  "scenery_local_path", "bgl_filename"}));

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
  approachStmt->prepare(util.buildInsertStatement("approach", QString(), {"fix_airport_ident", "aircraft_category", "heading"}));

  transitionStmt = new SqlQuery(db);
  transitionStmt->prepare(util.buildInsertStatement("transition", QString(),
                                                    {"fix_airport_ident", "aircraft_category", "dme_airport_ident"}));

  approachLegStmt = new SqlQuery(db);
  approachLegStmt->prepare(util.buildInsertStatement("approach_leg", QString(),
                                                     {"fix_lonx", "fix_laty", "recommended_fix_fix_lonx", "recommended_fix_fix_laty"}));

  transitionLegStmt = new SqlQuery(db);
  transitionLegStmt->prepare(util.buildInsertStatement("transition_leg", QString(),
                                                       {"fix_airport_ident", "fix_lonx", "fix_laty", "recommended_fix_fix_lonx",
                                                        "recommended_fix_fix_laty"}));

  waypointStmt = new SqlQuery(db);
  waypointStmt->prepare(util.buildInsertStatement("waypoint", QString(), {"name", "airport_ident", "airport_id", "arinc_type"}));

  vorStmt = new SqlQuery(db);
  vorStmt->prepare(util.buildInsertStatement("vor", QString(), {"airport_id", "airport_ident"}));

  ndbStmt = new SqlQuery(db);
  ndbStmt->prepare(util.buildInsertStatement("ndb"));

  ilsStmt = new SqlQuery(db);
  ilsStmt->prepare(util.buildInsertStatement("ils", QString(), {"perf_indicator", "provider"}));

  tmpAirwayPointStmt = new SqlQuery(db);
  tmpAirwayPointStmt->prepare(util.buildInsertStatement("tmp_airway_point", QString(),
                                                        {"next_airport_ident", "previous_airport_ident", "next_maximum_altitude",
                                                         "previous_maximum_altitude"}));
}

const atools::fs::sc::db::FacilityIdSet SimConnectWriter::getNavaidIds()
{
  enum {IDENT, TYPE, LONX, LATY};

  FacilityIdSet ids;
  SqlQuery query("select distinct ident, type, lonx, laty from ( "
                 "select ident, region, 'V' as type, lonx, laty from vor "
                 "union "
                 "select ident, region, 'N' as type, lonx, laty from ndb "
                 "union "
                 "select ident, region, 'V' as type, lonx, laty from ils "
                 "union "
                 "select ident, region, 'W' as type, lonx, laty from waypoint)", db);
  query.exec();
  while(query.next())
    ids.insert(FacilityId(query.valueStr(IDENT), QString(), query.valueChar(TYPE), query.valueFloat(LONX), query.valueFloat(LATY)));
  return ids;
}

void SimConnectWriter::clearAllBoundValues()
{
  // Clear all values in case of exception to avoid using old values
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
  waypointStmt->clearBoundValues();
  vorStmt->clearBoundValues();
  ndbStmt->clearBoundValues();
  ilsStmt->clearBoundValues();
  tmpAirwayPointStmt->clearBoundValues();
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
  ATOOLS_DELETE(waypointStmt);
  ATOOLS_DELETE(vorStmt);
  ATOOLS_DELETE(ndbStmt);
  ATOOLS_DELETE(ilsStmt);
  ATOOLS_DELETE(tmpAirwayPointStmt);
}

bool SimConnectWriter::writeAirportsToDatabase(QHash<atools::fs::sc::db::IcaoId, atools::fs::sc::db::Airport>& airports, int fileId)
{
  ilsToRunwayMap.clear();

  atools::sql::SqlTransaction transaction(db);

  if(!airports.isEmpty())
    qDebug() << Q_FUNC_INFO << "Batch size" << airports.size();
  else
    qDebug() << Q_FUNC_INFO << "Batch is empty";

  bool aborted = callProgress(tr("Writing airport facilities to database"));

  while(!airports.isEmpty() && !aborted)
  {
    // Consume airport facility
    auto it = airports.constBegin();
    Airport airport = *it;
    QString airportIdent = airport.getIcao(), airportRegion = airport.getRegion();
    const AirportFacility& airportFacility = airport.getAirportFacility();
    airports.erase(it);

    groupProcedures(airport);

    try
    {
      // Write airport =========================================================================================
      Pos airportPos(airportFacility.longitude, airportFacility.latitude, airportFacility.altitude);
      Pos towerPos(airportFacility.towerLongitude, airportFacility.towerLatitude, airportFacility.towerAltitude);

      // Airport position is center and bounding will not be extended beyond MAX_DISTANCE_TO_BOUNDING_NM
      Rect bounding(airportPos);

      if(verbose)
        qDebug() << Q_FUNC_INFO << airportIdent << airportFacility.longitude << airportFacility.latitude;

      aborted = callProgressUpdate();

      // Set base information ==============================
      airportStmt->bindValue(":airport_id", ++airportId);
      airportStmt->bindValue(":file_id", fileId);
      airportStmt->bindValue(":ident", airportIdent);
      airportStmt->bindValue(":name", airportFacility.name);
      airportStmt->bindValue(":region", airportRegion);
      airportStmt->bindValue(":tower_frequency", airport.getTowerFrequency());
      airportStmt->bindValue(":atis_frequency", airport.getAtisFrequency());
      airportStmt->bindValue(":awos_frequency", airport.getAwosFrequency());
      airportStmt->bindValue(":asos_frequency", airport.getAsosFrequency());
      airportStmt->bindValue(":unicom_frequency", airport.getUnicomFrequency());
      airportStmt->bindValue(":is_military", util::isNameMilitary(airportFacility.name));
      airportStmt->bindValue(":is_closed", 0);
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
      airportStmt->bindValue(":num_runway_end_closed", 0);
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
        airportStmt->bindValue(":longest_runway_length", mToFt(runway.length));
        airportStmt->bindValue(":longest_runway_width", mToFt(runway.width));
        airportStmt->bindValue(":longest_runway_heading", runway.heading);
        airportStmt->bindValue(":longest_runway_surface", surfaceToDb(static_cast<Surface>(runway.surface)));
      }
      else
      {
        airportStmt->bindValue(":longest_runway_length", 0);
        airportStmt->bindValue(":longest_runway_width", 0);
        airportStmt->bindValue(":longest_runway_heading", 0.f);
        airportStmt->bindNullStr(":longest_runway_surface");
      }

      airportStmt->bindValue(":num_runways", airport.getRunways().size());
      airportStmt->bindValue(":largest_parking_ramp", airport.getLargestParkingRamp());
      airportStmt->bindValue(":largest_parking_gate", airport.getLargestParkingGate());

      // Calculate rating =======================================
      airportStmt->bindValue(":rating",
                             util::calculateAirportRating(false /* isAddon */,
                                                          !airport.getTowerFrequency().isNull(),
                                                          true /* msfs */,
                                                          airport.getTaxiPathFacilities().size(),
                                                          airport.getTaxiParkingFacilities().size(),
                                                          0 /* numAprons */));

      airportStmt->bindValue(":is_3d", 0);

      // SimConnect MAGVAR gives wrong indications - calculate using WMM for airports
      airportStmt->bindValue(":mag_var", magDecReader->getMagVar(airportPos));

      bindPosAlt(airportStmt, towerPos, "tower_");
      extend(bounding, towerPos);

      airportStmt->bindValue(":transition_altitude", mToFt(airportFacility.transitionAltitude));
      airportStmt->bindValue(":transition_level", mToFt(airportFacility.transitionLevel));
      bindPosAlt(airportStmt, airportPos);

      // Runways ===========================================================================================
      // Index stores all runway end ids for this airport - used by start positions and procedures for this one airport
      atools::fs::db::RunwayIndex runwayIndex;
      int numLightedRunways = 0;
      for(const Runway& runway : airport.getRunways())
      {
        // Check ranges and skip if invalid ====================
        // Primary threshold // Primary blastpad // Primary overrun
        // Secondary threshold // Secondary blastpad // Secondary overrun
        if(runway.getPavementFacilities().size() != 6)
          throw Exception(QString("Wrong pavements size %1 in airport %2").arg(runway.getPavementFacilities().size()).arg(airportIdent));

        // Primary approach lights // Secondary approach lights
        if(runway.getApproachLightFacilities().size() != 2)
          throw Exception(QString("Wrong approach lights size %1 in airport %2").
                          arg(runway.getApproachLightFacilities().size()).arg(airportIdent));

        // Primary left vasi // Primary right vasi // Secondary left vasi // Secondary right vasi
        if(runway.getVasiFacilities().size() != 4)
          throw Exception(QString("Wrong VASI lights size %1 in airport %2").arg(runway.getVasiFacilities().size()).arg(airportIdent));

        // Runway ===========================================================================================
        const RunwayFacility& runwayFacility = runway.getFacility();
        const ApproachLightFacility& primaryApproachLights = runway.getApproachLightFacilities().at(APPROACH_LIGHTS_PRIMARY);
        const ApproachLightFacility& secondaryApproachLights = runway.getApproachLightFacilities().at(APPROACH_LIGHTS_SECONDARY);

        runwayStmt->bindValue(":runway_id", ++runwayId);
        runwayStmt->bindValue(":airport_id", airportId);

        // Calculate end ids
        int primaryEndId = ++runwayEndId;
        int secondaryEndId = ++runwayEndId;

        runwayStmt->bindValue(":primary_end_id", primaryEndId);
        runwayStmt->bindValue(":secondary_end_id", secondaryEndId);
        runwayStmt->bindValue(":surface", surfaceToDb(static_cast<Surface>(runwayFacility.surface)));
        runwayStmt->bindValue(":length", mToFt(runwayFacility.length));
        runwayStmt->bindValue(":width", mToFt(runwayFacility.width));
        runwayStmt->bindValue(":heading", runwayFacility.heading);
        runwayStmt->bindValue(":pattern_altitude", atools::roundToInt(mToFt(runwayFacility.patternAltitude)));
        runwayStmt->bindValue(":marking_flags", 0);

        // Look for lighted runway taxipaths which overlap with this primary or secondary runway number =========
        // Not reliable if taxipath is missing
        bool pathLights = false;
        for(const TaxiPathFacility& path : airport.getTaxiPathFacilities())
        {
          // Any edge or center light makes the runway lighted
          if(path.type == bgl::taxipath::RUNWAY &&
             ((runwayFacility.primaryNumber == path.runwayNumber && runwayFacility.primaryDesignator == path.runwayDesignator) ||
              (runwayFacility.secondaryNumber == path.runwayNumber && runwayFacility.secondaryDesignator == path.runwayDesignator)))
            pathLights |= path.leftEdgeLighted | path.rightEdgeLighted | path.centerLineLighted;
        }

        // Assume that the runway is lighted if it has ILS, touchdown lights or any other approach lights ================
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
        Pos runwayCenterPos(runwayFacility.longitude, runwayFacility.latitude, runwayFacility.altitude);
        extend(bounding, runwayCenterPos);

        Pos primaryPos = runwayCenterPos.endpoint(runwayFacility.length / 2.f, atools::geo::opposedCourseDeg(runwayFacility.heading));
        primaryPos.setAltitude(runwayCenterPos.getAltitude());
        extend(bounding, primaryPos);

        Pos secondaryPos = runwayCenterPos.endpoint(runwayFacility.length / 2.f, runwayFacility.heading);
        secondaryPos.setAltitude(runwayCenterPos.getAltitude());
        extend(bounding, secondaryPos);

        bindPos(runwayStmt, primaryPos, "primary_");
        bindPos(runwayStmt, secondaryPos, "secondary_");
        bindPosAlt(runwayStmt, runwayCenterPos);
        runwayStmt->exec();

        // Primary runway end ===================================================
        auto pavements = runway.getPavementFacilities();
        runwayEndStmt->bindValue(":runway_end_id", primaryEndId);
        QString runwayName = bgl::converter::runwayToStr(runwayFacility.primaryNumber, runwayFacility.primaryDesignator);
        runwayEndStmt->bindValue(":name", runwayName);
        runwayEndStmt->bindValue(":end_type", "P");
        runwayEndStmt->bindValue(":offset_threshold", mToFt(pavements.at(PRIMARY_THRESHOLD).length));
        runwayEndStmt->bindValue(":blast_pad", mToFt(pavements.at(PRIMARY_BLASTPAD).length));
        runwayEndStmt->bindValue(":overrun", mToFt(pavements.at(PRIMARY_OVERRUN).length));
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
        runwayEndStmt->exec();

        // Add to runway end index and to ILS map
        runwayIndex.add(airportIdent, runwayName, primaryEndId);
        ilsToRunwayMap.insert(FacilityId(runwayFacility.primaryIlsIcao, runwayFacility.primaryIlsRegion),
                              RunwayId(airportIdent, airportRegion, runwayName, primaryEndId));

        // Secondary runway end ===================================================
        runwayEndStmt->bindValue(":runway_end_id", secondaryEndId);
        runwayName = bgl::converter::runwayToStr(runwayFacility.secondaryNumber, runwayFacility.secondaryDesignator);
        runwayEndStmt->bindValue(":name", runwayName);
        runwayEndStmt->bindValue(":end_type", "S");
        runwayEndStmt->bindValue(":offset_threshold", mToFt(pavements.at(SECONDARY_THRESHOLD).length));
        runwayEndStmt->bindValue(":blast_pad", mToFt(pavements.at(SECONDARY_BLASTPAD).length));
        runwayEndStmt->bindValue(":overrun", mToFt(pavements.at(SECONDARY_OVERRUN).length));
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
        runwayEndStmt->exec();

        // Add to runway end index and to ILS map
        runwayIndex.add(airportIdent, runwayName, secondaryEndId);
        ilsToRunwayMap.insert(FacilityId(runwayFacility.secondaryIlsIcao, runwayFacility.secondaryIlsRegion),
                              RunwayId(airportIdent, airportRegion, runwayName, secondaryEndId));
      } // for(const Runway& runway : airport.getRunways())

      // Starts ===========================================================================================
      // Maps position and index for helipad to start relation
      QVector<std::pair<Pos, int> > startIndex;
      for(const StartFacility& start : airport.getStartFacilities())
      {
        Pos startPos(start.longitude, start.latitude, start.altitude);

        if(valid(startPos))
        {
          startStmt->bindValue(":start_id", ++startId);
          startStmt->bindValue(":airport_id", airportId);

          // Look for runway ends in index if start is of type runway =======================
          bgl::start::StartType startType = static_cast<bgl::start::StartType>(start.type);
          if(startType == bgl::start::RUNWAY)
          {
            QString runwayName = bgl::converter::runwayToStr(start.number, start.designator);
            int endId = runwayIndex.getRunwayEndId(airportIdent, runwayName, "SimConnect - start " % startPos.toString());

            if(endId > 0)
              startStmt->bindValue(":runway_end_id", endId);
            else
              startStmt->bindNullInt(":runway_end_id");

            if(!runwayName.isEmpty() && runwayName != "RW00" && runwayName != "00")
              startStmt->bindValue(":runway_name", runwayName);
            else
              startStmt->bindNullStr(":runway_name");
          }
          else
            startStmt->bindNullStr(":runway_name");

          startStmt->bindValue(":type", enumToStr(bgl::Start::startTypeToStr, startType));
          startStmt->bindValue(":heading", start.heading);
          startStmt->bindValue(":number", start.number);

          bindPosAlt(startStmt, startPos);
          startStmt->exec();
          extend(bounding, startPos);
          startIndex.append(std::make_pair(startPos, startId));
        }
      }

      // Frequencies / COM ===========================================================================================
      for(const FrequencyFacility& frequency : airport.getFrequencyFacilities())
      {
        frequencyStmt->bindValue(":com_id", ++frequencyId);
        frequencyStmt->bindValue(":airport_id", airportId);
        frequencyStmt->bindValue(":type", enumToStr(bgl::Com::comTypeToStr, static_cast<bgl::com::ComType>(frequency.type)));
        frequencyStmt->bindValue(":frequency", frequency.frequency);
        frequencyStmt->bindValue(":name", frequency.name);
        frequencyStmt->exec();
      }

      // Helipads ===========================================================================================
      for(const HelipadFacility& helipad : airport.getHelipadFacilities())
      {
        Pos helipadPos(helipad.longitude, helipad.latitude, helipad.altitude);
        if(valid(helipadPos))
        {
          // Look for a start position by coordinates for the helipad =========
          int helipadStartId = -1;
          for(const std::pair<Pos, int>& start : startIndex)
          {
            if(start.first.almostEqual(helipadPos, atools::geo::Pos::POS_EPSILON_1M))
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

          helipadStmt->bindValue(":length", mToFt(helipad.length));
          helipadStmt->bindValue(":width", mToFt(helipad.width));
          helipadStmt->bindValue(":heading", helipad.heading);
          helipadStmt->bindValue(":is_transparent", 0);
          helipadStmt->bindValue(":is_closed", 0);
          bindPosAlt(helipadStmt, helipadPos);
          helipadStmt->exec();
          extend(bounding, helipadPos);
        }
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
        QString runwayName;
        int endId = 0;
        if(approachFacility.runwayNumber > 0)
        {
          runwayName = bgl::converter::runwayToStr(approachFacility.runwayNumber, approachFacility.runwayDesignator);
          endId = runwayIndex.getRunwayEndId(airportIdent, runwayName,
                                             "SimConnect - approach " % QString(approach.getApproachFacility().fafIcao));
        }

        if(endId > 0)
          approachStmt->bindValue(":runway_end_id", endId);
        else
          approachStmt->bindNullInt(":runway_end_id");

        if(!runwayName.isEmpty() && runwayName != "RW00" && runwayName != "00")
          approachStmt->bindValue(":runway_name", runwayName);
        else
          approachStmt->bindNullStr(":runway_name");

        // Build ARINC name
        approachStmt->bindValue(":arinc_name",
                                bgl::ap::arincNameAppr(static_cast<bgl::ap::ApproachType>(approachFacility.type), runwayName,
                                                       static_cast<char>(approachFacility.suffix), false));
        approachStmt->bindValue(":airport_ident", airportIdent);
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
        approachStmt->bindValue(":altitude", mToFt(approachFacility.fafAltitude));
        approachStmt->bindValue(":missed_altitude", mToFt(approachFacility.missedAltitude));

        // Approach Transitions ==============================
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
          transitionStmt->bindValue(":altitude", mToFt(transitionFacility.iafAltitude));
          transitionStmt->bindValue(":dme_ident", transitionFacility.dmeArcIcao);
          transitionStmt->bindValue(":dme_region", transitionFacility.dmeArcRegion);
          transitionStmt->bindValue(":dme_radial", transitionFacility.dmeArcRadial);
          transitionStmt->bindValue(":dme_distance", mToNm(transitionFacility.dmeArcDistance));

          // Transition legs ==============================
          for(const LegFacility& leg : transition.getApproachTransitionLegFacilities())
            writeLeg(transitionLegStmt, leg, TRANS);

          transitionStmt->exec();
        }

        // Approach legs =========
        bool verticalAngleFound = false; // Check for vertical path angle in legs
        for(const LegFacility& leg : approach.getFinalApproachLegFacilities())
          writeLeg(approachLegStmt, leg, APPROACH, &verticalAngleFound);

        // Missed approach legs =========
        for(const LegFacility& leg : approach.getMissedApproachLegFacilities())
          writeLeg(approachLegStmt, leg, MISSED);

        approachStmt->bindValue(":has_vertical_angle", verticalAngleFound);
        approachStmt->exec();
      }

      // STAR / Arrivals ===========================================================================================
      // Legs are saved in flying order from transition entry to STAR exit
      for(const Arrival& arrival : airport.getArrivals())
      {
        // Currently duplicate the full procedure for each runway transition instead of aggregating into
        // multi-runway definitions like "23B" or "ALL"
        for(const RunwayTransition& runwayTransition : arrival.getRunwayTransitions())
        {
          approachStmt->bindValue(":approach_id", ++approachId);
          approachStmt->bindValue(":airport_id", airportId);

          bindRunway(approachStmt, runwayIndex, airportIdent, runwayTransition,
                     "SimConnect - arrival " % QString(arrival.getArrivalFacility().name));
          approachStmt->bindValue(":airport_ident", airportIdent);
          approachStmt->bindValue(":type", "GPS");
          approachStmt->bindValue(":suffix", "A");
          approachStmt->bindValue(":has_gps_overlay", 1);
          approachStmt->bindValue(":has_rnp", 0);
          approachStmt->bindValue(":fix_type", QChar(runwayTransition.getLegFacilities().constFirst().fixType));
          approachStmt->bindValue(":fix_ident", arrival.getArrivalFacility().name);
          approachStmt->bindValue(":fix_region", runwayTransition.getLegFacilities().constFirst().fixRegion);
          approachStmt->bindNullFloat(":altitude");
          approachStmt->bindNullFloat(":missed_altitude");
          approachStmt->exec();

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
            transitionStmt->bindNullFloat(":altitude");
            transitionStmt->bindNullStr(":dme_ident");
            transitionStmt->bindNullStr(":dme_region");
            transitionStmt->bindNullFloat(":dme_radial");
            transitionStmt->bindNullFloat(":dme_distance");
            transitionStmt->exec();

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
          approachStmt->bindValue(":approach_id", ++approachId);
          approachStmt->bindValue(":airport_id", airportId);
          bindRunway(approachStmt, runwayIndex, airportIdent, runwayTransition,
                     "SimConnect - departure " % QString(departure.getDepartureFacility().name));
          approachStmt->bindValue(":airport_ident", airportIdent);
          approachStmt->bindValue(":type", "GPS");
          approachStmt->bindValue(":suffix", "D");
          approachStmt->bindValue(":has_gps_overlay", 1);
          approachStmt->bindValue(":has_rnp", 0);
          approachStmt->bindValue(":fix_type", QChar(runwayTransition.getLegFacilities().constLast().fixType));
          approachStmt->bindValue(":fix_ident", departure.getDepartureFacility().name);
          approachStmt->bindValue(":fix_region", runwayTransition.getLegFacilities().constLast().fixRegion);
          approachStmt->bindNullFloat(":altitude");
          approachStmt->bindNullFloat(":missed_altitude");
          approachStmt->exec();

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
            transitionStmt->bindNullFloat(":altitude");
            transitionStmt->bindNullStr(":dme_ident");
            transitionStmt->bindNullStr(":dme_region");
            transitionStmt->bindNullFloat(":dme_radial");
            transitionStmt->bindNullFloat(":dme_distance");
            transitionStmt->exec();

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
        Pos parkingPos(calcPosFromBias(airportPos, parking.biasX, parking.biasY));

        // Add only valid and omit vehicle parking
        if(valid(parkingPos) && parkingType != bgl::ap::VEHICLES && !parkingTypeStr.isEmpty())
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
          parkingStmt->bindValue(":radius", mToFt(parking.radius));
          parkingStmt->bindValue(":heading", parking.heading);
          parkingStmt->bindValue(":has_jetway", jetwayIndexes.contains(std::make_pair(parking.name, parking.suffix)));

          bindPos(parkingStmt, parkingPos);
          parkingStmt->exec();
          extend(bounding, parkingPos);
        }

        // Insert all parking spots
        parkingIndexes.insert(index, &parking);

        index++;
      }

      // Taxi path ===========================================================================================
      for(const TaxiPathFacility& path : airport.getTaxiPathFacilities())
      {
        if(path.type == bgl::taxipath::PATH || path.type == bgl::taxipath::TAXI || path.type == bgl::taxipath::PARKING)
        {
          // Check range violations and skip if any
          if(!atools::inRange(airport.getTaxiPointFacilities(), path.start))
          {
            qWarning() << Q_FUNC_INFO << "Start path index" << path.start << "out of range in airport" << airportIdent;
            continue;
          }

          // Look for path end either in points or parking
          if(path.type == bgl::taxipath::PARKING)
          {
            if(!parkingIndexes.contains(path.end))
            {
              qWarning() << Q_FUNC_INFO << airportIdent << "path.end out of range. Is" << path.end << path.type
                         << "from 0 to" << airport.getTaxiParkingFacilities().size();
              continue;
            }
          }
          else if(!atools::inRange(airport.getTaxiPointFacilities(), path.end))
          {
            qWarning() << Q_FUNC_INFO << airportIdent << "path.end out of range. Is" << path.end << path.type
                       << "from 0 to" << airport.getTaxiPointFacilities().size();
            continue;
          }

          // Taxi path ============================================================
          taxiPathStmt->bindValue(":taxi_path_id", ++taxiPathId);
          taxiPathStmt->bindValue(":airport_id", airportId);
          taxiPathStmt->bindValue(":type", enumToStr(bgl::TaxiPath::pathTypeToString, static_cast<bgl::taxipath::Type>(path.type)));
          taxiPathStmt->bindValue(":width", mToFt(path.width));
          taxiPathStmt->bindValue(":surface", surfaceToDb(TAXIPATH));

          // Bind name if index is valid
          if(atools::inRange(airport.getTaxiNameFacilities(), static_cast<int>(path.nameIndex)))
            taxiPathStmt->bindValue(":name", airport.getTaxiNameFacilities().at(static_cast<int>(path.nameIndex)).name);
          else
            qWarning() << Q_FUNC_INFO << airportIdent << "path.name out of range. Is" << path.nameIndex << path.type
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

          if(valid(startPos) && valid(endPos))
          {
            taxiPathStmt->bindValue(":end_type", endType);
            taxiPathStmt->bindValue(":end_dir", endDir);
            bindPos(taxiPathStmt, endPos, "end_");
            taxiPathStmt->exec();

            extend(bounding, startPos);
            extend(bounding, endPos);
          }
        }
      }

      // Finalize airport =================================
      airportStmt->bindValue(":fuel_flags", airportHasFuel ? bgl::ap::MSFS_DEFAULT_FUEL : bgl::ap::NO_FUEL_FLAGS);
      airportStmt->bindValue(":has_avgas", airportHasFuel);
      airportStmt->bindValue(":has_jetfuel", airportHasFuel);
      airportStmt->bindValue(":has_tower_object", valid(towerPos) && airport.getTowerFrequency().toInt() > 0);
      airportStmt->bindValue(":num_runway_light", numLightedRunways);
      airportStmt->bindValue(":num_approach", numProcedures);
      airportStmt->bindValue(":left_lonx", bounding.getTopLeft().getLonX());
      airportStmt->bindValue(":top_laty", bounding.getTopLeft().getLatY());
      airportStmt->bindValue(":right_lonx", bounding.getBottomRight().getLonX());
      airportStmt->bindValue(":bottom_laty", bounding.getBottomRight().getLatY());
      airportStmt->exec();

      airportFileStmt->bindValue(":airport_file_id", ++airportFileId);
      airportFileStmt->bindValue(":file_id", fileId);
      airportFileStmt->bindValue(":ident", airportIdent);
      airportFileStmt->exec();

    } // try
    catch(Exception& e)
    {
      errors.append(tr("Caught exception writing airport %1. Error: %2").arg(airportIdent).arg(e.what()));
      qWarning() << Q_FUNC_INFO << errors.constLast();

      clearAllBoundValues();
    }
    catch(...)
    {
      errors.append(tr("Caught unknown exception writing airport %1.").arg(airportIdent));
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
  return aborted;
}

bool SimConnectWriter::writeNdbToDatabase(const QList<NdbFacility>& ndbs, int fileId)
{
  atools::sql::SqlTransaction transaction(db);

  bool aborted = callProgress(tr("Writing NDB to database"));

  for(const NdbFacility& ndb : ndbs)
  {
    if(aborted)
      return true;
    float magvar = magDecReader->getMagVar(ndb.longitude, ndb.latitude);

    ndbStmt->bindValue(":ndb_id", ++ndbId);
    ndbStmt->bindValue(":file_id", fileId);
    ndbStmt->bindValue(":ident", ndb.icao);
    ndbStmt->bindValue(":name", ndb.name);
    ndbStmt->bindValue(":region", ndb.region);
    ndbStmt->bindValue(":frequency", ndb.frequency / 10);
    ndbStmt->bindValue(":range", mToNm(ndb.range));
    ndbStmt->bindValue(":type", enumToStr(bgl::Ndb::ndbTypeToStr, static_cast<atools::fs::bgl::nav::NdbType>(ndb.type)));
    ndbStmt->bindValue(":mag_var", magvar);
    bindPosAlt(ndbStmt, ndb.longitude, ndb.latitude, ndb.altitude);
    ndbStmt->exec();

    // Create a shadow waypoint for this NDB which can be used to connect airways - will be hidden in the GUI
    // Counts are updated in update_wp_ids.sql
    waypointStmt->bindValue(":waypoint_id", ++waypointId);
    waypointStmt->bindValue(":file_id", fileId);
    waypointStmt->bindValue(":nav_id", ndbId);
    waypointStmt->bindValue(":ident", ndb.icao);
    waypointStmt->bindValue(":region", ndb.region);
    waypointStmt->bindValue(":mag_var", magvar);
    waypointStmt->bindValue(":artificial", 1); // Indicates hidden shadow
    waypointStmt->bindValue(":type", enumToStr(bgl::Waypoint::waypointTypeToStr, bgl::nav::NDB));
    waypointStmt->bindValue(":num_victor_airway", 0);
    waypointStmt->bindValue(":num_jet_airway", 0);
    bindPos(waypointStmt, ndb.longitude, ndb.latitude);
    waypointStmt->exec();
  }

  transaction.commit();
  return aborted;

}

bool SimConnectWriter::writeVorAndIlsToDatabase(const QList<VorFacility>& vors, int fileId)
{
  atools::sql::SqlTransaction transaction(db);

  bool aborted = callProgress(tr("Writing VOR and ILS to database"));

  for(const VorFacility& vorIls : vors)
  {
    if(aborted)
      return true;

    // Type is not reliable - have to use other methods to detect ILS
    if(vorIls.isIls())
    {
      // Write ILS ==========================================================================================
      float magvar = magDecReader->getMagVar(vorIls.vorLongitude, vorIls.vorLatitude);
      float heading = atools::geo::normalizeCourse(vorIls.localizer + bgl::converter::adjustMagvar(vorIls.magvar));

      ilsStmt->bindValue(":ils_id", ++ilsId);
      ilsStmt->bindValue(":ident", vorIls.icao);
      ilsStmt->bindValue(":name", vorIls.name);
      ilsStmt->bindValue(":region", vorIls.region);
      ilsStmt->bindValue(":type", lsTypeToDb(static_cast<LsCategory>(vorIls.lsCategory)));
      ilsStmt->bindValue(":frequency", vorIls.frequency / 1000);
      ilsStmt->bindValue(":range", roundToInt(mToNm(vorIls.navRange)));
      ilsStmt->bindValue(":mag_var", magvar);
      ilsStmt->bindValue(":has_backcourse", vorIls.hasBackCourse);

      // DME part ==================================
      if(vorIls.isDme)
      {
        bindPosAlt(ilsStmt, vorIls.dmeLongitude, vorIls.dmeLatitude, vorIls.dmeAltitude, "dme_");
        ilsStmt->bindValue(":dme_range", roundToInt(mToNm(vorIls.navRange)));
      }
      else
      {
        bindPosAltNull(ilsStmt, "dme_");
        ilsStmt->bindNullInt(":dme_range");
      }

      // GS part ==================================
      if(vorIls.hasGlideSlope)
      {
        bindPosAlt(ilsStmt, vorIls.gsLongitude, vorIls.gsLatitude, vorIls.gsAltitude, "gs_");
        ilsStmt->bindValue(":gs_range", roundToInt(mToNm(vorIls.navRange)));
        ilsStmt->bindValue(":gs_pitch", vorIls.glideSlope);
      }
      else
      {
        bindPosAltNull(ilsStmt, "gs_");
        ilsStmt->bindNullInt(":gs_range");
        ilsStmt->bindNullFloat(":gs_pitch");
      }

      // Get runway for localizer ==================================
      RunwayId runwayId = ilsToRunwayMap.value(FacilityId(vorIls.icao, vorIls.region));
      if(runwayId.isValid())
      {
        ilsStmt->bindValue(":loc_runway_end_id", runwayId.getRunwayEndId());
        ilsStmt->bindValue(":loc_airport_ident", runwayId.getAirportIdent());
        ilsStmt->bindValue(":loc_runway_name", runwayId.getRunway());
      }
      else
      {
        ilsStmt->bindNullInt(":loc_runway_end_id");
        ilsStmt->bindNullStr(":loc_airport_ident");
        ilsStmt->bindNullStr(":loc_runway_name");
      }

      // Localizer part - always present ==================================
      ilsStmt->bindValue(":loc_heading", heading);
      ilsStmt->bindValue(":loc_width", vorIls.localizerWidth);

      // Calculate feather geometry ===================================
      Pos p1, p2, pmid, pos(vorIls.vorLongitude, vorIls.vorLatitude);
      util::calculateIlsGeometry(pos, heading, vorIls.localizerWidth, atools::fs::util::DEFAULT_FEATHER_LEN_NM, p1, p2, pmid);

      bindPos(ilsStmt, p1, "end1_");
      bindPos(ilsStmt, pmid, "end_mid_");
      bindPos(ilsStmt, p2, "end2_");
      bindPosAlt(ilsStmt, vorIls.vorLongitude, vorIls.vorLatitude, vorIls.vorAltitude);
      ilsStmt->exec();
    }
    else
    {
      // Write VOR, VORDME, DME, VORTAC and TACAN =========================================================================
      vorStmt->bindValue(":vor_id", ++vorId);
      vorStmt->bindValue(":file_id", fileId);
      vorStmt->bindValue(":ident", vorIls.icao);
      vorStmt->bindValue(":name", vorIls.name);
      vorStmt->bindValue(":region", vorIls.region);
      vorStmt->bindValue(":type", vorTypeToDb(static_cast<VorType>(vorIls.type), vorIls.isNav, vorIls.isTacan));

      // Write channel for TACAN and VORTAC
      if(vorIls.isTacan)
        vorStmt->bindValue(":channel", util::tacanChannelForFrequency(vorIls.frequency / 10000));

      float magvar = bgl::converter::adjustMagvar(vorIls.magvar);
      vorStmt->bindValue(":frequency", vorIls.frequency / 1000);
      vorStmt->bindValue(":range", roundToInt(mToNm(vorIls.navRange)));
      vorStmt->bindValue(":mag_var", magvar);
      vorStmt->bindValue(":dme_only", !vorIls.isNav && !vorIls.isTacan && vorIls.isDme);
      if(vorIls.isDme)
        bindPosAlt(vorStmt, vorIls.dmeLongitude, vorIls.dmeLatitude, vorIls.dmeAltitude, "dme_");
      else
        bindPosAltNull(vorStmt, "dme_");
      bindPosAlt(vorStmt, vorIls.vorLongitude, vorIls.vorLatitude, vorIls.vorAltitude);
      vorStmt->exec();

      // Create a shadow waypoint for this VOR which can be used to connect airways - will be hidden in the GUI
      // Counts are updated in update_wp_ids.sql
      waypointStmt->bindValue(":waypoint_id", ++waypointId);
      waypointStmt->bindValue(":file_id", fileId);
      waypointStmt->bindValue(":nav_id", vorId);
      waypointStmt->bindValue(":ident", vorIls.icao);
      waypointStmt->bindValue(":region", vorIls.region);
      waypointStmt->bindValue(":mag_var", magvar);
      waypointStmt->bindValue(":artificial", 1); // Indicates hidden shadow
      waypointStmt->bindValue(":type", enumToStr(bgl::Waypoint::waypointTypeToStr, bgl::nav::VOR));
      waypointStmt->bindValue(":num_victor_airway", 0);
      waypointStmt->bindValue(":num_jet_airway", 0);
      bindPos(waypointStmt, vorIls.vorLongitude, vorIls.vorLatitude);
      waypointStmt->exec();
    }
  }
  transaction.commit();
  return aborted;
}

bool SimConnectWriter::writeWaypointsAndAirwaysToDatabase(const QMap<unsigned long, Waypoint>& waypoints, int fileId)
{
  atools::sql::SqlTransaction transaction(db);

  if(!waypoints.isEmpty())
    qDebug() << Q_FUNC_INFO << "Batch size" << waypoints.size() << "from"
             << waypoints.first().getWaypointFacility().icao << "..." << waypoints.last().getWaypointFacility().icao;
  else
    qDebug() << Q_FUNC_INFO << "Batch is empty";

  bool aborted = callProgress(tr("Writing waypoints and airways to database"));

  for(const Waypoint& waypoint : waypoints)
  {
    if(aborted)
      return true;

    const WaypointFacility& waypointFacility = waypoint.getWaypointFacility();
    bgl::nav::WaypointType type = static_cast<bgl::nav::WaypointType>(waypointFacility.type);
    float magvar = magDecReader->getMagVar(waypointFacility.longitude, waypointFacility.latitude);

    waypointStmt->bindValue(":waypoint_id", ++waypointId);
    waypointStmt->bindValue(":file_id", fileId);
    waypointStmt->bindNullInt(":nav_id"); // Filled later in script update_wp_ids.sql
    waypointStmt->bindValue(":ident", waypointFacility.icao);
    waypointStmt->bindValue(":region", waypointFacility.region);
    waypointStmt->bindValue(":mag_var", magvar);

    if((type == bgl::nav::VOR || type == bgl::nav::NDB) /* && (waypoint.getNumVictorAirway() > 0 || waypoint.getNumJetAirway() > 0)*/)
      waypointStmt->bindValue(":artificial", 1); // Indicates hidden shadow
    else
      waypointStmt->bindNullInt(":artificial");

    waypointStmt->bindValue(":type", enumToStr(bgl::Waypoint::waypointTypeToStr, type));
    waypointStmt->bindValue(":num_victor_airway", waypoint.getNumVictorAirway());
    waypointStmt->bindValue(":num_jet_airway", waypoint.getNumJetAirway());
    bindPos(waypointStmt, waypointFacility.longitude, waypointFacility.latitude);
    waypointStmt->exec();

    // Save routes to temporary table which are resolved later to keys ================================
    for(const RouteFacility& route : waypoint.getRouteFacilities())
    {
      bgl::nav::AirwayType airwayType = static_cast<bgl::nav::AirwayType>(route.type);

      tmpAirwayPointStmt->bindValue(":airway_point_id", ++tmpAirwayPointId);
      tmpAirwayPointStmt->bindValue(":waypoint_id", waypointId);
      tmpAirwayPointStmt->bindValue(":name", route.name);
      tmpAirwayPointStmt->bindValue(":type", enumToStr(bgl::AirwaySegment::airwayTypeToStr, airwayType));
      tmpAirwayPointStmt->bindValue(":mid_type", enumToStr(bgl::AirwayWaypoint::airwayWaypointTypeToStr,
                                                           bgl::Waypoint::airwayWaypointTypeFromWaypointType(type)));
      tmpAirwayPointStmt->bindValue(":mid_ident", waypointFacility.icao);
      tmpAirwayPointStmt->bindValue(":mid_region", waypointFacility.region);

      // Next waypoint ============================================================
      if(strlen(route.nextIcao) > 0)
      {
        tmpAirwayPointStmt->bindValue(":next_direction", "N");
        tmpAirwayPointStmt->bindValue(":next_type", waypointTypeToRouteDb(route.nextType));
        tmpAirwayPointStmt->bindValue(":next_ident", route.nextIcao);
        tmpAirwayPointStmt->bindValue(":next_region", route.nextRegion);
        tmpAirwayPointStmt->bindValue(":next_minimum_altitude", mToFt(route.nextAltitude));
      }
      else
      {
        tmpAirwayPointStmt->bindNullStr(":next_direction");
        tmpAirwayPointStmt->bindNullStr(":next_type");
        tmpAirwayPointStmt->bindNullStr(":next_ident");
        tmpAirwayPointStmt->bindNullStr(":next_region");
        tmpAirwayPointStmt->bindNullInt(":next_minimum_altitude");
      }

      // Previous waypoint ============================================================
      if(strlen(route.prevIcao) > 0)
      {
        tmpAirwayPointStmt->bindValue(":previous_direction", "N");
        tmpAirwayPointStmt->bindValue(":previous_type", waypointTypeToRouteDb(route.prevType));
        tmpAirwayPointStmt->bindValue(":previous_ident", route.prevIcao);
        tmpAirwayPointStmt->bindValue(":previous_region", route.prevRegion);
        tmpAirwayPointStmt->bindValue(":previous_minimum_altitude", mToFt(route.prevAltitude));
      }
      else
      {
        tmpAirwayPointStmt->bindNullStr(":previous_direction");
        tmpAirwayPointStmt->bindNullStr(":previous_type");
        tmpAirwayPointStmt->bindNullStr(":previous_ident");
        tmpAirwayPointStmt->bindNullStr(":previous_region");
        tmpAirwayPointStmt->bindNullInt(":previous_minimum_altitude");
      }

      tmpAirwayPointStmt->exec();
    }
  }
  transaction.commit();

  return aborted;
}

void SimConnectWriter::bindRunway(atools::sql::SqlQuery *query, const atools::fs::db::RunwayIndex& runwayIndex, const QString& airportIcao,
                                  const RunwayTransition& runwayTransition, const QString& sourceObject) const
{
  if(!runwayTransition.getRunwayGroup().isEmpty())
  {
    approachStmt->bindValue(":arinc_name", runwayTransition.getRunwayGroup());
    approachStmt->bindNullStr(":runway_name");
    approachStmt->bindNullInt(":runway_end_id");
  }
  else
  {
    // Get runway name and end id from index
    const RunwayTransitionFacility& transitionFacility = runwayTransition.getTransitionFacility();
    QString runwayName = bgl::converter::runwayToStr(transitionFacility.runwayNumber, transitionFacility.runwayDesignator);
    int endId = runwayIndex.getRunwayEndId(airportIcao, runwayName, sourceObject);

    if(endId > 0)
      query->bindValue(":runway_end_id", endId);
    else
      query->bindNullInt(":runway_end_id");

    if(!runwayName.isEmpty() && runwayName != "RW00" && runwayName != "00")
    {
      query->bindValue(":arinc_name", QString("RW") + runwayName);
      query->bindValue(":runway_name", runwayName);
    }
    else
    {
      query->bindNullInt(":arinc_name");
      query->bindNullStr(":runway_name");
    }

  }
}

void SimConnectWriter::bindVasi(atools::sql::SqlQuery *query, const Runway& runway, bool primary) const
{
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
      // Uses approach table
      query->bindValue(":approach_leg_id", ++approachLegId);
      query->bindValue(":approach_id", approachId);
      query->bindValue(":is_missed", type == MISSED);
      break;

    case TRANS:
    case SIDTRANS:
    case STARTRANS:
      // Uses transition table
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
  query->bindValue(":rnp", mToNm(leg.requiredNavigationPerformance));
  query->bindValue(":fix_type", QChar(leg.fixType));
  query->bindValue(":fix_ident", leg.fixIcao);
  query->bindValue(":fix_region", leg.fixRegion);

  // Omit coordinates since they seem to be not accurate - let Little Navmap resolve the fixes by name
  // Recommended ===============================================================
  if(strlen(leg.arcCenterFixIcao) > 0)
  {
    query->bindValue(":recommended_fix_type", QChar(leg.arcCenterFixType));
    query->bindValue(":recommended_fix_ident", leg.arcCenterFixIcao);
    query->bindValue(":recommended_fix_region", leg.arcCenterFixRegion);
  }
  else
  {
    query->bindValue(":recommended_fix_type", QChar(leg.originType));
    query->bindValue(":recommended_fix_ident", leg.originIcao);
    query->bindValue(":recommended_fix_region", leg.originRegion);
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
    query->bindValue(":distance", mToNm(leg.routeDistance));
    query->bindNullFloat(":time");
  }

  query->bindValue(":theta", leg.theta);
  query->bindValue(":rho", mToNm(leg.rho));

  query->bindValue(":altitude1", mToFt(leg.altitude1));
  query->bindValue(":altitude2", mToFt(leg.altitude2));

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
  query->exec();
}

} // namespace db
} // namespace sc
} // namespace fs
} // namespace atools
