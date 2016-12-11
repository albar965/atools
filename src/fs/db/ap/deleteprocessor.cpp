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

#include "fs/db/ap/deleteprocessor.h"
#include "fs/db/ap/approachwriter.h"
#include "sql/sqldatabase.h"
#include "sql/sqlquery.h"
#include "sql/sqlutil.h"
#include "fs/db/datawriter.h"
#include "fs/bgl/ap/del/deleteairport.h"
#include "fs/bgl/util.h"
#include "fs/bgl/ap/airport.h"
#include "fs/db/ap/transitionwriter.h"
#include "fs/navdatabaseoptions.h"

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
  deleteDeleteApStmt = new SqlQuery(sqlDb);

  fetchRunwayEndIdStmt = new SqlQuery(sqlDb);
  deleteRunwayEndStmt = new SqlQuery(sqlDb);
  deleteIlsStmt = new SqlQuery(sqlDb);

  fetchPrimaryRunwayEndIdStmt = new SqlQuery(sqlDb);
  fetchSecondaryRunwayEndIdStmt = new SqlQuery(sqlDb);
  updateApprochRwIds = new SqlQuery(sqlDb);
  updateApprochStmt = new SqlQuery(sqlDb);
  deleteApprochStmt = new SqlQuery(sqlDb);

  delWpStmt = new SqlQuery(sqlDb);
  updateVorStmt = new SqlQuery(sqlDb);
  updateNdbStmt = new SqlQuery(sqlDb);
  deleteAirportStmt = new SqlQuery(sqlDb);
  selectAirportStmt = new SqlQuery(sqlDb);
  deleteApronStmt = new SqlQuery(sqlDb);
  updateApronStmt = new SqlQuery(sqlDb);
  deleteApronLightStmt = new SqlQuery(sqlDb);
  updateApronLightStmt = new SqlQuery(sqlDb);
  deleteFenceStmt = new SqlQuery(sqlDb);
  updateFenceStmt = new SqlQuery(sqlDb);
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

  deleteTransitionLegStmt = new SqlQuery(sqlDb);
  deleteApproachLegStmt = new SqlQuery(sqlDb);
  deleteTransitionStmt = new SqlQuery(sqlDb);
  deleteApproachStmt = new SqlQuery(sqlDb);
  fetchOldApproachIdStmt = new SqlQuery(sqlDb);

  // Most queries act on all other airports with the given ident except the current one
  // where a.ident = :apIdent and a.airport_id <> :curApId

  // Define subqueries for features to delete

  // Delete all runways of all other airports
  deleteRunwayStmt->prepare(
    "delete from runway where airport_id in ( "
    "select a.airport_id from airport a "
    "where a.ident = :apIdent and a.airport_id <> :curApId)");

  // Delete all runways of all other airports
  updateRunwayStmt->prepare(updateAptFeatureStmt("runway"));

  // Get all primary and secondary end ids for the given airports
  fetchRunwayEndIdStmt->prepare(
    "select r.primary_end_id as runway_end_id "
    "from airport a join runway r on a.airport_id = r.airport_id "
    "where a.ident = :apIdent and a.airport_id <> :curApId "
    "union "
    "select r.secondary_end_id as runway_end_id "
    "from airport a join runway r on a.airport_id = r.airport_id "
    "where a.ident = :apIdent and a.airport_id <> :curApId");

  // Query to get old and new matching runway end ids - one for primary and secondary ends
  QString fetchOldAndNewRwIds("select "
                              "e.runway_end_id as old_runway_end_id, "
                              "eo.runway_end_id as new_runway_end_id "
                              "from airport a "
                              "join airport ao on a.ident = ao.ident "
                              "join runway r on r.airport_id = a.airport_id "
                              "join runway_end e on r.%1_end_id = e.runway_end_id "
                              "join runway ro on ro.airport_id = ao.airport_id "
                              "join runway_end eo on ro.%1_end_id = eo.runway_end_id "
                              "where a.ident = :apIdent and a.airport_id <> :curApId and "
                              "ao.ident = :apIdent and ao.airport_id == :curApId and "
                              "e.name = eo.name");
  fetchPrimaryRunwayEndIdStmt->prepare(fetchOldAndNewRwIds.arg("primary"));
  fetchSecondaryRunwayEndIdStmt->prepare(fetchOldAndNewRwIds.arg("secondary"));

  // Delete a runway end
  deleteRunwayEndStmt->prepare("delete from runway_end where runway_end_id = :endId");

  // Delete the ILS for a runway end
  deleteIlsStmt->prepare("delete from ils where loc_runway_end_id = :endId");

  // Delete all other airports
  deleteAirportStmt->prepare("delete from airport where ident = :apIdent and airport_id <> :curApId");

  // Get facility counts for all other airports to save some empty queries
  selectAirportStmt->prepare(
    "select airport_id, num_apron, num_com, num_helipad, num_taxi_path, num_runways, "
    "num_runway_end_ils, num_approach, is_addon, rating "
    "from airport where ident = :apIdent and airport_id <> :curApId");

  // Delete all facilities of the old airport
  deleteComStmt->prepare(delAptFeatureStmt("com"));
  deleteHelipadStmt->prepare(delAptFeatureStmt("helipad"));
  deleteStartStmt->prepare(delAptFeatureStmt("start"));
  deleteParkingStmt->prepare(delAptFeatureStmt("parking"));
  updateParkingStmt->prepare(updateAptFeatureStmt("parking"));
  deleteApronStmt->prepare(delAptFeatureStmt("apron"));
  deleteApronLightStmt->prepare(delAptFeatureStmt("apron_light"));
  deleteFenceStmt->prepare(delAptFeatureStmt("fence"));
  updateFenceStmt->prepare(updateAptFeatureStmt("fence"));
  deleteTaxiPathStmt->prepare(delAptFeatureStmt("taxi_path"));
  deleteDeleteApStmt->prepare(delAptFeatureStmt("delete_airport"));

  delWpStmt->prepare(delAptFeatureStmt("waypoint"));

  // Update NDB and VOR airport ids - resulting duplicates will be deleted later
  updateVorStmt->prepare(updateAptFeatureStmt("vor"));
  updateNdbStmt->prepare(updateAptFeatureStmt("ndb"));

  // Update facilities with new airport id
  updateComStmt->prepare(updateAptFeatureStmt("com"));
  updateHelipadStmt->prepare(updateAptFeatureStmt("helipad"));
  updateStartStmt->prepare(updateAptFeatureStmt("start"));
  updateApronStmt->prepare(updateAptFeatureStmt("apron"));
  updateApronLightStmt->prepare(updateAptFeatureStmt("apron_light"));
  updateTaxiPathStmt->prepare(updateAptFeatureStmt("taxi_path"));

  // Relink approach (and everything dependent) to a new airport
  updateApprochRwIds->prepare("update approach set runway_end_id = :newRwId where runway_end_id = :oldRwId");
  updateApprochStmt->prepare(updateAptFeatureStmt("approach"));
  deleteApprochStmt->prepare(delAptFeatureStmt("approach"));

  deleteTransitionLegStmt->prepare("delete from transition_leg where transition_id in "
                                   "(select transition_id from transition where approach_id = :id)");
  deleteApproachLegStmt->prepare("delete from approach_leg where approach_id = :id");
  deleteTransitionStmt->prepare("delete from transition where approach_id = :id");
  deleteApproachStmt->prepare("delete from approach where approach_id = :id");

  // Collect all approach_ids of the other airports
  fetchOldApproachIdStmt->prepare("select app.approach_id from airport a "
                                  "join approach app on app.airport_id = a.airport_id "
                                  "where a.ident = :apIdent and a.airport_id <> :curApId ");
}

