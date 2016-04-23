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

#include "routeedgewriter.h"

#include "sql/sqldatabase.h"
#include "geo/pos.h"
#include "geo/rect.h"
#include "geo/calculations.h"
#include "sql/sqlutil.h"

namespace atools {
namespace fs {
namespace db {

using atools::sql::SqlDatabase;
using atools::sql::SqlQuery;
using atools::sql::SqlUtil;
using atools::geo::Pos;
using atools::geo::Rect;

// Added to range of current radio id
const int MAX_RADIO_RANGE_METER = atools::geo::nmToMeter(200);
const int NUM_SECTORS = 8;

// Increase rectangle at a maximum of this number
const int MAX_ITERATIONS = 2;

// Minimum distance that is needed before an edge is generated
const int MIN_DISTANCE_METER = atools::geo::nmToMeter(30);

const int MAX_EDGES_PER_SECTOR = 4;
const int MIN_EDGES_PER_SECTOR = 1;

// Index VOR=0, VORDME=1, DME=2, NDB=3
// const int PRIORITY_BY_TYPE[] = {2 /* VOR */, 3 /* VORDME */, 0 /* DME */, 1 /* NDB */};

const float INFLATE_RECT_DEGREES = 4.f;

RouteEdgeWriter::RouteEdgeWriter(atools::sql::SqlDatabase *sqlDb)
  : db(sqlDb)
{

}

void RouteEdgeWriter::run()
{
  SqlQuery selectStmt(
    "select node_id, range, type, lonx, laty from route_node_radio", db);

  SqlQuery insertStmt(db);
  insertStmt.prepare(
    "insert into route_edge_radio (from_node_id, from_node_type, to_node_id, to_node_type, distance) "
    "values(?, ?, ?, ?, ?)");

  SqlQuery nearestStmt(db);
  nearestStmt.prepare(
    "select node_id, type, range, lonx, laty from route_node_radio "
    "where lonx between :leftx and :rightx and laty between :bottomy and :topy");

  selectStmt.exec();
  QVariantList toNodeIdVars, toNodeTypeVars, toNodeDistanceVars, fromNodeIdVars, fromNodeTypeVars;

  int average = 0, total = 0, maximum = 0, numEmpty = 0;
  while(selectStmt.next())
  {
    // Look at each node
    int fromRangeMeter = selectStmt.value("range").toInt();
    int fromNodeId = selectStmt.value("node_id").toInt();
    int fromNodeType = selectStmt.value("type").toInt();

    Pos pos(selectStmt.value("lonx").toFloat(), selectStmt.value("laty").toFloat());

    toNodeIdVars.clear();
    toNodeTypeVars.clear();
    toNodeDistanceVars.clear();

    // Get all navaids in bounding rectangle
    Rect queryRect(pos, MAX_RADIO_RANGE_METER);
    bool nearestSatisfied = nearest(nearestStmt, pos, queryRect, fromRangeMeter,
                                    toNodeIdVars, toNodeTypeVars, toNodeDistanceVars);

    // If not all sectors have an edge increase rectangle and try again
    int maxIter = 0;
    while(!nearestSatisfied)
    {
      toNodeIdVars.clear();
      toNodeTypeVars.clear();
      toNodeDistanceVars.clear();
      queryRect.inflate(INFLATE_RECT_DEGREES);
      nearestSatisfied = nearest(nearestStmt, pos, queryRect, fromRangeMeter,
                                 toNodeIdVars, toNodeTypeVars, toNodeDistanceVars);
      if(maxIter++ > MAX_ITERATIONS)
        break;
    }

    fromNodeIdVars.clear();
    fromNodeTypeVars.clear();
    for(int i = 0; i < toNodeIdVars.size(); i++)
    {
      fromNodeIdVars.append(fromNodeId);
      fromNodeTypeVars.append(fromNodeType);
    }

    insertStmt.addBindValue(fromNodeIdVars);
    insertStmt.addBindValue(fromNodeTypeVars);
    insertStmt.addBindValue(toNodeIdVars);
    insertStmt.addBindValue(toNodeTypeVars);
    insertStmt.addBindValue(toNodeDistanceVars);
    insertStmt.execBatch();

    total += toNodeIdVars.size();
    average = (average + toNodeIdVars.size()) / 2;
    maximum = std::max(maximum, toNodeIdVars.size());

    if(toNodeIdVars.size() == 0)
      numEmpty++;
  }

  qDebug() << "Edge writer: total" << total << "average" << average
           << "max" << maximum << "numEmpty" << numEmpty;
}

bool RouteEdgeWriter::nearest(SqlQuery& nearestStmt, const Pos& pos, const Rect& queryRect,
                              int fromRangeMeter, QVariantList& toNodeIds,
                              QVariantList& toNodeTypes, QVariantList& toNodeDistances)
{
  struct TempNodeTo
  {
    int nodeId;
    int type; // VOR=0, VORDME=1, DME=2, NDB=3,
    int range;
    int distance;
  };

  QVector<QVector<TempNodeTo> > sectorsOther;
  for(int i = 0; i < NUM_SECTORS; i++)
    sectorsOther.append(QVector<TempNodeTo>());

  QVector<QVector<TempNodeTo> > sectorsReachable;
  for(int i = 0; i < NUM_SECTORS; i++)
    sectorsReachable.append(QVector<TempNodeTo>());

  for(const Rect& rect : queryRect.splitAtAntiMeridian())
  {
    bindCoordinatePointInRect(rect, &nearestStmt);

    nearestStmt.exec();

    while(nearestStmt.next())
    {
      int toRangeMeter = nearestStmt.value("range").toInt();

      Pos toPos(nearestStmt.value("lonx").toFloat(), nearestStmt.value("laty").toFloat());
      int distanceMeter = static_cast<int>(pos.distanceMeterTo(toPos) + 0.5f);

      if(distanceMeter < MIN_DISTANCE_METER)
        continue;

      int courseDeg = static_cast<int>(pos.angleDegTo(toPos) + 0.5f);
      if(courseDeg >= 360)
        courseDeg -= 360;

      int sectorNum = courseDeg / (360 / NUM_SECTORS);

      int toNodeId = nearestStmt.value("node_id").toInt();
      int toNodeType = nearestStmt.value("type").toInt();

      TempNodeTo tmp = {toNodeId, toNodeType, toRangeMeter, distanceMeter};

      bool reachable = fromRangeMeter + fromRangeMeter > distanceMeter;

      QVector<TempNodeTo>::iterator it;

      if(reachable)
      {
        QVector<TempNodeTo>& sector = sectorsReachable[sectorNum];
        // farthest at beginning of list
        it = std::lower_bound(sector.begin(), sector.end(), tmp,
                              [ = ](const TempNodeTo &n1, const TempNodeTo &n2)->bool
                              {
                                // int p1 = PRIORITY_BY_TYPE[n1.type];
                                // int p2 = PRIORITY_BY_TYPE[n2.type];
                                // if(p1 == p2)
                                return n1.distance > n2.distance;
                                // else
                                // return p1 > p2;
                              });
        sector.insert(it, tmp);
      }
      else
      {
        QVector<TempNodeTo>& sector = sectorsOther[sectorNum];
        // nearest at beginning of list
        it = std::lower_bound(sector.begin(), sector.end(), tmp,
                              [ = ](const TempNodeTo &n1, const TempNodeTo &n2)->bool
                              {
                                // int p1 = PRIORITY_BY_TYPE[n1.type];
                                // int p2 = PRIORITY_BY_TYPE[n2.type];
                                // if(p1 == p2)
                                return n1.distance < n2.distance;
                                // else
                                // return p1 > p2;
                              });
        sector.insert(it, tmp);
      }
    }
  }

  bool retval = true;

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
  query->bindValue(":leftx", rect.getWest());
  query->bindValue(":rightx", rect.getEast());
  query->bindValue(":bottomy", rect.getSouth());
  query->bindValue(":topy", rect.getNorth());
}

} // namespace writer
} // namespace fs
} // namespace atools
