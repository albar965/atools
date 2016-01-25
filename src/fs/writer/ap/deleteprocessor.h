/*
 * DeleteProcessor.h
 *
 *  Created on: 19.05.2015
 *      Author: alex
 */

#ifndef WRITER_AP_DELETEPROCESSOR_H_
#define WRITER_AP_DELETEPROCESSOR_H_

#include "sql/sqlquery.h"
#include "fs/bgl/ap/del/deleteairport.h"
#include "fs/bgl/ap/airport.h"

namespace db {
class Database;
}
namespace bgl {
namespace ap {
class Airport;
namespace del {
class DeleteAirport;
}
}
}

namespace atools {
namespace fs {
namespace writer {

class DataWriter;
class ApproachWriter;

class DeleteProcessor
{
public:
  DeleteProcessor(atools::sql::SqlDatabase& db, atools::fs::writer::DataWriter& dataWriter);
  virtual ~DeleteProcessor();

  void processDelete(const atools::fs::bgl::DeleteAirport& del,
                     const atools::fs::bgl::Airport *type,
                     int currentId);

private:
  void executeStatement(atools::sql::SqlQuery& stmt, const QString& what);
  void fetchIds(atools::sql::SqlQuery& stmt, QList<int>& ids, const QString& what);

  void copyApproaches(atools::sql::SqlQuery& fetchApprStmt, int currentId, const QString& ident);
  void deleteApproaches(int currentId, const QString& ident);
  void deleteRunways(int currentId, const QString& ident);
  void deleteAirport(int currentId, const QString& ident);

  writer::DataWriter& dataWriter;
  atools::sql::SqlDatabase& db;

  atools::sql::SqlQuery deletePrimaryApproachStmt, deletePrimaryTransitionStmt,
                        deleteSecondaryApproachStmt, deleteSecondaryTransitionStmt,
                        deleteRunwayStmt,
                        deleteParkingStmt, deleteDeleteApStmt,
                        fetchRunwayEndIdStmt, deleteRunwayEndStmt,
                        deleteIlsStmt, deleteWpStmt, deleteVorStmt, deleteNdbStmt,
                        deleteAirportStmt, updateHasApronStmt,
                        deleteComStmt, updateComStmt, updateNumHelipadsStmt, updateHasTaxiwaysStmt,
                        fetchPrimaryAppStmt,
                        fetchSecondaryAppStmt, fetchTransitionStmt;
};

} // namespace writer
} // namespace fs
} // namespace atools

#endif /* WRITER_AP_DELETEPROCESSOR_H_ */
