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

#ifndef ATOOLS_ROUTEEDGEWRITER_H
#define ATOOLS_ROUTEEDGEWRITER_H

#include <QVariantList>
#include <QCoreApplication>

class QString;

namespace atools {
namespace geo {
class Rect;
class Pos;
}
namespace sql {
class SqlDatabase;
class SqlQuery;
}

namespace fs {
namespace db {

class ProgressHandler;

/*
 * Creates a routing network from VOR and NDB stations that are reachable from each other.
 * Fills table route_edge_radio and reads from table route_node_radio.
 */
class RouteEdgeWriter
{
  Q_DECLARE_TR_FUNCTIONS(AirwayResolver)

public:
  RouteEdgeWriter(atools::sql::SqlDatabase *sqlDb, atools::fs::db::ProgressHandler& progress,
                  int numProgressSteps);

  /*
   * Run the process and fill the route_edge_radio table. Reports process every 500 ms.
   * @return true if the process was aborted
   */
  bool run();

private:
  void bindCoordinatePointInRect(const atools::geo::Rect& rect, atools::sql::SqlQuery *query);
  bool nearest(atools::sql::SqlQuery& nearestStmt, int fromNodeId, const geo::Pos& pos,
               const geo::Rect& queryRect,
               int fromRangeMeter, QVariantList& toNodeIds, QVariantList& toNodeTypes,
               QVariantList& toNodeDistances);

  int numSteps = 10;
  atools::fs::db::ProgressHandler& progressHandler;
  atools::sql::SqlDatabase *db;

};

} // namespace writer
} // namespace fs
} // namespace atools

#endif // ATOOLS_ROUTEEDGEWRITER_H
