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
  DeleteProcessor(atools::sql::SqlDatabase& sqlDb, atools::fs::writer::DataWriter& writer);
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
                        deleteHelipadStmt, updateHelipadStmt,
                        deleteStartStmt, updateStartStmt,
                        deleteComStmt, updateComStmt, updateHasTaxiwaysStmt,
                        fetchPrimaryAppStmt,
                        fetchSecondaryAppStmt, fetchTransitionStmt;
};

} // namespace writer
} // namespace fs
} // namespace atools

#endif /* WRITER_AP_DELETEPROCESSOR_H_ */
