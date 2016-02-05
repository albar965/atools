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

#include "fs/writer/ap/deleteprocessor.h"
#include "fs/writer/ap/approachwriter.h"
#include "sql/sqldatabase.h"
#include "sql/sqlquery.h"
#include "fs/writer/datawriter.h"
#include "fs/bgl/ap/del/deleteairport.h"
#include "fs/bgl/util.h"
#include "fs/bgl/ap/airport.h"
#include "fs/writer/ap/transitionwriter.h"
#include "fs/bglreaderoptions.h"

#include <functional>
#include "logging/loggingdefs.h"

namespace atools {
namespace fs {
namespace writer {

using atools::fs::bgl::DeleteAirport;
using atools::fs::bgl::Airport;
using atools::sql::SqlQuery;
using bgl::util::isFlagSet;

DeleteProcessor::DeleteProcessor(atools::sql::SqlDatabase& sqlDb, DataWriter& writer)
  : dataWriter(writer), db(sqlDb)
{
  deletePrimaryApproachStmt = new SqlQuery(sqlDb);
  deletePrimaryApproachLegStmt = new SqlQuery(sqlDb);
  deleteSecondaryApproachStmt = new SqlQuery(sqlDb);
  deleteSecondaryApproachLegStmt = new SqlQuery(sqlDb);
  deletePrimaryTransitionStmt = new SqlQuery(sqlDb);
  deletePrimaryTransitionLegStmt = new SqlQuery(sqlDb);
  deleteSecondaryTransitionStmt = new SqlQuery(sqlDb);
  deleteSecondaryTransitionLegStmt = new SqlQuery(sqlDb);

  deleteRunwayStmt = new SqlQuery(sqlDb);
  deleteParkingStmt = new SqlQuery(sqlDb);
  deleteDeleteApStmt = new SqlQuery(sqlDb);

  fetchRunwayEndIdStmt = new SqlQuery(sqlDb);
  deleteRunwayEndStmt = new SqlQuery(sqlDb);
  deleteIlsStmt = new SqlQuery(sqlDb);

  fetchPrimaryRunwayEndIdStmt = new SqlQuery(sqlDb);
  fetchSecondaryRunwayEndIdStmt = new SqlQuery(sqlDb);
  updateApprochRwIds = new SqlQuery(sqlDb);

  nullWpStmt = new SqlQuery(sqlDb);
  nullVorStmt = new SqlQuery(sqlDb);
  nullNdbStmt = new SqlQuery(sqlDb);
  deleteAirportStmt = new SqlQuery(sqlDb);
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
  deleteApproacheStmt = new SqlQuery(sqlDb);
  fetchOldApproachIdStmt = new SqlQuery(sqlDb);

  // Most query act on all airports with the given ident except the current one
  // (where a.ident = :apIdent and a.airport_id <> :curApId)

  // Define subqueries for features to delete
  QString primaryRwEnds("select e.runway_end_id "
                        "from airport a "
                        "join runway r on r.airport_id = a.airport_id "
                        "join runway_end e on r.primary_end_id = e.runway_end_id "
                        "where a.ident = :apIdent and a.airport_id <> :curApId");

  QString secondaryRwEnds("select e.runway_end_id "
                          "from airport a "
                          "join runway r on r.airport_id = a.airport_id "
                          "join runway_end e on r.secondary_end_id = e.runway_end_id "
                          "where a.ident = :apIdent and a.airport_id <> :curApId");

  QString primaryAppr("select app.approach_id "
                      "from airport a "
                      "join runway r on r.airport_id = a.airport_id "
                      "join runway_end e on r.primary_end_id = e.runway_end_id "
                      "join approach app on app.runway_end_id = e.runway_end_id "
                      "where a.ident = :apIdent and a.airport_id <> :curApId");

  QString secondaryAppr("select app.approach_id "
                        "from airport a "
                        "join runway r on r.airport_id = a.airport_id "
                        "join runway_end e on r.secondary_end_id = e.runway_end_id "
                        "join approach app on app.runway_end_id = e.runway_end_id "
                        "where a.ident = :apIdent and a.airport_id <> :curApId");

  QString primaryTrans("select tr.transition_id "
                       "from airport a  "
                       "join runway r on r.airport_id = a.airport_id  "
                       "join runway_end e on r.primary_end_id = e.runway_end_id  "
                       "join approach app on app.runway_end_id = e.runway_end_id  "
                       "join transition tr on app.approach_id = tr.approach_id  "
                       "where a.ident = :apIdent and a.airport_id <> :curApId");

  QString secondaryTrans("select tr.transition_id "
                         "from airport a  "
                         "join runway r on r.airport_id = a.airport_id  "
                         "join runway_end e on r.secondary_end_id = e.runway_end_id  "
                         "join approach app on app.runway_end_id = e.runway_end_id  "
                         "join transition tr on app.approach_id = tr.approach_id  "
                         "where a.ident = :apIdent and a.airport_id <> :curApId");

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

  // Delete all approaches on the primary runway ends of an airport
  deletePrimaryApproachStmt->prepare(
    "delete from approach where runway_end_id in (" + primaryRwEnds + ")");
  deletePrimaryApproachLegStmt->prepare(
    "delete from approach_leg where approach_id in (" + primaryAppr + ")");

  // Delete all approaches on the secondary runway ends of an airport
  deleteSecondaryApproachStmt->prepare(
    "delete from approach where runway_end_id in (" + secondaryRwEnds + ")");
  deleteSecondaryApproachLegStmt->prepare(
    "delete from approach_leg where approach_id in (" + secondaryAppr + ")");

  // Delete all transitions on the primary runway ends of an airport
  deletePrimaryTransitionStmt->prepare(
    "delete from transition where approach_id in (" + primaryAppr + ")");
  deletePrimaryTransitionLegStmt->prepare(
    "delete from transition_leg where transition_id in (" + primaryTrans + ")");

  // Delete all transitions on the secondary runway ends of an airport
  deleteSecondaryTransitionStmt->prepare(
    "delete from transition where approach_id in (" + secondaryAppr + ")");
  deleteSecondaryTransitionLegStmt->prepare(
    "delete from transition_leg where transition_id in (" + secondaryTrans + ")");

  // Delete all runways of an airport
  deleteRunwayStmt->prepare(
    "delete from runway where airport_id in ( "
    "select a.airport_id "
    "from airport a "
    "where a.ident = :apIdent and a.airport_id <> :curApId)");

  // Get all primary and secondary end ids for the given airports
  fetchRunwayEndIdStmt->prepare(
    "select r.primary_end_id as runway_end_id "
    "from airport a join runway r on a.airport_id = r.airport_id "
    "where a.ident = :apIdent and a.airport_id <> :curApId "
    "union "
    "select r.secondary_end_id as runway_end_id "
    "from airport a join runway r on a.airport_id = r.airport_id "
    "where a.ident = :apIdent and a.airport_id <> :curApId");

  fetchPrimaryRunwayEndIdStmt->prepare(fetchOldAndNewRwIds.arg("primary"));
  fetchSecondaryRunwayEndIdStmt->prepare(fetchOldAndNewRwIds.arg("secondary"));

  deleteRunwayEndStmt->prepare(
    "delete from runway_end where runway_end_id = :endId");

  deleteIlsStmt->prepare(
    "delete from ils where loc_runway_end_id = :endId");

  deleteAirportStmt->prepare(
    "delete from airport "
    "where ident = :apIdent and airport_id <> :curApId");

  deleteComStmt->prepare(delAptFeatureStmt("com"));
  deleteHelipadStmt->prepare(delAptFeatureStmt("helipad"));
  deleteStartStmt->prepare(delAptFeatureStmt("start"));
  deleteParkingStmt->prepare(delAptFeatureStmt("parking"));
  deleteApronStmt->prepare(delAptFeatureStmt("apron"));
  deleteApronLightStmt->prepare(delAptFeatureStmt("apron_light"));
  deleteFenceStmt->prepare(delAptFeatureStmt("fence"));
  deleteTaxiPathStmt->prepare(delAptFeatureStmt("taxi_path"));
  deleteDeleteApStmt->prepare(delAptFeatureStmt("delete_airport"));

  // TODO better erase waypoints
  nullWpStmt->prepare(updateAptFeatureToNullStmt("waypoint"));
  nullVorStmt->prepare(updateAptFeatureToNullStmt("vor"));
  nullNdbStmt->prepare(updateAptFeatureToNullStmt("ndb"));

  updateComStmt->prepare(updateAptFeatureStmt("com"));
  updateHelipadStmt->prepare(updateAptFeatureStmt("helipad"));
  updateStartStmt->prepare(updateAptFeatureStmt("start"));
  updateApronStmt->prepare(updateAptFeatureStmt("apron"));
  updateApronLightStmt->prepare(updateAptFeatureStmt("apron_light"));
  updateTaxiPathStmt->prepare(updateAptFeatureStmt("taxi_path"));

  updateApprochRwIds->prepare("update approach set runway_end_id = :newRwId where runway_end_id = :oldRwId");

  deleteTransitionLegStmt->prepare("delete from transition_leg "
                                   "where transition_id in "
                                   "(select transition_id from transition where approach_id = :id)");
  deleteApproachLegStmt->prepare("delete from approach_leg where approach_id = :id");
  deleteTransitionStmt->prepare("delete from transition where approach_id = :id");
  deleteApproacheStmt->prepare("delete from approach where approach_id = :id");
  fetchOldApproachIdStmt->prepare(
    "select app.approach_id as primary_app_id, app2.approach_id as secondary_app_id "
    "from airport a  "
    "join airport ao on a.ident = ao.ident "
    "join runway r on r.airport_id = a.airport_id  "
    "join approach app on app.runway_end_id = r.primary_end_id  "
    "join approach app2 on app2.runway_end_id = r.secondary_end_id  "
    "where a.ident = :apIdent and a.airport_id <> :curApId ");

}

DeleteProcessor::~DeleteProcessor()
{
  delete deletePrimaryApproachStmt;
  delete deletePrimaryApproachLegStmt;
  delete deleteSecondaryApproachStmt;
  delete deleteSecondaryApproachLegStmt;
  delete deletePrimaryTransitionStmt;
  delete deletePrimaryTransitionLegStmt;
  delete deleteSecondaryTransitionStmt;
  delete deleteSecondaryTransitionLegStmt;

  delete deleteRunwayStmt;
  delete deleteParkingStmt;
  delete deleteDeleteApStmt;
  delete fetchRunwayEndIdStmt;
  delete deleteRunwayEndStmt;
  delete deleteIlsStmt;

  delete fetchPrimaryRunwayEndIdStmt;
  delete fetchSecondaryRunwayEndIdStmt;
  delete updateApprochRwIds;

  delete nullWpStmt;
  delete nullVorStmt;
  delete nullNdbStmt;

  delete deleteAirportStmt;
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
  delete deleteApproacheStmt;
  delete fetchOldApproachIdStmt;
}

void DeleteProcessor::processDelete(const DeleteAirport *delAp, const Airport *airport, int currentApId)
{
  this->del = delAp;
  this->type = airport;
  this->currentId = currentApId;

  ident = type->getIdent();

  if(isFlagSet(del->getFlags(), bgl::del::APPROACHES))
    deleteApproaches();
  else
    transferApproaches();

  deleteOrUpdate(deleteApronLightStmt, updateApronLightStmt, bgl::del::APRONLIGHTS);
  deleteOrUpdate(deleteApronStmt, updateApronStmt, bgl::del::APRONS);
  deleteOrUpdate(deleteComStmt, updateComStmt, bgl::del::COMS);
  deleteOrUpdate(deleteHelipadStmt, updateHelipadStmt, bgl::del::HELIPADS);
  deleteOrUpdate(deleteStartStmt, updateStartStmt, bgl::del::STARTS);
  deleteOrUpdate(deleteTaxiPathStmt, updateTaxiPathStmt, bgl::del::TAXIWAYS);

  if(isFlagSet(del->getFlags(), bgl::del::RUNWAYS))
    // TODO no update yet since it does not appear in reality
    deleteRunways();

  // TODO this keeps from updating airport features
  deleteAirport();
}

void DeleteProcessor::deleteApproaches()
{
  // Delete primary transitions
  bindAndExecute(deletePrimaryTransitionLegStmt, "primary transition legs deleted");
  bindAndExecute(deletePrimaryTransitionStmt, "primary transitions deleted");

  // Delete secondary transitions
  bindAndExecute(deleteSecondaryTransitionLegStmt, "secondary transition legs deleted");
  bindAndExecute(deleteSecondaryTransitionStmt, "secondary transitions deleted");

  // Delete primary approaches
  bindAndExecute(deletePrimaryApproachLegStmt, "secondary approach legs deleted");
  bindAndExecute(deletePrimaryApproachStmt, "secondary approaches deleted");

  // Delete secondary approaches
  bindAndExecute(deleteSecondaryApproachLegStmt, "secondary approach legs deleted");
  bindAndExecute(deleteSecondaryApproachStmt, "secondary approaches deleted");
}

void DeleteProcessor::deleteRunways()
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

void DeleteProcessor::deleteAirport()
{
  bindAndExecute(deleteParkingStmt, "parking spots deleted");
  bindAndExecute(deleteDeleteApStmt, "delete airports deleted");
  bindAndExecute(deleteFenceStmt, "fences deleted");

  // Unlink navigation - will be update later update_nav_ids.sql script
  // TODO this might result in duplicate waypoints
  bindAndExecute(nullWpStmt, "fences deleted"); // only type that appears in airport records
  bindAndExecute(nullVorStmt, "fences deleted");
  bindAndExecute(nullNdbStmt, "fences deleted");

  bindAndExecute(deleteAirportStmt, "airports deleted");
}

void DeleteProcessor::deleteApproachesAndTransitions(const QSet<int>& ids)
{
  for(int i : ids)
  {
    deleteTransitionLegStmt->bindValue(":id", i);
    executeStatement(deleteTransitionLegStmt, "transition_leg");

    deleteApproachLegStmt->bindValue(":id", i);
    executeStatement(deleteApproachLegStmt, "approach_leg");

    deleteTransitionStmt->bindValue(":id", i);
    executeStatement(deleteTransitionStmt, "transition");

    deleteApproacheStmt->bindValue(":id", i);
    executeStatement(deleteApproacheStmt, "approach");
  }
}

QSet<int> DeleteProcessor::fetchOldApproachIds()
{
  QSet<int> ids;
  bindAndExecute(fetchOldApproachIdStmt, "old approach ids");
  while(fetchOldApproachIdStmt->next())
  {
    ids.insert(fetchOldApproachIdStmt->value("primary_app_id").toInt());
    ids.insert(fetchOldApproachIdStmt->value("secondary_app_id").toInt());
  }
  return ids;
}

void DeleteProcessor::transferApproaches()
{
  QSet<int> ids = fetchOldApproachIds();

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

  deleteApproachesAndTransitions(ids);
}

void DeleteProcessor::executeStatement(SqlQuery *stmt, const QString& what)
{
  stmt->exec();
  int retval = stmt->numRowsAffected();

  if(dataWriter.getOptions().isVerbose())
    qDebug() << retval << " " << what;
}

void DeleteProcessor::fetchIds(SqlQuery *stmt, QList<int>& ids, const QString& what)
{
  stmt->exec();
  while(stmt->next())
    ids.push_back(stmt->value(0).toInt());

  if(dataWriter.getOptions().isVerbose())
    qDebug() << ids.size() << " " << what;
}

void DeleteProcessor::deleteOrUpdate(SqlQuery *deleteStmt,
                                     SqlQuery *updateStmt,
                                     bgl::del::DeleteAllFlags flag)
{
  QString delTypeStr = bgl::DeleteAirport::deleteAllFlagsToStr(flag).toLower();

  if(isFlagSet(del->getFlags(), flag))
    bindAndExecute(deleteStmt, delTypeStr + " deleted");
  else
    bindAndExecute(updateStmt, delTypeStr + " updated");
}

QString DeleteProcessor::updateAptFeatureToNullStmt(const QString& table)
{
  return "update " + table + " set airport_id = null where airport_id in ( "
                             "select a.airport_id "
                             "from airport a "
                             "where a.ident = :apIdent and a.airport_id <> :curApId)";

}

QString DeleteProcessor::updateAptFeatureStmt(const QString& table)
{
  return "update " + table + " set airport_id = :curApId where airport_id in ( "
                             "select a.airport_id "
                             "from airport a "
                             "where a.ident = :apIdent and a.airport_id <> :curApId)";

}

QString DeleteProcessor::delAptFeatureStmt(const QString& table)
{
  return "delete from " + table +
         " where airport_id in ( "
         "select a.airport_id from airport a "
         "where a.ident = :apIdent and a.airport_id <> :curApId)";

}

void DeleteProcessor::bindAndExecute(SqlQuery *delQuery, const QString& msg)
{
  delQuery->bindValue(":apIdent", ident);
  delQuery->bindValue(":curApId", currentId);
  executeStatement(delQuery, msg);
}

} // namespace writer
} // namespace fs
} // namespace atools
