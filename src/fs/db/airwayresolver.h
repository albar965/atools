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

#ifndef ATOOLS_FS_DB_AIRWAYRESOLVER_H
#define ATOOLS_FS_DB_AIRWAYRESOLVER_H

#include "sql/sqlquery.h"

#include <QSet>
#include <QCoreApplication>

namespace atools {
namespace geo {
class Pos;
}
namespace fs {
class ProgressHandler;
namespace db {

/*
 * Reads from the tmp_airway_point table that was filled with waypoint record data and connects the
 * waypoint lists to airways that are stored in table airway.
 */
class AirwayResolver
{
  Q_DECLARE_TR_FUNCTIONS(AirwayResolver)

public:
  AirwayResolver(atools::sql::SqlDatabase *sqlDb, atools::fs::ProgressHandler& progress);
  virtual ~AirwayResolver();

  /*
   * Build airways from tmp_airway_point table that uses only idents and region codes to connect waypoints to a chain.
   * This process has to run after all BGL files are loaded since the airways cross multiple
   * scenery areas and BGL files.
   * Reads from "tmp_airway_point" joined with "waypoint" and writes to table "airway".
   * @return true if the process was aborted
   */
  bool run(int numReportSteps);

  struct AirwaySegment;

  /*
   * Assigns the waypoint_id in table tmp_airway_point. Not needed for all compilations.
   */
  void assignWaypointIds();

  /* Maximum length before creating a new fragment in meter */
  void setMaxAirwaySegmentLengthNm(float value)
  {
    maxAirwaySegmentLengthNm = value;
  }

private:
  float maxAirwaySegmentLengthNm = 0.f;

  typedef std::pair<QString, QVariant> TypeRowValue;
  typedef QList<TypeRowValue> TypeRowValueList;

  struct Fragment
  {
    QSet<int> waypoints;
    QList<TypeRowValueList> boundValues;
  };

  void buildAirway(const QString& airwayName, QSet<atools::fs::db::AirwayResolver::AirwaySegment>& airway,
                   QList<Fragment>& fragments);

  /* Remove empty segments and segments that are contained by another */
  void cleanFragments(QList<Fragment>& fragments);

  /* Save airways to table airway */
  void saveAirway(QSet<AirwaySegment>& airway, const QString& currentAirway);

  /* Fetch navaid id and position. Takes the nearest in case of disambiguities */
  void fetchNavaid(int& id, atools::geo::Pos& pos, sql::SqlQuery& tmpAirwayPointQuery, sql::SqlQuery& tmpWaypointQuery,
                   const QString& prefix, const atools::geo::Pos& lastPos);

  atools::fs::ProgressHandler& progressHandler;
  int curAirwayId, numAirways;
  atools::sql::SqlQuery airwayInsertStmt;
  atools::sql::SqlDatabase *db;
};

} // namespace writer
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_DB_AIRWAYRESOLVER_H
