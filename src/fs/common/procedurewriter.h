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

#ifndef ATOOLS_FS_COMMON_PROCWRITER_H
#define ATOOLS_FS_COMMON_PROCWRITER_H

#include "fs/xp/xpwriter.h"

#include "sql/sqlrecord.h"
#include "geo/pos.h"
#include "sql/sqltypes.h"

namespace atools {

namespace sql {
class SqlDatabase;
class SqlQuery;
}

namespace fs {
class NavDatabaseErrors;
class NavDatabaseOptions;
class ProgressHandler;

namespace common {
class AirportIndex;

namespace rc {

/* Row code indicating approach type */
enum RowCode
{
  NONE,
  APPROACH,
  SID,
  STAR,
  RWY,
  PRDAT
};

}

/* Input from database table or X-Plane CIFP text file
 *  Comments denote column in ARINC text file and chapters explaining value */
struct ProcedureInput
{
  /* Context to be used to prefix warning messages */
  QString context;
  QString airportIdent;
  int airportId;
  atools::geo::DPos airportPos;

  /* APPCH, SID, STAR, or RWY (ignored) */
  QString rowCode; // All: 4.1.9.1. Page 55 (70)

  int seqNr; // 27-29   5.12
  char routeType; // 20      5.7
  QString sidStarAppIdent; // 14-19   5.9&5.10 Examples: DEPU2, SCK4, TRP7, 41M3, MONTH6
  QString transIdent; // 21-25   5.11

  QString fixIdent; // 30-34   5.13
  QString region; // 35-36   5.14
  QString secCode; // 37      5.4
  QString subCode; // 38      5.5
  QString descCode; // 40-43   5.17
  atools::geo::DPos waypointPos; // Need accurate positions so we can query for exact values

  QString turnDir; // 44      5.20
  QString pathTerm; // 48-49   5.21

  QString recdNavaid; // 51-54   5.23
  QString recdRegion; // 55-56   5.14
  QString recdSecCode; // 79      5.4
  QString recdSubCode; // 80      5.5
  atools::geo::DPos recdWaypointPos;

  float theta; // 63-66   5.24
  float rho; // 67-70   5.25
  float magCourse; // 71-74   5.26
  float rnp; // Required Navigation Performance - 5.211

  float rteHoldDist; // 75-78   5.27
  float rteHoldTime; // 75-78   5.27

  QString altDescr; // 83      5.29
  QString altitude; // 85-89   5.30
  QString altitude2; // 90-94   5.30
  QString transAlt; // 95-99   5.53
  QString speedLimitDescr; // 118     5.261
  int speedLimit; // 100-102 5.72
  QVariant verticalAngle; // degree with sign or null, 100 5.70
  QString centerFixOrTaaPt; // 107-111 5.144/5.271
  QString centerIcaoCode; // 113-114 5.14
  QString centerSecCode; // 115     5.4
  QString centerSubCode; // 116     5.5
  atools::geo::DPos centerPos;

  QString gnssFmsIndicator; // 117     5.222
  QString aircraftCategory; // 5.221
};

const float INVALID_FLOAT = std::numeric_limits<float>::max();

/*
 * Write SIDs, STARs, approaches and transitions to the database tables approach, approach_leg,
 * transition and transition_leg.
 *
 * Consumes one ProcedureInput struct for each line in a text file or each row in a database table
 * and writes all approaches,transitons, SIDs and STARs into the database.
 */
class ProcedureWriter
{
public:
  ProcedureWriter(atools::sql::SqlDatabase& sqlDb, atools::fs::common::AirportIndex *airportIndexParam);
  virtual ~ProcedureWriter();

  ProcedureWriter(const ProcedureWriter& other) = delete;
  ProcedureWriter& operator=(const ProcedureWriter& other) = delete;

  /* Call this for each line or row */
  void write(const ProcedureInput& line);

  /* Finalize and write the last procedure where information was collected */
  void finish(const ProcedureInput& line);

  /* Reset after writing procedures for one airport */
  void reset();

private:
  /* Used to store a procedure before writing to the database */
  struct Procedure
  {
    Procedure()
    {
    }

    Procedure(rc::RowCode rc, const atools::sql::SqlRecord& rec, bool commonRouteParam, const QString& sidStarNameParam)
      : rowCode(rc), record(rec), isCommonRoute(commonRouteParam), sidStarName(sidStarNameParam)
    {
    }

