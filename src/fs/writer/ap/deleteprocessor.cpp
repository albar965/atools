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
#include "sql/sqlutil.h"
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
using atools::sql::SqlUtil;

template<class IDSOURCE>
class IdColFunc :
  public std::binary_function<SqlQuery&, SqlQuery&, bool>
{
public:
  IdColFunc(const QString& col, IDSOURCE *writer)
    : writer(writer), col(col), bindCol(":" + col)
  {
  }

  bool operator()(SqlQuery& from, SqlQuery& to)
  {
    int nextId = writer->getNextId();
    to.bindValue(bindCol, nextId);

    for(QPair<QString, int> iter : fixedColValues)
      to.bindValue(iter.first, iter.second);

    mappedIds.push_back(QPair<int, int>(from.value(col).toInt(), nextId));
    return true;
  }

  QList<QPair<int, int> > mappedIds;
  QList<QPair<QString, int> > fixedColValues;

private:
  IDSOURCE *writer;
  QString col;
  QString bindCol;
};

DeleteProcessor::DeleteProcessor(atools::sql::SqlDatabase& sqlDb, DataWriter& writer)
  : dataWriter(writer), db(sqlDb)
{
  deletePrimaryApproachStmt.prepare(
    "delete from approach where runway_end_id in ( "
    "select e.runway_end_id "
    "from airport a "
    "join runway r on r.airport_id = a.airport_id "
    "join runway_end e on r.primary_end_id = e.runway_end_id "
    "where a.ident = :apIdent and a.airport_id <> :curApId)");

  deletePrimaryTransitionStmt.prepare(
    "delete from transition where approach_id in ( "
    "select app.approach_id "
    "from airport a "
    "join runway r on r.airport_id = a.airport_id "
    "join runway_end e on r.primary_end_id = e.runway_end_id "
    "join approach app on app.runway_end_id = e.runway_end_id "
    "where a.ident = :apIdent and a.airport_id <> :curApId)");

  deleteSecondaryApproachStmt.prepare(
    "delete from approach where runway_end_id in ( "
    "select e.runway_end_id "
    "from airport a "
    "join runway r on r.airport_id = a.airport_id "
    "join runway_end e on r.secondary_end_id = e.runway_end_id "
    "where a.ident = :apIdent and a.airport_id <> :curApId)");

  deleteSecondaryTransitionStmt.prepare(
    "delete from transition where approach_id in ( "
    "select app.approach_id "
    "from airport a "
    "join runway r on r.airport_id = a.airport_id "
    "join runway_end e on r.secondary_end_id = e.runway_end_id "
    "join approach app on app.runway_end_id = e.runway_end_id "
    "where a.ident = :apIdent and a.airport_id <> :curApId)");

  deleteRunwayStmt.prepare(
    "delete from runway where airport_id in ( "
    "select a.airport_id "
    "from airport a "
    "where a.ident = :apIdent and a.airport_id <> :curApId)");

  fetchRunwayEndIdStmt.prepare(
    "select r.primary_end_id as runway_end_id "
    "from airport a join runway r on a.airport_id = r.airport_id "
    "where a.ident = :apIdent and a.airport_id <> :curApId "
    "union "
    "select r.secondary_end_id as runway_end_id "
    "from airport a join runway r on a.airport_id = r.airport_id "
    "where a.ident = :apIdent and a.airport_id <> :curApId");

  deleteRunwayEndStmt.prepare(
    "delete from runway_end where runway_end_id = :endId");

  deleteIlsStmt.prepare(
    "delete from ils where loc_runway_end_id = :endId");

  deleteAirportStmt.prepare(
    "delete from airport "
    "where ident = :apIdent and airport_id <> :curApId");

  deleteComStmt.prepare(delAptFeatureStmt("com"));
  deleteHelipadStmt.prepare(delAptFeatureStmt("helipad"));
  deleteStartStmt.prepare(delAptFeatureStmt("start"));
  deleteParkingStmt.prepare(delAptFeatureStmt("parking"));
  deleteApronStmt.prepare(delAptFeatureStmt("apron"));
  deleteApronLightStmt.prepare(delAptFeatureStmt("apron_light"));
  deleteFenceStmt.prepare(delAptFeatureStmt("fence"));
  deleteTaxiPathStmt.prepare(delAptFeatureStmt("taxi_path"));
  deleteDeleteApStmt.prepare(delAptFeatureStmt("delete_airport"));
  deleteWpStmt.prepare(delAptFeatureStmt("waypoint"));
  deleteVorStmt.prepare(delAptFeatureStmt("vor"));
  deleteNdbStmt.prepare(delAptFeatureStmt("ndb"));

  updateComStmt.prepare(updateAptFeatureStmt("com"));
  updateHelipadStmt.prepare(updateAptFeatureStmt("helipad"));
  updateStartStmt.prepare(updateAptFeatureStmt("start"));
  updateApronStmt.prepare(updateAptFeatureStmt("apron"));
  updateApronLightStmt.prepare(updateAptFeatureStmt("apron_light"));
  updateTaxiPathStmt.prepare(updateAptFeatureStmt("taxi_path"));

  // TODO
  // updateHasTaxiwaysStmt.prepare(
  // "update airport set has_taxiways = 0 "
  // "where ident = :apIdent and airport_id <> :curApId");

  fetchPrimaryAppStmt.prepare(
    "select "
    "ap.approach_id, eo.runway_end_id, ap.type, ap.has_gps_overlay, "
    "ap.fix_type, ap.fix_ident, ap.fix_region, ap.fix_airport_ident, ap.altitude, ap.heading, ap.missed_altitude "
    "from airport a "
    "join runway r on r.airport_id = a.airport_id "
    "join approach ap on ap.runway_end_id = e.runway_end_id "
    "join runway_end e on r.primary_end_id = e.runway_end_id "
    "join airport ao  on ao.ident = a.ident "
    "join runway ro on ro.airport_id = ao.airport_id "
    "join runway_end eo on ro.primary_end_id = eo.runway_end_id "
    "where "
    "a.ident = :apIdent and a.airport_id = :curApId and "
    "ao.ident = :apIdent and ao.airport_id <> :curApId and "
    "e.name = eo.name ");

  fetchSecondaryAppStmt.prepare(
    "select "
    "ap.approach_id, eo.runway_end_id, ap.type, ap.has_gps_overlay, "
    "ap.fix_type, ap.fix_ident, ap.fix_region, ap.fix_airport_ident, ap.altitude, ap.heading, ap.missed_altitude "
    "from airport a "
    "join runway r on r.airport_id = a.airport_id "
    "join approach ap on ap.runway_end_id = e.runway_end_id "
    "join runway_end e on r.secondary_end_id = e.runway_end_id "
    "join airport ao  on ao.ident = a.ident "
    "join runway ro on ro.airport_id = ao.airport_id "
    "join runway_end eo on ro.secondary_end_id = eo.runway_end_id "
    "where "
    "a.ident = :apIdent and a.airport_id = :curApId and "
    "ao.ident = :apIdent and ao.airport_id <> :curApId and "
    "e.name = eo.name ");

  fetchTransitionStmt.prepare("select * from transition where approach_id = :approachId");
}

