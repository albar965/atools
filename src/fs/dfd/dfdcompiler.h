/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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
#include "geo/linestring.h"
#include "sql/sqltypes.h"

#include <QString>

namespace atools {
namespace sql {
class SqlDatabase;
class SqlQuery;
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
 * Creates a Little Navmap scenery database from an extended DFD database.
 * Only for command line based compilation.
 */
class DfdCompiler
{
public:
  DfdCompiler(atools::sql::SqlDatabase& sqlDb, const atools::fs::NavDatabaseOptions& opts,
              atools::fs::ProgressHandler *progressHandler);
  virtual ~DfdCompiler();

  DfdCompiler(const DfdCompiler& other) = delete;
  DfdCompiler& operator=(const DfdCompiler& other) = delete;

  void close();

  /* AIRAC cycle as read from the source database by readHeader() */
  const QString& getAiracCycle() const
  {
    return airacCycle;
  }

  /* Validity period as read from the source database by readHeader() */
  const QString& getValidThrough() const
  {
    return validThrough;
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

  /* Fill parking from tbl_gates */
  void writeParking();

  /* Fills boundary table from DFD tables */
  void writeAirspaces();

  /* Update FIR/UIR com frequencies in boundaries table */
  void writeAirspaceCom();

  /* Fills com table - airport communications. Also updates airport fields. */
  void writeCom();

  /* Update declination for waypoint and NDB */
  void updateMagvar();

  /* Update TACAN and VORTAC channels */
  void updateTacanChannel();

  /* Calculate the ILS endpoints for map display */
  void updateIlsGeometry();

  /* Calculate the airport MSA geometry */
  void writeAirportMsa();

  /* FIll table airway */
  void writeAirways();

  /* Read approaches, transitions, SIDs and STARs */
  void writeProcedures();

  /* minimum off route altitude - read source table and write to mora_grid table */
  void writeMora();

  void initQueries();
  void deInitQueries();

  /* Convert all airport ident columns to the three letter codes since DFD has only four-letters */
  void updateTreeLetterAirportCodes();

private:
  /* Write all collected runways for an airport */
  void writeRunwaysForAirport(atools::sql::SqlRecordList& runways, const QString& apt);

  /* Match opposing runway ends */
  void pairRunways(QVector<std::pair<atools::sql::SqlRecord, atools::sql::SqlRecord> >& runwaypairs,
                   const sql::SqlRecordList& runways);

  /* Fill input structure for ProcedureWriter */
  void fillProcedureInput(atools::fs::common::ProcedureInput& procInput, const atools::sql::SqlQuery& query);

  /* Write on procedure type - SID, STAR, approaches */
  void writeProcedure(const QString& table, const QString& rowCode);

  /* Start airspace and fill insert query with general airspace data like limits and name from the first source column */
  void beginAirspace(const sql::SqlQuery& query);

  /* Specialized begin airspace methods - passed as pointer to writeAirspace() */
  void beginRestrictiveAirspace(atools::sql::SqlQuery& query);
  void beginControlledAirspace(atools::sql::SqlQuery& query);

  /* For old compatible airspaces as centers */
  void beginFirUirAirspaceCenter(atools::sql::SqlQuery& query);

  /* For new FIR, UIR and BOTH type */
  void beginFirUirAirspaceNew(atools::sql::SqlQuery& query);

  /* Bind airspace geometry from airspaceSegments to insert query */
  void writeAirspaceGeometry(sql::SqlQuery& query);
  void updateAirspaceCom(const sql::SqlQuery& com, atools::sql::SqlQuery& update, int airportId);

  /* Reads all rows of source airspace table */
  void writeAirspace(atools::sql::SqlQuery& query, void (DfdCompiler::*beginFunc)(atools::sql::SqlQuery&));

  /* Finalize and execute insert query */
  void finishAirspace();

  /* Get aispace altitude restriction which can start with FL and is converted into feet in this case */
  int airspaceAlt(const QString& altStr);

  /* Update airport ident with three letter code for given table */
  void updateTreeLetterAirportCodes(const QHash<QString, QString>& codeMap, const QString& table,
                                    const QString& column);

  /* Airspace segment containing information */
  struct AirspaceSeg
  {
    atools::geo::Pos pos, center /* Circle or arc center */;
    QString via; /* Flags */
    float distance; /* Circle or arc radius */
  };

  QVector<AirspaceSeg> airspaceSegments;

  /* Maps concatenated FIR and UIR airspace key columns to boundary_id in database */
  QHash<QString, int> airspaceIdentIdMap;

  const atools::fs::NavDatabaseOptions& options;
  atools::sql::SqlDatabase& db;
  atools::fs::ProgressHandler *progress = nullptr;
  atools::fs::common::MagDecReader *magDecReader = nullptr;
  atools::fs::common::AirportIndex *airportIndex = nullptr;
  atools::fs::common::ProcedureWriter *procWriter = nullptr;
  atools::fs::common::MetadataWriter *metadataWriter = nullptr;

  int curAirportId = 0, curRunwayId = 0, curRunwayEndId = 0, curAirspaceId = 0;

  /* Hardcoded dummy ids. Only one entry is created. */
  const int FILE_ID = 1, SCENERY_ID = 1;
  QString airacCycle, validThrough;

  /* Remember surface of longest runway for an airport*/
  QHash<QString, QString> longestRunwaySurfaceMap;
  QHash<QString, atools::geo::Rect> airportRectMap;

  atools::sql::SqlQuery *airportQuery = nullptr, *airportWriteQuery = nullptr, *airportFileWriteQuery = nullptr,
                        *airportUpdateQuery = nullptr, *runwayQuery = nullptr, *runwayWriteQuery = nullptr,
                        *runwayEndWriteQuery = nullptr, *metadataQuery = nullptr, *airspaceWriteQuery = nullptr,
                        *moraQuery = nullptr;

};

} // namespace ng
} // namespace fs
} // namespace atools

#endif // ATOOLS_DFDDATACOMPILER_H
