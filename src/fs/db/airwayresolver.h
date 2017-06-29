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

#ifndef ATOOLS_FS_DB_AIRWAYRESOLVER_H
#define ATOOLS_FS_DB_AIRWAYRESOLVER_H

#include "sql/sqlquery.h"
#include "geo/pos.h"

#include <QSet>
#include <QCoreApplication>

namespace atools {
namespace fs {

class ProgressHandler;

namespace db {

/*
 * Reads from the airway_point table that was filled with waypoint record data and connects the
 * waypoint lists to airways that are stored in table airway.
 */
class AirwayResolver
{
  Q_DECLARE_TR_FUNCTIONS(AirwayResolver)

public:
  AirwayResolver(atools::sql::SqlDatabase *sqlDb, atools::fs::ProgressHandler& progress);
  virtual ~AirwayResolver();

  /*
   * Build airways from airway_point table that uses only idents and region codes to connect waypoints to a chain.
   * This process has to run after all BGL files are loaded since the airways cross multiple
   * scenery areas and BGL files.
   * @return true if the process was aborted
   */
  bool run();

  struct AirwaySegment;

private:
  static const int MAX_AIRWAY_SEGMENT_LENGTH_NM = 1000;

  typedef std::pair<QString, QVariant> TypeRowValue;
  typedef QVector<TypeRowValue> TypeRowValueVector;

  struct Fragment
  {
    QSet<int> waypoints;
    QVector<TypeRowValueVector> boundValues;
  };

  void buildAirway(const QString& airwayName, QSet<atools::fs::db::AirwayResolver::AirwaySegment>& airway,
                   QVector<Fragment>& fragments);
  void cleanFragments(QVector<Fragment>& fragments);

  atools::fs::ProgressHandler& progressHandler;
  int curAirwayId, numAirways;
  atools::sql::SqlQuery airwayInsertStmt;
  atools::sql::SqlDatabase *db;
};

} // namespace writer
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_DB_AIRWAYRESOLVER_H
