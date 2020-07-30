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

#include "routeedgewriter.h"

#include "sql/sqldatabase.h"
#include "geo/pos.h"
#include "geo/rect.h"
#include "geo/calculations.h"
#include "sql/sqlutil.h"
#include "sql/sqlquery.h"
#include "fs/progresshandler.h"

#include <QElapsedTimer>

namespace atools {
namespace fs {
namespace db {

using atools::sql::SqlDatabase;
using atools::sql::SqlUtil;
using atools::sql::SqlQuery;
using atools::geo::Pos;
using atools::geo::Rect;

/* Added to range of current radio id */
const int MAX_RADIO_RANGE_METER = atools::geo::nmToMeter(200);

/* Minimum distance that is needed before an edge is generated*/
const int MIN_DISTANCE_METER = atools::geo::nmToMeter(30);

/* Number of sectors around the navaid. */
const int NUM_SECTORS = 8;

/* Stop the process for one navaid when all sectors have at least this amount of neighbours */
const int MAX_EDGES_PER_SECTOR = 2;

/* Increase search radius around a navaid and search again until we find at least this amount of neigbours s  */
const int MIN_EDGES_PER_SECTOR = 1;

/* Increase rectangle at a maximum of this number */
const int MAX_ITERATIONS = 2;

/* Prioritize navaids by type - Index VOR=0, VORDME=1, DME=2, NDB=3 */
const int PRIORITY_BY_TYPE[] = {0 /* None */, 2 /* VOR */, 3 /* VORDME */, 0 /* DME */, 1 /* NDB */};

/* Inflate seach rectangle for each iteration.  */
const float INFLATE_RECT_LON_DEGREES = 6.f;
const float INFLATE_RECT_LAT_DEGREES = 4.f;

// Query result column indexes
enum ColumnIndex
{
  NODE_ID, RANGE, TYPE, LONX, LATY
};

enum BindColumnIndex
{
  LEFTX,
  RIGHTX,
  BOTTOMY,
  TOPY
};

RouteEdgeWriter::RouteEdgeWriter(atools::sql::SqlDatabase *sqlDb)
  : db(sqlDb)
{

}

void RouteEdgeWriter::run()
{
  // Iterate over all radio navaids
  SqlQuery selectNodesQuery("select node_id, range, type, lonx, laty from route_node_radio", db);

  // Get all nodes nearby this navaids
  SqlQuery nearestNodesQuery(db);
  nearestNodesQuery.prepare("select node_id, range, type, lonx, laty from route_node_radio "
                            "where lonx between ? and ? and laty between ? and ?");

  // Insert edges into databases
  SqlQuery insertEdgesQuery(db);
  insertEdgesQuery.prepare("insert into route_edge_radio "
                           "(from_node_id, from_node_type, to_node_id, to_node_type, distance) "
                           "values(?, ?, ?, ?, ?)");

  // Clean the result table
  SqlQuery stmt(db);
  stmt.exec("delete from route_edge_radio");
  int deleted = stmt.numRowsAffected();
  qInfo() << "Removed" << deleted << "from route_edge_radio table";

  QVariantList toNodeIdVars, toNodeTypeVars, toNodeDistanceVars, fromNodeIdVars, fromNodeTypeVars;

  int average = 0, total = 0, maximum = 0, numEmpty = 0;

  selectNodesQuery.exec();
  while(selectNodesQuery.next())
  {
    // Look at each node
    int fromRangeMeter = selectNodesQuery.value(RANGE).toInt();
    int fromNodeId = selectNodesQuery.value(NODE_ID).toInt();
    int fromNodeType = selectNodesQuery.value(TYPE).toInt();

    Pos pos(selectNodesQuery.value(LONX).toFloat(), selectNodesQuery.value(LATY).toFloat());

    // Clear result lists
    toNodeIdVars.clear();
    toNodeTypeVars.clear();
    toNodeDistanceVars.clear();

    // Get all navaids in bounding rectangle - first iterations
    Rect queryRect(pos, MAX_RADIO_RANGE_METER);
    bool nearestSatisfied = nearest(nearestNodesQuery, fromNodeId, pos, queryRect, fromRangeMeter,
                                    toNodeIdVars, toNodeTypeVars, toNodeDistanceVars);

    // If not all sectors have an edge increase rectangle and try again for MAX_ITERATIONS
    int maxIter = 0;
    while(!nearestSatisfied)
    {
      toNodeIdVars.clear();
      toNodeTypeVars.clear();
      toNodeDistanceVars.clear();
      queryRect.inflate(INFLATE_RECT_LON_DEGREES, INFLATE_RECT_LAT_DEGREES);
      nearestSatisfied = nearest(nearestNodesQuery, fromNodeId, pos, queryRect, fromRangeMeter,
                                 toNodeIdVars, toNodeTypeVars, toNodeDistanceVars);
      if(maxIter++ > MAX_ITERATIONS)
        break;
    }

    // Update from value arrays with values for bind
    fromNodeIdVars.clear();
    fromNodeTypeVars.clear();
    for(int i = 0; i < toNodeIdVars.size(); i++)
    {
      fromNodeIdVars.append(fromNodeId);
      fromNodeTypeVars.append(fromNodeType);
    }

    // Use batch update to insert values into edge table
    insertEdgesQuery.addBindValue(fromNodeIdVars);
    insertEdgesQuery.addBindValue(fromNodeTypeVars);
    insertEdgesQuery.addBindValue(toNodeIdVars);
    insertEdgesQuery.addBindValue(toNodeTypeVars);
    insertEdgesQuery.addBindValue(toNodeDistanceVars);
    insertEdgesQuery.execBatch();

    total += toNodeIdVars.size();
    average = (average + toNodeIdVars.size()) / 2;
    maximum = std::max(maximum, toNodeIdVars.size());

    if(toNodeIdVars.size() == 0)
      numEmpty++;
  }

  qDebug() << "Edge writer: total" << total << "average" << average
           << "max" << maximum << "numEmpty" << numEmpty;
}

/*
 * Get nearest neighbours for a navaid
 * @param nearestStmt
 * @param fromNodeId node ID for current navaid
 * @param pos position of current navaid
 * @param queryRect current query rectangle
 * @param fromRangeMeter range for current navaid
 * @param toNodeIds result list
 * @param toNodeTypes result list
 * @param toNodeDistances result list
 * @return
 */
bool RouteEdgeWriter::nearest(SqlQuery& nearestStmt, int fromNodeId, const Pos& pos, const Rect& queryRect,
                              int fromRangeMeter, QVariantList& toNodeIds,
                              QVariantList& toNodeTypes, QVariantList& toNodeDistances)
{
  struct TempNodeTo
  {
    int nodeId;
    int type; // VOR=1, VORDME=2, DME=3, NDB=4,
    int range;
    int distance;
    int priority;
  };

  // Nodes with reachable navaids - one list of nodes per sector
  QVector<QVector<TempNodeTo> > sectorsReachable;
  for(int i = 0; i < NUM_SECTORS; i++)
    sectorsReachable.append(QVector<TempNodeTo>());

  // Nodes with unreachable navaids - one list of nodes per sector
  QVector<QVector<TempNodeTo> > sectorsOther;
  for(int i = 0; i < NUM_SECTORS; i++)
    sectorsOther.append(QVector<TempNodeTo>());

  for(const Rect& rect : queryRect.splitAtAntiMeridian())
  {
    bindCoordinatePointInRect(rect, &nearestStmt);

    nearestStmt.exec();

    while(nearestStmt.next())
    {
      int toNodeId = nearestStmt.value(NODE_ID).toInt();
      if(toNodeId == fromNodeId)
        continue;

      int toRangeMeter = nearestStmt.value(RANGE).toInt();
      Pos toPos(nearestStmt.value(LONX).toFloat(), nearestStmt.value(LATY).toFloat());
      int distanceMeter = static_cast<int>(pos.distanceMeterTo(toPos) + 0.5f);

      if(distanceMeter < MIN_DISTANCE_METER)
        // Navaid is too close
        continue;

      int courseDeg = static_cast<int>(pos.angleDegTo(toPos) + 0.5f);
      if(courseDeg >= 360)
        courseDeg -= 360;

      // Calculate sector number for this node
      int sectorNum = courseDeg / (360 / NUM_SECTORS);

      int toNodeType = nearestStmt.value(TYPE).toInt();

      TempNodeTo tmp = {toNodeId, toNodeType, toRangeMeter, distanceMeter, PRIORITY_BY_TYPE[toNodeType]};

      bool reachable = fromRangeMeter + toRangeMeter > distanceMeter;

      QVector<TempNodeTo>::iterator it;

      if(reachable)
      {
        QVector<TempNodeTo>& sector = sectorsReachable[sectorNum];

        // farthest at beginning of list
        it = std::lower_bound(sector.begin(), sector.end(), tmp,
                              [](const TempNodeTo& n1, const TempNodeTo& n2) -> bool
              {
                if(n1.priority == n2.priority)
                  return n1.distance > n2.distance;
                else
                  return n1.priority > n2.priority;
              });
        sector.insert(it, tmp);
      }
      else
      {
        QVector<TempNodeTo>& sector = sectorsOther[sectorNum];

        // nearest at beginning of list
        it = std::lower_bound(sector.begin(), sector.end(), tmp,
                              [](const TempNodeTo& n1, const TempNodeTo& n2) -> bool
              {
                if(n1.priority == n2.priority)
                  return n1.distance < n2.distance;
                else
                  return n1.priority > n2.priority;
              });
        sector.insert(it, tmp);
      }
    }
  }

  bool retval = true;

  // Now check for each sector if conditions are met
  for(int sectorNum = 0; sectorNum < NUM_SECTORS; sectorNum++)
  {
    const QVector<TempNodeTo>& sectorReachable = sectorsReachable.value(sectorNum);
    const QVector<TempNodeTo>& sectorOther = sectorsOther.value(sectorNum);
    int numOtherEntries = std::min(sectorOther.size(), MAX_EDGES_PER_SECTOR);
    int numReachableEntries = std::min(sectorReachable.size(), MAX_EDGES_PER_SECTOR);

    // We need more nodes for this sector
    if(numOtherEntries + numReachableEntries < MIN_EDGES_PER_SECTOR)
      retval = false;

    // Add all reachable first for this sector
    for(int i = 0; i < numReachableEntries; i++)
    {
      const TempNodeTo& tn = sectorReachable.at(i);
      toNodeIds.append(tn.nodeId);
      toNodeTypes.append(tn.type);
      toNodeDistances.append(tn.distance);
    }

    // Then add the unreachable
    for(int i = 0; i < numOtherEntries; i++)
    {
      const TempNodeTo& tn = sectorOther.at(i);
      toNodeIds.append(tn.nodeId);
      toNodeTypes.append(tn.type);
      toNodeDistances.append(tn.distance);
    }
  }
  return retval;
}

void RouteEdgeWriter::bindCoordinatePointInRect(const atools::geo::Rect& rect, atools::sql::SqlQuery *query)
{
  query->bindValue(LEFTX, rect.getWest());
  query->bindValue(RIGHTX, rect.getEast());
  query->bindValue(BOTTOMY, rect.getSouth());
  query->bindValue(TOPY, rect.getNorth());
}

} // namespace writer
} // namespace fs
} // namespace atools
