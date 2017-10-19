/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_DFDDATACOMPILER_H
#define ATOOLS_DFDDATACOMPILER_H

#include "geo/rect.h"

#include <QString>

namespace atools {
namespace sql {
class SqlDatabase;
class SqlQuery;
class SqlRecordVector;
class SqlRecord;
}
namespace fs {

namespace common {
class MagDecReader;
class MetadataWriter;
class AirportIndex;
class ProcedureWriter;
struct ProcedureInput;
}

class NavDatabaseOptions;
class NavDatabaseErrors;
class ProgressHandler;

namespace ng {

/*
 * Creates a Little Navmap scenery database from a DFD database.
 * Only for command line based compilation.
 */
class DfdCompiler
{
public:
  DfdCompiler(atools::sql::SqlDatabase& sqlDb, const atools::fs::NavDatabaseOptions& opts,
              atools::fs::ProgressHandler *progressHandler, atools::fs::NavDatabaseErrors *navdatabaseErrors);
  virtual ~DfdCompiler();

  void close();

  const QString& getAiracCycle() const
  {
    return airacCycle;
  }

  /* Read magnetic declination. */
  void compileMagDeclBgl();

  /* Reads AIRAC number from header file */
  void readHeader();

  /* Write a single dummy scenery area and file */
  void writeFileAndSceneryMetadata();

  /* Attach source database with alias "src" */
  void attachDatabase();
  void detachDatabase();

  /* Read source airport and write to airport */
  void writeAirports();

  /* Read runway table and write to runway and runway_end */
  void writeRunways();

  /* Fills waypoint, vor, ndb, ils and marker.
   * Also creates NDB and VOR waypoints from airway and procedure references */
  void writeNavaids();

  /* Update declination for waypoint and NDB */
  void updateMagvar();

  /* Update TACAN and VORTAC channels */
  void updateTacanChannel();

  /* Calculate the ILS endpoints for map displayy */
  void updateIlsGeometry();

  /* FIll table airway */
  void writeAirways();

  /* Read approaches, transitions, SIDs and STARs */
  void writeProcedures();

  void initQueries();
  void deInitQueries();

private:
  /* Length of the ILS feather */
  const int ILS_FEATHER_LEN_NM = 9;

  /* Write all collected runways for an airport */
  void writeRunwaysForAirport(sql::SqlRecordVector& runways, const QString& apt);

  /* Match opposing runway ends */
  void pairRunways(QVector<std::pair<atools::sql::SqlRecord, atools::sql::SqlRecord> >& runwaypairs,
                   sql::SqlRecordVector& runways);
  void fillProcedureInput(atools::fs::common::ProcedureInput& procInput, const atools::sql::SqlQuery& query);
  void writeProcedure(const QString& table, const QString& rowCode);

  const atools::fs::NavDatabaseOptions& options;
  atools::sql::SqlDatabase& db;
  atools::fs::ProgressHandler *progress = nullptr;
  atools::fs::NavDatabaseErrors *errors = nullptr;
  atools::fs::common::MagDecReader *magDecReader = nullptr;
  atools::fs::common::AirportIndex *airportIndex = nullptr;
  atools::fs::common::ProcedureWriter *procWriter = nullptr;
  atools::fs::common::MetadataWriter *metadataWriter = nullptr;

  int curAirportId = 0, curRunwayId = 0, curRunwayEndId = 0;

  /* Hardcoded dummy ids */
  const int FILE_ID = 1, SCENERY_ID = 1;
  QString airacCycle;

  /* Remember surface of longest runway for an airport*/
  QHash<QString, QString> longestRunwaySurfaceMap;
  QHash<QString, atools::geo::Rect> airportRectMap;

  atools::sql::SqlQuery *airportQuery = nullptr, *airportWriteQuery = nullptr, *airportUpdateQuery = nullptr,
                        *runwayQuery = nullptr, *runwayWriteQuery = nullptr, *runwayEndWriteQuery = nullptr,
                        *metadataQuery = nullptr;

};

} // namespace ng
} // namespace fs
} // namespace atools

#endif // ATOOLS_DFDDATACOMPILER_H
