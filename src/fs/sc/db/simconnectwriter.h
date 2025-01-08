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

#ifndef ATOOLS_SIMCONNECTWRITER_H
#define ATOOLS_SIMCONNECTWRITER_H

#include <QMap>
#include <QCoreApplication>
#include <QHash>
#include <QElapsedTimer>

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

namespace common {
class MagDecReader;
}

namespace db {
class RunwayIndex;
}
namespace sc {
namespace db {

class IcaoId;
class RunwayId;
class FacilityId;
struct VorFacility;
struct NdbFacility;
struct LegFacility;
class Airport;
class Runway;
class Waypoint;
typedef QSet<FacilityId> FacilityIdSet;

/* Message, incProgress = true to increment progress, otherwise "still alive" message.  */
typedef std::function<bool (const QString& message, bool incProgress)> SimConnectWriterProgressCallback;

/*
 * Writes all airport and child structures as well as navaids to the database.
 * These are airports, runways, starts, parking taxiways and all procedures defined in fs/sc/db/simconnectairport.h.
 * Furthermore VOR, ILS, NDB, waypoints and airways as defined in simconnectnav.h are written.
 *
 * Note that methods have to be called in order writeAirportsToDatabase(), writeWaypointsAndAirwaysToDatabase(),
 * writeVorAndIlsToDatabase() and writeNdbToDatabase() to ensure correct runway/ILS matching.
 */
class SimConnectWriter
{
  Q_DECLARE_TR_FUNCTIONS(SimConnectWriter)

public:
  SimConnectWriter(atools::sql::SqlDatabase& sqlDb, bool verboseParam);
  ~SimConnectWriter();

  SimConnectWriter(const SimConnectWriter& other) = delete;
  SimConnectWriter& operator=(const SimConnectWriter& other) = delete;

  /* Writes all airport and child structures to the database and commits after finishing. The map key is not used.
   * fileId is used to fill db.file_id field for all airports.
   * Entries from the map airports are consumed while writing */
  bool writeAirportsToDatabase(QHash<atools::fs::sc::db::IcaoId, atools::fs::sc::db::Airport>& airports, int fileId);

  /* Writes all waypoints and airways to the database and commits after finishing. The map key is not used.
   * fileId is used to fill db.file_id field for all navaids. */
  bool writeWaypointsAndAirwaysToDatabase(const QMap<unsigned long, atools::fs::sc::db::Waypoint>& waypoints, int fileId);

  /* Writes all VOR and ILS to the database and commits after finishing. The map key is not used.
   * fileId is used to fill db.file_id field for all navaids. */
  bool writeVorAndIlsToDatabase(const QList<atools::fs::sc::db::VorFacility>& vors, int fileId);

  /* Writes all NDB to the database and commits after finishing. The map key is not used.
   * fileId is used to fill db.file_id field for all navaids. */
  bool writeNdbToDatabase(const QList<atools::fs::sc::db::NdbFacility>& ndbs, int fileId);

  /* Get full list of navaid ids from the database tables waypoint, ils, vor and ndb. */
  atools::fs::sc::db::FacilityIdSet getNavaidIds();

  /* Create and prepare all database queries. Call before any write*ToDatabase() */
  void initQueries();

  /* Delete all database queries */
  void deInitQueries();

  /* Get error or exception messages from writing using write*ToDatabase() */
  const QStringList& getErrors() const
  {
    return errors;
  }

  void clearErrors()
  {
    errors.clear();
  }

  /* Progress callback called before writing a batch to the database. Return true to abort loading. */
  void setProgressCallback(const SimConnectWriterProgressCallback& callback)
  {
    progressCallback = callback;
  }

private:
  const int MAX_ERRORS = 100;

  /* Indentifies parent procedure type when writing legs */
  enum LegType {APPROACH, MISSED, TRANS, SID, SIDTRANS, STAR, STARTRANS};

  /* Write one procedure leg for SID, STAR, approaches, transitions, missed and more */
  void writeLeg(atools::sql::SqlQuery *query, const atools::fs::sc::db::LegFacility& leg,
                atools::fs::sc::db::SimConnectWriter::LegType type, bool *verticalAngleFound = nullptr);

  /* Bind VASI information to runway end */
  void bindVasi(atools::sql::SqlQuery *query, const atools::fs::sc::db::Runway& runway, bool primary) const;

  /* Bind runway end ID, ARINC name and runway name to a procedure */
  void bindRunway(atools::sql::SqlQuery *query, const atools::fs::db::RunwayIndex& runwayIndex, const QString& airportIcao,
                  int runwayNumber, int runwayDesignator, const QString& sourceObject) const;

  /* Set all bound values for all queries to null */
  void clearAllBoundValues();

  bool callProgress(const QString& message, bool incProgress = true);
  bool callProgressUpdate();

  atools::sql::SqlDatabase& db;
  bool verbose = false;

  atools::fs::common::MagDecReader *magDecReader = nullptr;

  // All insert queries
  atools::sql::SqlQuery *airportStmt = nullptr, *airportFileStmt = nullptr, *runwayStmt = nullptr, *runwayEndStmt = nullptr,
                        *startStmt = nullptr, *frequencyStmt = nullptr, *helipadStmt = nullptr,
                        *taxiPathStmt = nullptr, *parkingStmt = nullptr,
                        *approachStmt = nullptr, *transitionStmt = nullptr, *approachLegStmt = nullptr, *transitionLegStmt = nullptr,
                        *waypointStmt = nullptr, *vorStmt = nullptr, *ndbStmt = nullptr, *ilsStmt = nullptr, *tmpAirwayPointStmt = nullptr;

  // Ids for primary keys
  int airportId = 0, airportFileId = 0, runwayId = 0, runwayEndId = 0, startId = 0, frequencyId = 0, helipadId = 0, taxiPathId = 0,
      parkingId = 0, approachId = 0, transitionId = 0, approachLegId = 0, transitionLegId = 0,
      waypointId = 0, vorId = 0, ndbId = 0, ilsId = 0, tmpAirwayPointId = 0;

  QStringList errors;

  /* Maps ILS ids found in runway structure to airport ident, runway name and runway end id.
   * Used when writing ILS. */
  QHash<atools::fs::sc::db::FacilityId, atools::fs::sc::db::RunwayId> ilsToRunwayMap;

  SimConnectWriterProgressCallback progressCallback = nullptr;

  int progressCounter = 0; // Counter for dot animation
  QString lastMessage; // Repeat last message when calling  callProgressUpdate()

  // Used to send progress reports not too often
  quint32 progressTimerElapsed = 0L;
  QElapsedTimer timer;
};

} // namespace db
} // namespace sc
} // namespace fs
} // namespace atools

#endif // ATOOLS_SIMCONNECTWRITER_H