DeleteProcessor::~DeleteProcessor()
{
  delete deleteRunwayStmt;
  delete updateRunwayStmt;
  delete deleteParkingStmt;
  delete updateParkingStmt;
  delete deleteDeleteApStmt;
  delete fetchRunwayEndIdStmt;
  delete deleteRunwayEndStmt;
  delete deleteIlsStmt;
  delete fetchPrimaryRunwayEndIdStmt;
  delete fetchSecondaryRunwayEndIdStmt;
  delete updateApprochRwIds;
  delete updateApprochStmt;
  delete deleteApprochStmt;
  delete delWpStmt;
  delete updateVorStmt;
  delete updateNdbStmt;
  delete deleteAirportStmt;
  delete selectAirportStmt;
  delete deleteApronStmt;
  delete updateApronStmt;
  delete deleteApronLightStmt;
  delete updateApronLightStmt;
  delete deleteFenceStmt;
  delete updateFenceStmt;
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
  delete deleteTransitionLegStmt;
  delete deleteApproachLegStmt;
  delete deleteTransitionStmt;
  delete deleteApproachStmt;
  delete fetchOldApproachIdStmt;
}

void DeleteProcessor::processDelete(const DeleteAirport *deleteAirportRec, const Airport *airport,
                                    int currentAirportId)
{
  qInfo() << "processDelete" << airport->getIdent() << "current id" << currentAirportId;

  deleteAirport = deleteAirportRec;
  type = airport;
  currentId = currentAirportId;
  ident = type->getIdent();

  // Get facility counts for current airport
  bindAndExecute(selectAirportStmt, "select airports");

  bool hasApproach = false, hasApron = false, hasCom = false, hasHelipad = false, hasTaxi = false,
       hasRunways = false, isAddon = false;
  int rating = 0;

  bool hasPrevious = false;
  int i = 0;
  while(selectAirportStmt->next())
  {
    if(i > 0)
    {
      // If we get more than one entry set everything to true just to be safe
      qWarning() << "Found more than one airport to delete for ident"
                 << type->getIdent() << "id" << currentId
                 << "found" << selectAirportStmt->value("airport_id").toInt();
    }
    hasApproach |= selectAirportStmt->value("num_approach").toInt() > 0;
    hasApron |= selectAirportStmt->value("num_apron").toInt() > 0;
    hasCom |= selectAirportStmt->value("num_com").toInt() > 0;
    hasHelipad |= selectAirportStmt->value("num_helipad").toInt() > 0;
    hasTaxi |= selectAirportStmt->value("num_taxi_path").toInt() > 0;
    hasRunways |= selectAirportStmt->value("num_runways").toInt() > 0;
    isAddon |= selectAirportStmt->value("is_addon").toBool();
    rating = std::max(rating, selectAirportStmt->value("rating").toInt());
    i++;
    hasPrevious = true;
  }

  if(hasApproach)
  {
    if(isFlagSet(deleteAirport->getFlags(), bgl::del::APPROACHES))
      // Delete the whole tree of apporaches, transitions and legs
      removeApproachesAndTransitions(fetchOldApproachIds());
    else if(hasPrevious)
    {
      // Relink the approach to the new airport and update the cound on the airport
      transferApproaches();
      bindAndExecute(copyFeatureStmt("airport", "num_approach"), "copied airport num_approach");
    }
  }

  // Work on facilities that will be either removed or attached to the new airport depending on flags
  if(hasApron)
  {
    removeOrUpdate(deleteApronLightStmt, updateApronLightStmt, bgl::del::APRONLIGHTS);
    removeOrUpdate(deleteApronStmt, updateApronStmt, bgl::del::APRONS);

    if(!isFlagSet(deleteAirport->getFlags(), bgl::del::APRONS) && hasPrevious)
      // Update apron count in new airport
      bindAndExecute(copyFeatureStmt("airport", "num_apron"), "copied airport num_apron");
  }

  if(hasCom)
  {
    removeOrUpdate(deleteComStmt, updateComStmt, bgl::del::COMS);

    if(!isFlagSet(deleteAirport->getFlags(), bgl::del::COMS) && hasPrevious)
    {
      // Copy all frequencies to the new airport
      QStringList cols;
      cols << "tower_frequency" << "atis_frequency" << "awos_frequency" << "asos_frequency"
           << "unicom_frequency" << "num_com";

      for(const QString& col : cols)
        bindAndExecute(copyFeatureStmt("airport", col), "copied airport " + col);
    }
  }

  if(hasHelipad)
  {
    removeOrUpdate(deleteHelipadStmt, updateHelipadStmt, bgl::del::HELIPADS);

    if(!isFlagSet(deleteAirport->getFlags(), bgl::del::HELIPADS) && hasPrevious)
      // Update helipad count in new airport
      bindAndExecute(copyFeatureStmt("airport", "num_helipad"), "copied airport num_helipad");
  }

  if(hasTaxi)
  {
    removeOrUpdate(deleteTaxiPathStmt, updateTaxiPathStmt, bgl::del::TAXIWAYS);

    if(!isFlagSet(deleteAirport->getFlags(), bgl::del::TAXIWAYS) && hasPrevious)
      // Update taxi count in new airport
      bindAndExecute(copyFeatureStmt("airport", "num_taxi_path"), "copied airport num_taxi_path");
  }

  removeOrUpdate(deleteStartStmt, updateStartStmt, bgl::del::STARTS);

  if(hasRunways)
  {
    if(isFlagSet(deleteAirport->getFlags(), bgl::del::RUNWAYS))
      removeRunways();
    else if(hasPrevious)
      // Relink runways
      updateRunways();
  }

  if(!airport->getParkings().isEmpty())
    // New airport has parking - delete the previous ones
    bindAndExecute(deleteParkingStmt, "parking spots deleted");
  else if(hasPrevious)
  {
    // New airport has no parking - transfer previous ones and update counts
    bindAndExecute(updateParkingStmt, "parking spots updated");

    QStringList cols;
    cols << "num_parking_gate" << "num_parking_ga_ramp" << "num_parking_cargo" << "num_parking_mil_cargo"
         << "num_parking_mil_combat" << "num_jetway" << "largest_parking_ramp" << "largest_parking_gate";

    for(const QString& col : cols)
      bindAndExecute(copyFeatureStmt("airport", col), "copied airport " + col);
  }

  if(!airport->getFences().isEmpty())
    // New airport has fences - delete the previous ones
    bindAndExecute(deleteFenceStmt, "fences deleted");
  else if(hasPrevious)
  {
    // New airport has no fences - transfer previous ones and update counts
    bindAndExecute(updateFenceStmt, "fences updated");
    bindAndExecute(copyFeatureStmt("airport", "num_boundary_fence"), "copied airport num_boundary_fence");
  }

  if(hasPrevious)
  {
    if(airport->getFuelFlags() == atools::fs::bgl::ap::NO_FUEL_FLAGS)
    {
      // Copy fuel flags from previous airport if this one doesn't have any
      QStringList cols;
      cols << "fuel_flags" << "has_avgas" << "has_jetfuel";

      for(const QString& col : cols)
        bindAndExecute(copyFeatureStmt("airport", col), "copied airport " + col);
    }

    // Update tower TODO not accurate
    if(!airport->hasTowerObj())
      bindAndExecute(copyFeatureStmt("airport", "has_tower_object"), "copied airport has_tower_object");

    if(airport->getTowerPosition().getAltitude() == 0.f)
      bindAndExecute(copyFeatureStmt("airport", "tower_altitude"), "copied airport tower_altitude");

    if(airport->getTowerPosition().getPos().isNull() || !airport->getTowerPosition().getPos().isValid())
    {
      bindAndExecute(copyFeatureStmt("airport", "tower_lonx"), "copied airport tower_lonx");
      bindAndExecute(copyFeatureStmt("airport", "tower_laty"), "copied airport tower_laty");
    }

    if(airport->getMagVar() == 0.f)
      // TODO FSAD does not update magvar in their airports yet
      bindAndExecute(copyFeatureStmt("airport", "mag_var"), "copied airport mag_var");

    int previousRating = airport->calculateRating(isAddon);
    if(previousRating > rating)
      // Copy rating only if this is worse
      bindAndExecute(copyFeatureStmt("airport", "rating"), "copied airport rating");

    if(isAddon)
    {
      // Previous was an addon - keep this state here, even if this airport is excluded
      bindAndExecute(copyFeatureStmt("airport", "is_addon"), "copied airport is_addon");
    }
  }

  bindAndExecute(deleteDeleteApStmt, "delete airports deleted");

  removeAirport();
}