    bool isValid() const
    {
      return rowCode != rc::NONE;
    }

    QStringList runways;
    rc::RowCode rowCode = rc::NONE;
    atools::sql::SqlRecord record;
    atools::sql::SqlRecordList legRecords;
    bool isCommonRoute = false;
    QString sidStarName;
  };

  /* Gets type and region for a navaid that was retrieved using incomplete information */
  struct NavIdInfo
  {
    NavIdInfo()
    {
    }

    NavIdInfo(const QString& typeParam, const QString& regionParam) : type(typeParam), region(regionParam)
    {
    }

    QString type, region; // Region is always set - either value passed to the method or found value
  };

  void initQueries();
  void deInitQueries();

  /* Write an approach, SID, STAR or transition */
  void writeProcedure(const ProcedureInput& line);

  /* Write an approach, SID, STAR or transition leg */
  void writeProcedureLeg(const ProcedureInput& line);

  /* Fill a leg for the transition_leg or approach_leg table */
  void bindLeg(const ProcedureInput& line, sql::SqlRecord& rec);

  /* Write an approach, SID, STAR */
  void writeApproach(const ProcedureInput& line);
  void writeApproachLeg(const ProcedureInput& line);

  /* Write a transition */
  void writeTransition(const ProcedureInput& line);
  void writeTransitionLeg(const ProcedureInput& line);

  /* Reorder and duplicate procedures and legs, then write into the database */
  void finishProcedure(const ProcedureInput& line);

  atools::fs::common::rc::RowCode toRowCode(const ProcedureInput& line);

  /* Calculate a navaid type based on section and subsection code or waypoint description.
   *  If not valid query the database for navaids */
  NavIdInfo navaidType(const QString& context, const QString& descCode, const QString& sectionCode,
                       const QString& subSectionCode, const QString& ident, const QString& region,
                       const geo::DPos& pos, const atools::geo::DPos& airportPos);
  NavIdInfo navaidTypeFix(const ProcedureInput& line);

  /* Calculate a database procedure type based on route type */
  QString procedureType(const ProcedureInput& line);

  /* Assigns new ids to the currently stored approaches */
  void assignApproachIds(ProcedureWriter::Procedure& proc);
  void assignApproachLegIds(atools::sql::SqlRecordList& records);

  /* Assigns new ids to the currently stored transitions */
  void assignTransitionIds(ProcedureWriter::Procedure& proc);

  /* Extract runway names */
  void apprRunwayNameAndSuffix(const ProcedureInput& line, QString& runway, QString& suffix);
  QString sidStarRunwayNameAndSuffix(const ProcedureInput& line);

  /* Extract altitude probably containing a FL prefix*/
  float altitudeFromStr(const QString& altStr);

  void findFix(atools::sql::SqlQuery *query, const QString& ident, const QString& region,
               const atools::geo::DPos& pos) const;

  /* Database ids */
  int curApproachId = 0, curTransitionId = 0, curApproachLegId = 0, curTransitionLegId = 0;

  /* Count number */
  int numProcedures = 0;

  atools::sql::SqlDatabase& db;
  atools::sql::SqlQuery *insertApproachQuery = nullptr, *insertTransitionQuery = nullptr,
                        *insertApproachLegQuery = nullptr, *insertTransitionLegQuery = nullptr,
                        *updateAirportQuery = nullptr,
                        *findWaypointExactQuery = nullptr, *findWaypointQuery = nullptr,
                        *findIlsExactQuery = nullptr, *findIlsQuery = nullptr;

  /* Index to look up airport and runway ids */
  atools::fs::common::AirportIndex *airportIndex;
  const atools::sql::SqlRecord APPROACH_RECORD, APPROACH_LEG_RECORD, TRANSITION_RECORD, TRANSITION_LEG_RECORD;

  /* Temporary storage before writing to database keeps one approach/SID/STAR and respective transitions
   *  before writing  */
  QVector<Procedure> approaches;
  QVector<Procedure> transitions;

  rc::RowCode curRowCode;
  int curSeqNo = std::numeric_limits<int>::max();
  char curRouteType = ' ';
  QString curRouteIdent, curTransIdent;

  bool writingMissedApproach = false;

};

} // namespace common
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_COMMON_PROCWRITER_H