DeleteProcessor::~DeleteProcessor()
{
}

QString DeleteProcessor::updateAptFeatureStmt(const QString& table)
{
  return "update " + table +
         " set airport_id = :curApId where airport_id in ( "
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

void DeleteProcessor::copyApproaches(SqlQuery& fetchApprStmt, int currentId, const QString& ident)
{
  SqlUtil util(&db);
  SqlQuery insertApprStmt(db);
  insertApprStmt.prepare(util.buildInsertStatement("approach"));
  SqlQuery insertTrStmt(db);
  insertTrStmt.prepare(util.buildInsertStatement("transition"));

  IdColFunc<ApproachWriter> appIdColFunc("approach_id", dataWriter.getApproachWriter());
  IdColFunc<TransitionWriter> trIdColFunc("transition_id", dataWriter.getApproachTransWriter());

  fetchApprStmt.bindValue(":apIdent", ident);
  fetchApprStmt.bindValue(":curApId", currentId);
  fetchApprStmt.exec();

  int copied = util.copyResultValues(fetchApprStmt, insertApprStmt, appIdColFunc);

  if(dataWriter.getOptions().isVerbose())
    qDebug() << copied << " approaches copied";

  for(QPair<int, int> iter : appIdColFunc.mappedIds)
  {
    int oldId = iter.first, newId = iter.second;
    trIdColFunc.fixedColValues.clear();
    trIdColFunc.fixedColValues.push_back(QPair<QString, int>(":approach_id", newId));

    fetchTransitionStmt.bindValue(":approachId", oldId);
    fetchTransitionStmt.exec();

    int copyResults = util.copyResultValues(fetchTransitionStmt, insertTrStmt, trIdColFunc);
    if(dataWriter.getOptions().isVerbose())
      qDebug() << copyResults << " transitions copied";
  }
}

void DeleteProcessor::processDelete(const DeleteAirport& del, const Airport *type, int currentId)
{
  using bgl::util::isFlagSet;

  QString ident = type->getIdent();

  if(isFlagSet(del.getFlags(), bgl::del::APPROACHES))
    deleteApproaches(currentId, ident);
  else
  {
    deleteApproaches(currentId, ident);
    copyApproaches(fetchPrimaryAppStmt, currentId, ident);
    copyApproaches(fetchSecondaryAppStmt, currentId, ident);
  }

  if(isFlagSet(del.getFlags(), bgl::del::COMS))
  {
    deleteComStmt.bindValue(":apIdent", ident);
    deleteComStmt.bindValue(":curApId", currentId);
    executeStatement(deleteComStmt, "coms deleted");
  }
  else
  {
    updateComStmt.bindValue(":apIdent", ident);
    updateComStmt.bindValue(":curApId", currentId);
    executeStatement(updateComStmt, "coms updated");
  }

  if(isFlagSet(del.getFlags(), bgl::del::RUNWAYS))
  {
    deleteRunways(currentId, ident);
    deleteAirport(currentId, ident);
  }

  if(isFlagSet(del.getFlags(), bgl::del::APRONS))
  {
    deleteApronStmt.bindValue(":apIdent", ident);
    deleteApronStmt.bindValue(":curApId", currentId);
    executeStatement(deleteApronStmt, "aprons deleted");
  }
  else
  {
    updateApronStmt.bindValue(":apIdent", ident);
    updateApronStmt.bindValue(":curApId", currentId);
    executeStatement(updateApronStmt, "aprons updated");
  }

  if(isFlagSet(del.getFlags(), bgl::del::APRONLIGHTS))
  {
    deleteApronLightStmt.bindValue(":apIdent", ident);
    deleteApronLightStmt.bindValue(":curApId", currentId);
    executeStatement(deleteApronLightStmt, "apron lights deleted");
  }
  else
  {
    updateApronLightStmt.bindValue(":apIdent", ident);
    updateApronLightStmt.bindValue(":curApId", currentId);
    executeStatement(updateApronLightStmt, "apron lights updated");
  }

  if(isFlagSet(del.getFlags(), bgl::del::HELIPADS))
  {
    deleteHelipadStmt.bindValue(":apIdent", ident);
    deleteHelipadStmt.bindValue(":curApId", currentId);
    executeStatement(deleteHelipadStmt, "helipads deleted");
  }
  else
  {
    updateHelipadStmt.bindValue(":apIdent", ident);
    updateHelipadStmt.bindValue(":curApId", currentId);
    executeStatement(updateHelipadStmt, "helipads updated");
  }

  if(isFlagSet(del.getFlags(), bgl::del::STARTS))
  {
    deleteStartStmt.bindValue(":apIdent", ident);
    deleteStartStmt.bindValue(":curApId", currentId);
    executeStatement(deleteStartStmt, "starts deleted");
  }
  else
  {
    updateStartStmt.bindValue(":apIdent", ident);
    updateStartStmt.bindValue(":curApId", currentId);
    executeStatement(updateStartStmt, "starts updated");
  }

  if(isFlagSet(del.getFlags(), bgl::del::TAXIWAYS))
  {
    deleteTaxiPathStmt.bindValue(":apIdent", ident);
    deleteTaxiPathStmt.bindValue(":curApId", currentId);
    executeStatement(deleteTaxiPathStmt, "taxi paths deleted");
  }
  else
  {
    updateTaxiPathStmt.bindValue(":apIdent", ident);
    updateTaxiPathStmt.bindValue(":curApId", currentId);
    executeStatement(updateTaxiPathStmt, "taxi paths updated");
  }
}

void DeleteProcessor::executeStatement(SqlQuery& stmt, const QString& what)
{
  stmt.exec();
  int retval = stmt.numRowsAffected();

  if(dataWriter.getOptions().isVerbose())
    qDebug() << retval << " " << what;
}

void DeleteProcessor::fetchIds(SqlQuery& stmt, QList<int>& ids, const QString& what)
{
  stmt.exec();
  while(stmt.next())
    ids.push_back(stmt.value(0).toInt());

  if(dataWriter.getOptions().isVerbose())
    qDebug() << ids.size() << " " << what;
}

void DeleteProcessor::deleteApproaches(int currentId, const QString& ident)
{
  deletePrimaryTransitionStmt.bindValue(":apIdent", ident);
  deletePrimaryTransitionStmt.bindValue(":curApId", currentId);

  executeStatement(deletePrimaryTransitionStmt, "primary transitions deleted");

  deletePrimaryApproachStmt.bindValue(":apIdent", ident);
  deletePrimaryApproachStmt.bindValue(":curApId", currentId);
  executeStatement(deletePrimaryApproachStmt, "primary approaches deleted");

  deleteSecondaryTransitionStmt.bindValue(":apIdent", ident);
  deleteSecondaryTransitionStmt.bindValue(":curApId", currentId);
  executeStatement(deleteSecondaryTransitionStmt, "secondary transitions deleted");

  deleteSecondaryApproachStmt.bindValue(":apIdent", ident);
  deleteSecondaryApproachStmt.bindValue(":curApId", currentId);
  executeStatement(deleteSecondaryApproachStmt, "secondary approaches deleted");
}

void DeleteProcessor::deleteRunways(int currentId, const QString& ident)
{
  QList<int> runwayEndIds;
  fetchRunwayEndIdStmt.bindValue(":apIdent", ident);
  fetchRunwayEndIdStmt.bindValue(":curApId", currentId);
  fetchIds(fetchRunwayEndIdStmt, runwayEndIds, " runway ends to delete");

  deleteRunwayStmt.bindValue(":apIdent", ident);
  deleteRunwayStmt.bindValue(":curApId", currentId);
  executeStatement(deleteRunwayStmt, "runways deleted");

  for(int it : runwayEndIds)
  {
    deleteIlsStmt.bindValue(":endId", it);
    executeStatement(deleteIlsStmt, "ils deleted");
    deleteRunwayEndStmt.bindValue(":endId", it);
    executeStatement(deleteRunwayEndStmt, "runway ends deleted");
  }
}

void DeleteProcessor::deleteAirport(int currentId, const QString& ident)
{
  deleteParkingStmt.bindValue(":apIdent", ident);
  deleteParkingStmt.bindValue(":curApId", currentId);
  executeStatement(deleteParkingStmt, "parking spots deleted");

  deleteDeleteApStmt.bindValue(":apIdent", ident);
  deleteDeleteApStmt.bindValue(":curApId", currentId);
  executeStatement(deleteDeleteApStmt, "delete airports deleted");

  deleteFenceStmt.bindValue(":apIdent", ident);
  deleteFenceStmt.bindValue(":curApId", currentId);
  executeStatement(deleteFenceStmt, "fences deleted");

  // TODO check if these are really to be deleted - otherwise adapt ids
  // executeStatement(deleteVorStmt(":apIdent", ident)(":curApId", currentId), "vors deleted");
  // executeStatement(deleteNdbStmt(":apIdent", ident)(":curApId", currentId), "ndbs deleted");
  // executeStatement(deleteWpStmt(":apIdent", ident)(":curApId", currentId), "waypoints deleted");

  deleteAirportStmt.bindValue(":apIdent", ident);
  deleteAirportStmt.bindValue(":curApId", currentId);
  executeStatement(deleteAirportStmt, "airports deleted");
}

} // namespace writer
} // namespace fs
} // namespace atools