void DeleteProcessor::updateRunways()
{
  bindAndExecute(updateRunwayStmt, "runways updated");

  QStringList cols;
  cols << "is_closed" << "num_runway_hard" << "num_runway_soft" << "num_runway_water" <<
  "num_runway_light" << "num_runway_end_closed" << "num_runway_end_vasi" << "num_runway_end_als" <<
  "num_runway_end_ils" << "longest_runway_length" << "longest_runway_width" <<
  "longest_runway_heading" << "longest_runway_surface" << "num_runways" << "left_lonx" << "top_laty" <<
  "right_lonx" << "bottom_laty";

  for(const QString& col : cols)
    bindAndExecute(copyFeatureStmt("airport", col), "copied airport " + col);
}

void DeleteProcessor::removeRunways()
{
  QList<int> runwayEndIds;
  fetchRunwayEndIdStmt->bindValue(":apIdent", ident);
  fetchRunwayEndIdStmt->bindValue(":curApId", currentId);
  fetchIds(fetchRunwayEndIdStmt, runwayEndIds, " runway ends to delete");

  // Delete runway first due to foreign key from rw -> rw end
  bindAndExecute(deleteRunwayStmt, "runways deleted");

  for(int it : runwayEndIds)
  {
    // Delete the ILS too
    deleteIlsStmt->bindValue(":endId", it);
    executeStatement(deleteIlsStmt, "ils deleted");

    // Remove runway
    deleteRunwayEndStmt->bindValue(":endId", it);
    executeStatement(deleteRunwayEndStmt, "runway ends deleted");
  }
}

