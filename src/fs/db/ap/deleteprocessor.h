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

#ifndef ATOOLS_FS_DB_AP_DELETEPROCESSOR_H
#define ATOOLS_FS_DB_AP_DELETEPROCESSOR_H

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
namespace db {

class DataWriter;
class ApproachWriter;

/*
 * Deletes stock/default airports for a new airport. Uses the delete records and removes or updates all
 * old airports and their facilities.
 */
class DeleteProcessor
{
public:
  DeleteProcessor(atools::sql::SqlDatabase& sqlDb, const atools::fs::NavDatabaseOptions& opts);
  virtual ~DeleteProcessor();

  /*
   * Start the update or removal process of airports. The current/new airport has to
   * be stored in the database already.
   *
   * @param deleteAirportRec delete airport record containing information what to update and/or what to remove
   * @param airport airport record that will replace all other airports
   * @param currentAirportId database ID of the airport that will replace all other airports
   */
  void preProcessDelete();
  void postProcessDelete();

  void init(const atools::fs::bgl::DeleteAirport *deleteAirportRec,
            const atools::fs::bgl::Airport *airport, int airportId);

  const QString& getBglFilename() const
  {
    return bglFilename;
  }

  const QString& getSceneryLocalPath() const
  {
    return sceneryLocalPath;
  }

private:
  int executeStatement(sql::SqlQuery *stmt, const QString& what);
  void fetchIds(sql::SqlQuery *stmt, QList<int>& ids, const QString& what);

  void removeRunways();
  void removeAirport();

  QString updateAptFeatureStmt(const QString& table);
  QString delAptFeatureStmt(const QString& table);
  void removeOrUpdate(sql::SqlQuery *deleteStmt, sql::SqlQuery *updateStmt,
                      atools::fs::bgl::del::DeleteAllFlags flag);
  QString updateAptFeatureToNullStmt(const QString& table);
  void removeApproachesAndTransitions(const QList<int>& ids);
  void extractDeleteFlags();

  int bindAndExecute(sql::SqlQuery *query, const QString& msg);
  int bindAndExecute(const QString& sql, const QString& msg);
  void extractPreviousAirportFeatures();
  void copyAirportValues(const QStringList& copyAirportColumns);

  const atools::fs::NavDatabaseOptions& options;

  atools::sql::SqlQuery
  *deleteRunwayStmt = nullptr,
  *updateRunwayStmt = nullptr,
  *deleteParkingStmt = nullptr,
  *updateParkingStmt = nullptr,
  *deleteDeleteApStmt = nullptr,
  *fetchRunwayEndIdStmt = nullptr,
  *updateApprochRwIds = nullptr,
  *deleteRunwayEndStmt = nullptr,
  *delWpStmt = nullptr,
  *updateVorStmt = nullptr,
  *updateNdbStmt = nullptr,
  *deleteApprochStmt = nullptr, *updateApprochStmt = nullptr,
  *deleteAirportStmt = nullptr, *selectAirportStmt = nullptr,
  *deleteApronStmt = nullptr, *updateApronStmt = nullptr,
  *deleteApronLightStmt = nullptr, *updateApronLightStmt = nullptr,
  *deleteFenceStmt = nullptr, *updateFenceStmt = nullptr,
  *deleteHelipadStmt = nullptr, *updateHelipadStmt = nullptr,
  *deleteStartStmt = nullptr, *updateStartStmt = nullptr,
  *deleteTaxiPathStmt = nullptr, *updateTaxiPathStmt = nullptr,
  *deleteComStmt = nullptr, *updateComStmt = nullptr,
  *fetchPrimaryAppStmt = nullptr, *fetchSecondaryAppStmt = nullptr;

  const atools::fs::bgl::DeleteAirport *deleteAirport = nullptr;
  atools::fs::bgl::del::DeleteAllFlags deleteFlags = atools::fs::bgl::del::NONE;
  const atools::fs::bgl::Airport *newAirport = nullptr;
  int currentAirportId = 0;
  QString ident, bglFilename, sceneryLocalPath;
  atools::sql::SqlDatabase *db = nullptr;

  bool hasApproach = false, hasApron = false, hasCom = false, hasHelipad = false,
       hasTaxi = false, hasStart = false, hasRunways = false, isAddon = false;
  int previousRating = 0;
  bool hasPrevious = false;
  int prevAirportId = 0;

};

} // namespace writer
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_DB_AP_DELETEPROCESSOR_H
