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

#ifndef WRITER_AP_DELETEPROCESSOR_H_
#define WRITER_AP_DELETEPROCESSOR_H_

#include "fs/bgl/ap/del/deleteairport.h"
#include "fs/bgl/ap/airport.h"

namespace bgl {
namespace ap {
class Airport;
namespace del {
class DeleteAirport;
}
}
}

namespace atools {
namespace sql {
class SqlQuery;
class SqlDatabase;
}
namespace fs {
namespace writer {

class DataWriter;
class ApproachWriter;

class DeleteProcessor
{
public:
  DeleteProcessor(atools::sql::SqlDatabase& sqlDb, atools::fs::writer::DataWriter& writer);
  virtual ~DeleteProcessor();

  void processDelete(const bgl::DeleteAirport *delAp,
                     const atools::fs::bgl::Airport *airport,
                     int currentApId);

private:
  void executeStatement(sql::SqlQuery *stmt, const QString& what);
  void fetchIds(sql::SqlQuery *stmt, QList<int>& ids, const QString& what);

  void transferApproaches();
  void deleteRunways();
  void deleteAirport();

  writer::DataWriter& dataWriter;
  atools::sql::SqlDatabase& db;

  atools::sql::SqlQuery
  *deleteRunwayStmt,
  *deleteParkingStmt,
  *deleteDeleteApStmt,
  *fetchRunwayEndIdStmt,
  *fetchPrimaryRunwayEndIdStmt, *fetchSecondaryRunwayEndIdStmt, *updateApprochRwIds,
  *deleteRunwayEndStmt,
  *deleteIlsStmt,
  *nullWpStmt,
  *nullVorStmt,
  *nullNdbStmt,
  *deleteAirportStmt, *selectAirportStmt,
  *deleteApronStmt, *updateApronStmt,
  *deleteApronLightStmt, *updateApronLightStmt,
  *deleteFenceStmt, *updateFenceStmt,
  *deleteHelipadStmt, *updateHelipadStmt,
  *deleteStartStmt, *updateStartStmt,
  *deleteTaxiPathStmt, *updateTaxiPathStmt,
  *deleteComStmt, *updateComStmt,
  *fetchPrimaryAppStmt, *fetchSecondaryAppStmt,

  *deleteTransitionLegStmt,
  *deleteApproachLegStmt,
  *deleteTransitionStmt,
  *deleteApproacheStmt,
  *fetchOldApproachIdStmt;

  bool hasApproach = true, hasApron = true, hasCom = true, hasHelipad = true, hasTaxi = true,
       hasRunways = true;

  QString updateAptFeatureStmt(const QString& table);
  QString delAptFeatureStmt(const QString& table);
  void deleteOrUpdate(sql::SqlQuery *deleteStmt,
                      sql::SqlQuery *updateStmt,
                      atools::fs::bgl::del::DeleteAllFlags flag);

  const atools::fs::bgl::DeleteAirport *del;
  const atools::fs::bgl::Airport *type;
  int currentId;
  QString ident;
  void bindAndExecute(sql::SqlQuery *delQuery, const QString& msg);

  QString updateAptFeatureToNullStmt(const QString& table);
  void deleteApproachesAndTransitions(const QList<int>& ids);

  QList<int> fetchOldApproachIds();

};

} // namespace writer
} // namespace fs
} // namespace atools

#endif /* WRITER_AP_DELETEPROCESSOR_H_ */