void DeleteProcessor::removeAirport()
{
  // Unlink navigation - will be updated later in "update_nav_ids.sql" script
  // we accecpt duplicates here - these will be deleted later
  bindAndExecute(delWpStmt, "waypoints deleted"); // only type that appears in airport records

  bindAndExecute(updateVorStmt, "vors updated");
  bindAndExecute(updateNdbStmt, "ndb updated");

  bindAndExecute(deleteAirportStmt, "airports deleted");
}

void DeleteProcessor::removeApproachesAndTransitions(const QList<int>& ids)
{
  for(int i : ids)
  {
    deleteTransitionLegStmt->bindValue(":id", i);
    executeStatement(deleteTransitionLegStmt, "transition_leg deleted");

    deleteApproachLegStmt->bindValue(":id", i);
    executeStatement(deleteApproachLegStmt, "approach_leg deleted");

    deleteTransitionStmt->bindValue(":id", i);
    executeStatement(deleteTransitionStmt, "transition deleted");

    deleteApproachStmt->bindValue(":id", i);
    executeStatement(deleteApproachStmt, "approach deleted");
  }
}

/* Get a list of all old/other approach ids */
QList<int> DeleteProcessor::fetchOldApproachIds()
{
  QList<int> ids;
  bindAndExecute(fetchOldApproachIdStmt, "old approach ids");
  while(fetchOldApproachIdStmt->next())
    ids.append(fetchOldApproachIdStmt->value(0).toInt());
  return ids;
}

