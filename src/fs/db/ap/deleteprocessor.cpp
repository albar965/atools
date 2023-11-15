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

#include "fs/db/ap/deleteprocessor.h"

#include "fs/bgl/ap/airport.h"
#include "fs/bgl/ap/del/deleteairport.h"
#include "fs/bgl/util.h"
#include "fs/navdatabaseoptions.h"
#include "geo/calculations.h"
#include "sql/sqldatabase.h"
#include "sql/sqlquery.h"
#include "sql/sqlutil.h"

#include <QDebug>

namespace atools {
namespace fs {
namespace db {

using atools::fs::bgl::DeleteAirport;
using atools::fs::bgl::Airport;
using atools::sql::SqlQuery;
using atools::sql::SqlUtil;
using bgl::util::isFlagSet;

DeleteProcessor::DeleteProcessor(atools::sql::SqlDatabase& sqlDb, const NavDatabaseOptions& opts)
  : options(opts), db(&sqlDb)
{
  // Create all queries
  deleteRunwayStmt = new SqlQuery(sqlDb);
  updateRunwayStmt = new SqlQuery(sqlDb);
  deleteParkingStmt = new SqlQuery(sqlDb);
  updateParkingStmt = new SqlQuery(sqlDb);

  fetchRunwayEndIdStmt = new SqlQuery(sqlDb);
  deleteRunwayEndStmt = new SqlQuery(sqlDb);

  updateApproachRwIds = new SqlQuery(sqlDb);
  updateApproachStmt = new SqlQuery(sqlDb);
  deleteApproachStmt = new SqlQuery(sqlDb);

  updateWpStmt = new SqlQuery(sqlDb);
  updateVorStmt = new SqlQuery(sqlDb);
  updateNdbStmt = new SqlQuery(sqlDb);
  deleteAirportStmt = new SqlQuery(sqlDb);
  selectAirportStmt = new SqlQuery(sqlDb);
  deleteApronStmt = new SqlQuery(sqlDb);
  updateApronStmt = new SqlQuery(sqlDb);
  deleteHelipadStmt = new SqlQuery(sqlDb);
  updateHelipadStmt = new SqlQuery(sqlDb);
  deleteStartStmt = new SqlQuery(sqlDb);
  updateStartStmt = new SqlQuery(sqlDb);
  deleteTaxiPathStmt = new SqlQuery(sqlDb);
  updateTaxiPathStmt = new SqlQuery(sqlDb);
  deleteComStmt = new SqlQuery(sqlDb);
  updateComStmt = new SqlQuery(sqlDb);
  fetchPrimaryAppStmt = new SqlQuery(sqlDb);
  fetchSecondaryAppStmt = new SqlQuery(sqlDb);

  updateBoundingStmt = new SqlQuery(sqlDb);
  fetchBoundingStmt = new SqlQuery(sqlDb);

  // Most queries act on all other airports with the given ident except the current one
  // where a.ident = :apIdent and a.airport_id <> :curApId

  // Define subqueries for features to delete

  // Delete all runways of all other airports
  deleteRunwayStmt->prepare("delete from runway where airport_id  = :prevApId");

  // Delete all runways of all other airports
  updateRunwayStmt->prepare(updateAptFeatureStmt("runway"));

  // Get all primary and secondary end ids for the given airports
  fetchRunwayEndIdStmt->prepare(
    "select r.primary_end_id as runway_end_id "
    "from airport a join runway r on a.airport_id = r.airport_id "
    "where a.airport_id = :prevApId "
    "union "
    "select r.secondary_end_id as runway_end_id "
    "from airport a join runway r on a.airport_id = r.airport_id "
    "where a.airport_id == :prevApId");

  // Delete a runway end
  deleteRunwayEndStmt->prepare("delete from runway_end where runway_end_id = :endId");

  // Delete all other airports
  deleteAirportStmt->prepare("delete from airport where airport_id = :prevApId");

  // Get facility counts and ID for previous airport to save some empty queries
  selectAirportStmt->prepare(
    "select airport_id, name, city, state, country, region, "
    "num_apron, num_com, num_helipad, num_taxi_path, num_runways, num_approach, num_starts, "
    "is_addon, rating, bgl_filename, scenery_local_path, altitude, lonx, laty "
    "from airport where ident = :apIdent and airport_id <> :curApId order by airport_id desc limit 1");

  // Delete all facilities of the old airport
  deleteComStmt->prepare(delAptFeatureStmt("com"));
  deleteHelipadStmt->prepare(delAptFeatureStmt("helipad"));
  deleteStartStmt->prepare(delAptFeatureStmt("start"));
  deleteParkingStmt->prepare(delAptFeatureStmt("parking"));
  updateParkingStmt->prepare(updateAptFeatureStmt("parking"));
  deleteApronStmt->prepare(delAptFeatureStmt("apron"));
  deleteTaxiPathStmt->prepare(delAptFeatureStmt("taxi_path"));

  // Update NDB and VOR airport ids - resulting duplicates will be deleted later
  updateWpStmt->prepare(updateAptFeatureStmt("waypoint"));
  updateVorStmt->prepare(updateAptFeatureStmt("vor"));
  updateNdbStmt->prepare(updateAptFeatureStmt("ndb"));

  // Update facilities with new airport id
  updateComStmt->prepare(updateAptFeatureStmt("com"));
  updateHelipadStmt->prepare(updateAptFeatureStmt("helipad"));
  updateStartStmt->prepare(updateAptFeatureStmt("start"));
  updateApronStmt->prepare(updateAptFeatureStmt("apron"));
  updateTaxiPathStmt->prepare(updateAptFeatureStmt("taxi_path"));

  // Relink approach (and everything dependent) to a new airport
  updateApproachRwIds->prepare("update approach set runway_end_id = :newRwId where runway_end_id = :oldRwId");
  updateApproachStmt->prepare(updateAptFeatureStmt("approach"));
  deleteApproachStmt->prepare(delAptFeatureStmt("approach"));

  updateBoundingStmt->prepare("update airport set left_lonx = :leftlonx, top_laty = :toplaty, "
                              "right_lonx = :rightlonx, bottom_laty = :bottomlaty "
                              "where airport_id = :apid");

  fetchBoundingStmt->prepare(
    "select min(lonx) as left_lonx, max(laty) as top_laty, max(lonx) as right_lonx, min(laty) as bottom_laty "
    "from ( "
    "  select primary_lonx as lonx, primary_laty as laty from runway where airport_id = :apid "
    "  union "
    "  select secondary_lonx as lonx, secondary_laty as laty from runway where airport_id = :apid "
    "  union "
    "  select lonx, laty from parking where airport_id = :apid "
    "  union "
    "  select lonx, laty from helipad where airport_id = :apid "
    "  union "
    "  select start_lonx as lonx, start_laty as laty from taxi_path where airport_id = :apid "
    "  union "
    "  select end_lonx as lonx, end_laty as laty from taxi_path where airport_id = :apid "
    ")");

}

DeleteProcessor::~DeleteProcessor()
{
  delete deleteRunwayStmt;
  delete updateRunwayStmt;
  delete deleteParkingStmt;
  delete updateParkingStmt;
  delete fetchRunwayEndIdStmt;
  delete deleteRunwayEndStmt;
  delete updateApproachRwIds;
  delete updateApproachStmt;
  delete deleteApproachStmt;
  delete updateWpStmt;
  delete updateVorStmt;
  delete updateNdbStmt;
  delete deleteAirportStmt;
  delete selectAirportStmt;
  delete deleteApronStmt;
  delete updateApronStmt;
  delete deleteHelipadStmt;
  delete updateHelipadStmt;
  delete deleteStartStmt;
  delete updateStartStmt;
  delete deleteTaxiPathStmt;
  delete updateTaxiPathStmt;
  delete deleteComStmt;
  delete updateComStmt;
  delete fetchPrimaryAppStmt;
  delete fetchSecondaryAppStmt;

  delete updateBoundingStmt;
  delete fetchBoundingStmt;
}

void DeleteProcessor::init(const DeleteAirport *deleteAirportRec, const Airport *airport, int airportId,
                           const QString& name, const QString& city, const QString& state, const QString& country, const QString& region)
{
  if(options.isVerbose())
    qInfo() << Q_FUNC_INFO << airport->getIdent() << "curAirportId" << curAirportId;

  curAirport = airport;
  curAirportId = airportId;
  curName = name;
  curCity = city;
  curState = state;
  curCountry = country;
  curRegion = region;
  curIdent = curAirport->getIdent();
  deleteAirport = deleteAirportRec;
  deleteFlags = bgl::del::NONE;
}

void DeleteProcessor::preProcessDelete()
{
  if(options.isVerbose())
    qInfo() << Q_FUNC_INFO;

  // Calculate delete flags either from delete record or current airport
  extractDeleteFlags();

  // Get facility counts for current airport
  extractPreviousAirportFeatures();

  // Delete the whole tree of approaches, transitions and legs on the old airport later in
  // ":/atools/resources/sql/fs/db/delete_duplicates.sql"

  // if(prevHasApproach && isFlagSet(deleteFlags, bgl::del::APPROACHES))
  // {
  // SqlUtil sql(db);
  // sql.bindAndExec("delete from transition where transition.approach_id in "
  // "(select a.approach_id from approach a where a.airport_id = :prevApId)",
  // ":prevApId", prevAirportId);
  // sql.bindAndExec("delete from approach where airport_id = :prevApId",
  // ":prevApId", prevAirportId);
  // }

  // ILS will be deleted later by deduplication
}

void DeleteProcessor::postProcessDelete()
{
  static const QStringList FUEL_COLUMNS({"fuel_flags", "has_avgas", "has_jetfuel"});
  static const QStringList COM_COLUMNS({"tower_frequency", "atis_frequency", "awos_frequency", "asos_frequency", "unicom_frequency",
                                        "num_com"});
  static const QStringList RUNWAY_COLUMNS({"is_closed", "num_runway_hard", "num_runway_soft", "num_runway_water",
                                           "num_runway_light", "num_runway_end_closed", "num_runway_end_vasi", "num_runway_end_als",
                                           "longest_runway_length", "longest_runway_width", "longest_runway_heading",
                                           "longest_runway_surface", "num_runways", "left_lonx", "top_laty", "right_lonx", "bottom_laty"});
  static const QStringList AIRPORT_COLUMNS({"num_parking_gate", "num_parking_ga_ramp", "num_parking_cargo", "num_parking_mil_cargo",
                                            "num_parking_mil_combat", "num_jetway", "largest_parking_ramp", "largest_parking_gate"});

  if(options.isVerbose())
    qInfo() << Q_FUNC_INFO << curAirport->getIdent() << "current id" << curAirportId;

  if(prevAirportId == -1)
    return;

  // Collects all columns that will be copied from the previous airport to this new one
  QStringList copyAirportColumns;

  // Copy all names over from the previous airport if this one has empty names =================
  if(!prevName.isEmpty() && curName.isEmpty())
  {
    copyAirportColumns.append("name");
    copyAirportColumns.append("is_military");
  }

  if(!prevCity.isEmpty() && curCity.isEmpty())
    copyAirportColumns.append("city");

  if(!prevState.isEmpty() && curState.isEmpty())
    copyAirportColumns.append("state");

  if(!prevCountry.isEmpty() && curCountry.isEmpty())
    copyAirportColumns.append("country");

  if(!prevRegion.isEmpty() && curRegion.isEmpty())
    copyAirportColumns.append("region");

  // OBSOLETE: Imitating the wrong behavior of MSFS if an update tries to change the ident of an airport which was used at another one before.
  // Example: Boyd Field (54XS) in MSFS which actually Bar C Ranch Airport (54XS) at another location.
  // UPDATED:
  // Aboyne Airfield (EGZM), APX47110.bgl ->
  // Aboyne Airfield (EGAO), Official/OneStore/microsoft-airport-egao-aboyneairfield/scenery/microsoft/egao-mc/EGAO.bgl

  bool movedFar = false;
  if(options.getSimulatorType() == atools::fs::FsPaths::MSFS && prevPos.isValid() &&
     prevPos.distanceMeterTo(curAirport->getPos()) > atools::geo::nmToMeter(20))
  {
    // Do not copy old airport coordinates to new one if distance is more than 10 NM
    // Report far moved airport and update bounding rectangle later
    // copyAirportColumns.append("lonx");
    // copyAirportColumns.append("laty");
    if(deleteAirport != nullptr)
      qDebug() << Q_FUNC_INFO << "Moved far airport"
               << "new" << curAirport->getIdent() << curAirport->getName() << curAirport->getPos()
               << "prev" << prevAirportId << prevName << prevPos
               << "deleteAirport" << *deleteAirport;
    else
      qDebug() << Q_FUNC_INFO << "Moved far airport"
               << "newAirport " << curAirport->getIdent() << curAirport->getName() << curAirport->getPos()
               << "prev" << prevAirportId << prevName << prevPos;
    movedFar = true;
  }

  if(prevHasApproach)
  {
    removeOrUpdate(deleteApproachStmt, updateApproachStmt, bgl::del::APPROACHES);

    if(hasPrevious && !isFlagSet(deleteFlags, bgl::del::APPROACHES))
      // Relink the approaches to the new airport and update the count on the airport
      // transferApproaches(); not needed this is covered by sql update script
      copyAirportColumns.append("num_approach");
  }

  // Work on facilities that will be either removed or attached to the new airport depending on flags
  if(prevHasApron)
  {
    removeOrUpdate(deleteApronStmt, updateApronStmt, bgl::del::APRONS);

    if(!isFlagSet(deleteFlags, bgl::del::APRONS) && hasPrevious)
      // Update apron count in new airport
      copyAirportColumns.append("num_apron");
  }

  if(prevHasCom)
  {
    removeOrUpdate(deleteComStmt, updateComStmt, bgl::del::COMS);

    if(!isFlagSet(deleteFlags, bgl::del::COMS) && hasPrevious)
      // Copy all frequencies to the new airport
      copyAirportColumns.append(COM_COLUMNS);
  }

  if(prevHasHelipad)
  {
    removeOrUpdate(deleteHelipadStmt, updateHelipadStmt, bgl::del::HELIPADS);

    if(!isFlagSet(deleteFlags, bgl::del::HELIPADS) && hasPrevious)
      // Update helipad count in new airport
      copyAirportColumns.append("num_helipad");
  }

  if(prevHasTaxi)
  {
    removeOrUpdate(deleteTaxiPathStmt, updateTaxiPathStmt, bgl::del::TAXIWAYS);

    if(!isFlagSet(deleteFlags, bgl::del::TAXIWAYS) && hasPrevious)
      // Update taxi count in new airport
      copyAirportColumns.append("num_taxi_path");
  }

  if(prevHasStart)
  {
    removeOrUpdate(deleteStartStmt, updateStartStmt, bgl::del::STARTS);

    if(!isFlagSet(deleteFlags, bgl::del::STARTS) && hasPrevious)
      // Update start count in new airport
      copyAirportColumns.append("num_starts");
  }

  if(prevHasRunways)
  {
    if(isFlagSet(deleteFlags, bgl::del::RUNWAYS))
      removeRunways();
    else if(hasPrevious)
    {
      // Relink runways
      bindAndExecute(updateRunwayStmt, "runways updated");
      copyAirportColumns.append(RUNWAY_COLUMNS);
    }
  }

  if(!curAirport->getParkings().isEmpty())
    // New airport has parking - delete the previous ones
    bindAndExecute(deleteParkingStmt, "parking spots deleted");
  else if(hasPrevious)
  {
    // New airport has no parking - transfer previous ones and update counts
    bindAndExecute(updateParkingStmt, "parking spots updated");
    copyAirportColumns.append(AIRPORT_COLUMNS);
  }

  if(hasPrevious)
  {
    if(curAirport->getFuelFlags() == atools::fs::bgl::ap::NO_FUEL_FLAGS)
    {
      // Copy fuel flags from previous airport if this one doesn't have any
      for(const QString& col : FUEL_COLUMNS)
        copyAirportColumns.append(col);
    }

    // Update tower TODO not accurate
    if(!curAirport->hasTowerObj())
      copyAirportColumns.append("has_tower_object");

    if(curAirport->getTowerPosition().getAltitude() == 0.f)
      copyAirportColumns.append("tower_altitude");

    if(curAirport->getTowerPosition().getPos().isNull() || !curAirport->getTowerPosition().getPos().isValid())
    {
      copyAirportColumns.append("tower_lonx");
      copyAirportColumns.append("tower_laty");
    }

    if(curAirport->getMagVar() == 0.f)
      // TODO FSAD does not update magvar in their airports yet
      copyAirportColumns.append("mag_var");

    // Get the best rating
    int currentRating = std::max(curAirport->calculateRating(isAddon), previousRating);
    SqlQuery update(db);
    update.prepare("update airport set rating = :rating where airport_id = :apid");
    update.bindValue(":rating", currentRating);
    update.bindValue(":apid", curAirportId);
    update.exec();

    if(isAddon)
      // Previous was an addon - keep this state here, even if this airport is excluded
      copyAirportColumns.append("is_addon");
  }

  // Copy columns from previous airport to current airport
  copyAirportColumns.removeDuplicates();
  copyAirportValues(copyAirportColumns);

  // Airport has moved more than 500 meter from previous or has moved to a far position - update bounding rectangle for current airport
  if(hasPrevious && (curAirport->getPos().distanceMeterTo(prevPos) > 500.f || movedFar))
    updateBoundingRect();

  // Remove previous airport "delete from airport where airport_id = :prevApId"
  removePrevAirport();
}

void DeleteProcessor::updateBoundingRect()
{
  // Fetch min/max runway, taxipath, parking and other coordinates from current airport
  fetchBoundingStmt->bindValue(":apid", curAirportId);
  executeStatement(fetchBoundingStmt, "Fetch bounding");
  if(fetchBoundingStmt->next())
  {
    if(!fetchBoundingStmt->isNull("left_lonx") && !fetchBoundingStmt->isNull("top_laty") &&
       !fetchBoundingStmt->isNull("right_lonx") && !fetchBoundingStmt->isNull("bottom_laty"))
    {
      geo::Rect bounding(fetchBoundingStmt->value("left_lonx").toFloat(),
                         fetchBoundingStmt->value("top_laty").toFloat(),
                         fetchBoundingStmt->value("right_lonx").toFloat(),
                         fetchBoundingStmt->value("bottom_laty").toFloat());

      if(bounding.isValid())
      {
        bounding.extend(curAirport->getPos());

        // Check if rectangle exceeds 20 NM and convert to 500 meter rect if needed
        if(bounding.getHeightMeter() > atools::geo::nmToMeter(10) || bounding.getWidthMeter() > atools::geo::nmToMeter(10))
        {
          qDebug() << Q_FUNC_INFO << "Correcting bounding rectangle of" << curIdent << "bounding" << bounding;
          bounding = geo::Rect(curAirport->getPos(), 500.f, false /* fast */);
        }

#ifdef DEBUG_INFORMATION
        qDebug() << Q_FUNC_INFO << "ident" << curIdent << "currentAirportId" << curAirportId << "bounding" << bounding;
#endif

        // Update current airport
        updateBoundingStmt->bindValue(":apid", curAirportId);
        updateBoundingStmt->bindValue(":leftlonx", bounding.getWest());
        updateBoundingStmt->bindValue(":toplaty", bounding.getNorth());
        updateBoundingStmt->bindValue(":rightlonx", bounding.getEast());
        updateBoundingStmt->bindValue(":bottomlaty", bounding.getSouth());
        executeStatement(updateBoundingStmt, "Update bounding");
      }
    }
  }
  fetchBoundingStmt->finish();
}

void DeleteProcessor::removeRunways()
{
  QList<int> runwayEndIds;
  fetchRunwayEndIdStmt->bindValue(":prevApId", prevAirportId);
  fetchIds(fetchRunwayEndIdStmt, runwayEndIds, " runway ends to delete");

  // Delete runway first due to foreign key from rw -> rw end
  bindAndExecute(deleteRunwayStmt, "runways deleted");

  for(int it : qAsConst(runwayEndIds))
  {
    // Remove runway
    deleteRunwayEndStmt->bindValue(":endId", it);
    executeStatement(deleteRunwayEndStmt, "runway ends deleted");
  }
}

void DeleteProcessor::removePrevAirport()
{
  // Unlink navigation - will be updated later in "update_nav_ids.sql" script
  // we accecpt duplicates here - these will be deleted later
  bindAndExecute(updateWpStmt, "waypoints updated");
  bindAndExecute(updateVorStmt, "vors updated");
  bindAndExecute(updateNdbStmt, "ndb updated");

  int deleted = bindAndExecute(deleteAirportStmt, "airports deleted");
  if(deleted > 1)
    qWarning() << "Removed more than one airport" << deleted;
}

int DeleteProcessor::executeStatement(SqlQuery *stmt, const QString& what)
{
  stmt->exec();
  int retval = stmt->numRowsAffected();

  if(options.isVerbose())
    if(retval > 0)
      qDebug() << retval << " " << what /* << "bound" << stmt->boundValues()*/;

  return retval > 0 ? retval : 0;
}

void DeleteProcessor::fetchIds(SqlQuery *stmt, QList<int>& ids, const QString& what)
{
  stmt->exec();
  while(stmt->next())
    ids.append(stmt->value(0).toInt());

  if(options.isVerbose())
    qDebug() << ids.size() << " " << what /*<< "bound" << stmt->boundValues()*/;
}

/* use the remove of update query for a feture depending on the delete flag */
void DeleteProcessor::removeOrUpdate(SqlQuery *deleteStmt, SqlQuery *updateStmt, bgl::del::DeleteAllFlags flag)
{
  QString delTypeStr = bgl::DeleteAirport::deleteAllFlagsToStr(flag).toLower();

  if(isFlagSet(deleteFlags, flag))
    bindAndExecute(deleteStmt, delTypeStr + " deleted");
  else
    bindAndExecute(updateStmt, delTypeStr + " updated");
}

/* Create a statement that sets all airport_id columns to null in the given table that have
 * an airport_id that belongs to the other airports */
QString DeleteProcessor::updateAptFeatureToNullStmt(const QString& table)
{
  return "update " + table + " set airport_id = null where airport_id = :prevApId";
}

/* Create a statement that updates all airport_id columns in the given table that have
 * an airport_id that belongs to the other airports */
QString DeleteProcessor::updateAptFeatureStmt(const QString& table)
{
  return "update " + table + " set airport_id = :curApId where airport_id = :prevApId";
}

/* Create a statement that deletes all rows in the given table that have an airport_id
 * that belongs to the other airports */
QString DeleteProcessor::delAptFeatureStmt(const QString& table)
{
  return "delete from " + table + " where airport_id = :prevApId";
}

int DeleteProcessor::bindAndExecute(const QString& sql, const QString& msg)
{
  SqlQuery query(db);
  query.prepare(sql);
  return bindAndExecute(&query, msg);
}

int DeleteProcessor::bindAndExecute(SqlQuery *query, const QString& msg)
{
  if(query->hasPlaceholder(":prevApId"))
    query->bindValue(":prevApId", prevAirportId);
  if(query->hasPlaceholder(":curApId"))
    query->bindValue(":curApId", curAirportId);
  if(query->hasPlaceholder(":apIdent"))
    query->bindValue(":apIdent", curIdent);

  return executeStatement(query, msg);
}

void DeleteProcessor::extractDeleteFlags()
{
  if(deleteAirport != nullptr)
  {
    deleteFlags = deleteAirport->getFlags();
    // qDebug() << "processDelete Flags from delete record" << deleteFlags;
  }
  else
  {
    // Transfer all from the old airport if not delete record is given

    // The airport is an addon but there is no delete record
    // Check what is included an overwrite the old one
    // if(!newAirport->getApproaches().isEmpty())
    // deleteFlags |= bgl::del::APPROACHES;
    // if(!newAirport->getAprons().isEmpty())
    // deleteFlags |= bgl::del::APRONS;
    // if(!newAirport->getComs().isEmpty())
    // deleteFlags |= bgl::del::COMS;
    // if(!newAirport->getHelipads().isEmpty())
    // deleteFlags |= bgl::del::HELIPADS;
    // if(!newAirport->getTaxiPaths().isEmpty())
    // deleteFlags |= bgl::del::TAXIWAYS;
    // if(!newAirport->getRunways().isEmpty())
    // deleteFlags |= bgl::del::RUNWAYS;
    // qDebug() << "processDelete Made up flags" << deleteFlags;
  }

  if(options.getSimulatorType() != atools::fs::FsPaths::MSFS)
  {
    // Do not delete anything if the new airport has no corresponding features
    if(curAirport->getApproaches().isEmpty())
      deleteFlags &= ~bgl::del::APPROACHES;

    // if(newAirport->getAprons().isEmpty())
    // deleteFlags &= ~bgl::del::APRONS;

    if(curAirport->getComs().isEmpty())
      deleteFlags &= ~bgl::del::COMS;

    if(curAirport->getHelipads().isEmpty())
      deleteFlags &= ~bgl::del::HELIPADS;

    if(curAirport->getTaxiPaths().isEmpty())
      deleteFlags &= ~bgl::del::TAXIWAYS;

    if(curAirport->getRunways().isEmpty())
      deleteFlags &= ~bgl::del::RUNWAYS;
  }
}

void DeleteProcessor::extractPreviousAirportFeatures()
{
  bindAndExecute(selectAirportStmt, "select airports");

  prevHasApproach = false;
  prevHasApron = false;
  prevHasCom = false;
  prevHasHelipad = false;
  prevHasTaxi = false;
  prevHasRunways = false;
  prevHasStart = false;
  isAddon = false;
  previousRating = 0;
  hasPrevious = false;
  prevAirportId = -1;
  sceneryLocalPath.clear();
  bglFilename.clear();
  prevPos = atools::geo::Pos();
  prevName.clear();
  prevCity.clear();
  prevState.clear();
  prevCountry.clear();
  prevRegion.clear();

  if(selectAirportStmt->next())
  {
    prevHasApproach |= selectAirportStmt->valueInt("num_approach") > 0;
    prevHasApron |= selectAirportStmt->valueInt("num_apron") > 0;
    prevHasCom |= selectAirportStmt->valueInt("num_com") > 0;
    prevHasHelipad |= selectAirportStmt->valueInt("num_helipad") > 0;
    prevHasTaxi |= selectAirportStmt->valueInt("num_taxi_path") > 0;
    prevHasRunways |= selectAirportStmt->valueInt("num_runways") > 0;
    prevHasStart |= selectAirportStmt->valueInt("num_starts") > 0;
    isAddon |= selectAirportStmt->valueBool("is_addon");
    previousRating = std::max(previousRating, selectAirportStmt->valueInt("rating"));

    sceneryLocalPath = selectAirportStmt->valueStr("scenery_local_path");
    bglFilename = selectAirportStmt->valueStr("bgl_filename");

    prevPos = atools::geo::Pos(selectAirportStmt->valueFloat("lonx"), selectAirportStmt->valueFloat("laty"));

    prevAirportId = selectAirportStmt->valueInt("airport_id");

    prevName = selectAirportStmt->valueStr("name");
    prevCity = selectAirportStmt->valueStr("city");
    prevState = selectAirportStmt->valueStr("state");
    prevCountry = selectAirportStmt->valueStr("country");
    prevRegion = selectAirportStmt->valueStr("region");

    hasPrevious = true;

    // if(selectAirportStmt->valueInt("altitude") !=
    // atools::roundToInt(atools::geo::meterToFeet(newAirport->getPosition().getAltitude())))
    // qInfo().nospace().noquote()
    // << "Add-on airport altitude for "
    // << newAirport->getIdent()
    // << " changed from " << selectAirportStmt->valueInt("altitude") << " ft"
    // << " (BGL " << sceneryLocalPath << "/" << bglFilename << ")"
    // << " to " << atools::roundToInt(atools::geo::meterToFeet(newAirport->getPosition().getAltitude()))
    // << " ft";
  }
  selectAirportStmt->finish();
}

void DeleteProcessor::copyAirportValues(const QStringList& copyAirportColumns)
{
  if(!copyAirportColumns.isEmpty())
  {
    // Select values from previous/old airport
    SqlQuery query(db), insert(db);
    query.prepare("select " + copyAirportColumns.join(",") + " from airport where airport_id = :prevApId");
    query.bindValue(":prevApId", prevAirportId);
    query.exec();

    QStringList bindCols(copyAirportColumns);
    for(QString& bindCol : bindCols)
      bindCol = bindCol + " = :" + bindCol;

    if(query.next())
    {
      // Assign values to this current/new airport
      insert.prepare("update airport set " + bindCols.join(", ") + " where airport_id = :aptid");
      insert.bindValue(":aptid", curAirportId);
      SqlUtil::copyRowValues(query, insert);
      insert.exec();
      if(insert.numRowsAffected() <= 0)
        qWarning() << "Noting inserted for airport update" << curIdent << curAirportId;
    }
    query.finish();
  }
}

} // namespace writer
} // namespace fs
} // namespace atools
