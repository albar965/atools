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

#ifndef ATOOLS_SIMCONNECTWRITER_H
#define ATOOLS_SIMCONNECTWRITER_H

#include <QMap>
#include <QCoreApplication>

namespace atools {

namespace geo {
class Pos;
}

namespace win {
class ActivationContext;
}

namespace sql {
class SqlQuery;
class SqlDatabase;
}
namespace fs {

namespace db {
class RunwayIndex;
}
namespace sc {
namespace airport {

struct LegFacility;
class Airport;
class Runway;

/*
 * Writes all airport and child structures to the database.
 * These are airports, runways, starts, parking taxiways and all procedures defined in fs/sc/airport/simconnectfacilities.h
 */
class SimConnectWriter
{
  Q_DECLARE_TR_FUNCTIONS(SimConnectWriter)

public:
  SimConnectWriter(atools::sql::SqlDatabase& sqlDb, bool verboseParam);
  ~SimConnectWriter();

  /* Writes all airport and child structures to the database and commits after finishing the batch. The map key is not used.
   * fileId is used to fill airport.file_id field for all airports.
   * Clears errors before saving.*/
  void writeAirportsToDatabase(const QMap<unsigned long, atools::fs::sc::airport::Airport>& airports, int fileId);

  /* Create and prepare all database queries. Call before writeAirportsToDatabase() */
  void initQueries();

  /* Delete all database queries */
  void deInitQueries();

  /* Get error or exception messages from writing using writeAirportsToDatabase() */
  const QStringList& getErrors() const
  {
    return errors;
  }

private:
  const int MAX_ERRORS = 100;

  /* Indentifies parent procedure type when writing legs */
  enum LegType {APPROACH, MISSED, TRANS, SID, SIDTRANS, STAR, STARTRANS};

  /* Write one procedure leg for SID, STAR, approaches, transitions, missed and more */
  void writeLeg(atools::sql::SqlQuery *query, const atools::fs::sc::airport::LegFacility& leg,
                atools::fs::sc::airport::SimConnectWriter::LegType type, bool *verticalAngleFound = nullptr);

  /* Bind VASI information to runway end */
  void bindVasi(atools::sql::SqlQuery *query, const atools::fs::sc::airport::Runway& runway, bool primary) const;

  /* Bind runway end ID, ARINC name and runway name to a procedure */
  void bindRunway(atools::sql::SqlQuery *query, const atools::fs::db::RunwayIndex& runwayIndex, const QString& airportIcao,
                  int runwayNumber, int runwayDesignator, const QString& sourceObject) const;

  /* Set all bound values for all queries to null */
  void clearAllBoundValues();

  atools::sql::SqlDatabase& db;
  bool verbose = false;

  // All insert queries
  atools::sql::SqlQuery *airportStmt = nullptr, *airportFileStmt = nullptr, *runwayStmt = nullptr, *runwayEndStmt = nullptr,
                        *startStmt = nullptr, *frequencyStmt = nullptr, *helipadStmt = nullptr, *taxiPathStmt = nullptr,
                        *parkingStmt = nullptr,
                        *approachStmt = nullptr, *transitionStmt = nullptr, *approachLegStmt = nullptr, *transitionLegStmt = nullptr;

  // Ids for primary keys
  int airportId = 0, airportFileId = 0, runwayId = 0, runwayEndId = 0, startId = 0, frequencyId = 0, helipadId = 0, taxiPathId = 0,
      parkingId = 0, approachId = 0, transitionId = 0, approachLegId = 0, transitionLegId = 0;

  QStringList errors;

};

} // namespace airport
} // namespace sc
} // namespace fs
} // namespace atools

#endif // ATOOLS_SIMCONNECTWRITER_H