void DeleteProcessor::transferApproaches()
{
  QList<int> ids = fetchOldApproachIds();

  bindAndExecute(fetchPrimaryRunwayEndIdStmt, "fetched old and new primary runway end ids");
  while(fetchPrimaryRunwayEndIdStmt->next())
  {
    updateApprochRwIds->bindValue(":newRwId", fetchPrimaryRunwayEndIdStmt->value("new_runway_end_id").toInt());
    updateApprochRwIds->bindValue(":oldRwId", fetchPrimaryRunwayEndIdStmt->value("old_runway_end_id").toInt());
    executeStatement(updateApprochRwIds, "update approach primary runway end ids");
  }

  bindAndExecute(fetchSecondaryRunwayEndIdStmt, "fetched old and new secondary runway end ids");
  while(fetchSecondaryRunwayEndIdStmt->next())
  {
    updateApprochRwIds->bindValue(":newRwId", fetchSecondaryRunwayEndIdStmt->value("new_runway_end_id").toInt());
    updateApprochRwIds->bindValue(":oldRwId", fetchSecondaryRunwayEndIdStmt->value("old_runway_end_id").toInt());
    executeStatement(updateApprochRwIds, "update approach secondary runway end ids");
  }

  bindAndExecute(updateApprochStmt, "approaches updated");

  // Delete all leftovers of the old runways
  removeApproachesAndTransitions(ids);
}

void DeleteProcessor::executeStatement(SqlQuery *stmt, const QString& what)
{
  stmt->exec();
  int retval = stmt->numRowsAffected();

  // if(options.isVerbose())
  if(retval > 0)
    qDebug() << retval << " " << what /* << "bound" << stmt->boundValues()*/;
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
void DeleteProcessor::removeOrUpdate(SqlQuery *deleteStmt, SqlQuery *updateStmt,
                                     bgl::del::DeleteAllFlags flag)
{
  QString delTypeStr = bgl::DeleteAirport::deleteAllFlagsToStr(flag).toLower();

  if(isFlagSet(deleteAirport->getFlags(), flag))
    bindAndExecute(deleteStmt, delTypeStr + " deleted");
  else
    bindAndExecute(updateStmt, delTypeStr + " updated");
}

/* Create a statement that sets all airport_id columns to null in the given table that have
 * an airport_id that belongs to the other airports */
QString DeleteProcessor::updateAptFeatureToNullStmt(const QString& table)
{
  return "update " + table + " set airport_id = null where airport_id in ( "
                             "select a.airport_id "
                             "from airport a "
                             "where a.ident = :apIdent and a.airport_id <> :curApId)";

}

/* Create a statement that updates all airport_id columns in the given table that have
 * an airport_id that belongs to the other airports */
QString DeleteProcessor::updateAptFeatureStmt(const QString& table)
{
  return "update " + table + " set airport_id = :curApId where airport_id in ( "
                             "select a.airport_id "
                             "from airport a "
                             "where a.ident = :apIdent and a.airport_id <> :curApId)";
}

/* Create a statement that deletes all rows in the given table that have an airport_id
 * that belongs to the other airports */
QString DeleteProcessor::delAptFeatureStmt(const QString& table)
{
  return "delete from " + table +
         " where airport_id in ( "
         "select a.airport_id from airport a "
         "where a.ident = :apIdent and a.airport_id <> :curApId)";
}

/* Create a statement that copies features from the old table */
QString DeleteProcessor::copyFeatureStmt(const QString& table, const QString& column)
{
  return "update " + table +
         " set  " + column + " = " +
         " ( select a." + column +
         "   from " + table + " a " +
         "   where airport_id = a.airport_id and a.ident = :apIdent and a.airport_id <> :curApId) " +
         " where airport_id = :curApId";
}

void DeleteProcessor::bindAndExecute(SqlQuery *delQuery, const QString& msg)
{
  delQuery->bindValue(":apIdent", ident);
  delQuery->bindValue(":curApId", currentId);
  executeStatement(delQuery, msg);
}

void DeleteProcessor::bindAndExecute(const QString& sql, const QString& msg)
{
  SqlQuery query(db);
  query.prepare(sql);
  query.bindValue(":apIdent", ident);
  query.bindValue(":curApId", currentId);
  executeStatement(&query, msg);
}

} // namespace writer
} // namespace fs
} // namespace atools
