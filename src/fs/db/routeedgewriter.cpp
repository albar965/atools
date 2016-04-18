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
const int MAX_RADIO_RANGE_METER = atools::geo::nmToMeter(100);
// Mininum nodes to find before extending the rectangle
const int MIN_NODES = 24;
// Increase search rectangle by this factor
const int RECT_SIZE_INCREASE = 2;

RouteEdgeWriter::RouteEdgeWriter(atools::sql::SqlDatabase *sqlDb)
  : db(sqlDb)
{

}

void RouteEdgeWriter::run()
{
  SqlQuery selectStmt("select node_id, range, type, lonx, laty from route_node", db);

  SqlQuery insertStmt(db);
  insertStmt.prepare("insert into route_edge (from_node_id, from_node_type, to_node_id, to_node_type) "
                     "values(?, ?, ?, ?)");

  SqlQuery nearestStmt(db);
  nearestStmt.prepare(
    "select node_id, type, range, lonx, laty from route_node "
    "where lonx between :leftx and :rightx and laty between :bottomy and :topy");

  selectStmt.exec();
  QVariantList toNodeIdVars;
  QVariantList toNodeTypeVars;
  QVariantList fromNodeIdVars;
  QVariantList fromNodeTypeVars;

  int average = 0, total = 0, maximum = 0, numEmpty = 0;
  while(selectStmt.next())
  {
    int fromRangeMeter = atools::geo::nmToMeter(selectStmt.value("range").toInt());
    int fromNodeId = selectStmt.value("node_id").toInt();
    int fromNodeType = selectStmt.value("type").toInt();

    Pos pos(selectStmt.value("lonx").toFloat(), selectStmt.value("laty").toFloat());
    int queryRectRadiusMeter = fromRangeMeter + MAX_RADIO_RANGE_METER;
    Rect queryRect(pos, queryRectRadiusMeter);

    toNodeIdVars.clear();
    toNodeTypeVars.clear();
    nearest(nearestStmt, pos, queryRect, fromRangeMeter, 1, toNodeIdVars, toNodeTypeVars);

    int increase = RECT_SIZE_INCREASE;
    while(toNodeIdVars.size() < MIN_NODES)
    {
      toNodeIdVars.clear();
      toNodeTypeVars.clear();
      queryRect = Rect(pos, queryRectRadiusMeter * increase);
      nearest(nearestStmt, pos, queryRect, fromRangeMeter, increase, toNodeIdVars, toNodeTypeVars);
      increase += RECT_SIZE_INCREASE;
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

void RouteEdgeWriter::nearest(SqlQuery& nearestStmt, const Pos& pos, const Rect& queryRect,
                              int fromRangeMeter, int rangeScale, QVariantList& toNodeIds,
                              QVariantList& toNodeTypes)
{
  for(const Rect& rect : queryRect.splitAtAntiMeridian())
  {
    bindCoordinatePointInRect(rect, &nearestStmt);

    nearestStmt.exec();

    while(nearestStmt.next())
    {
      int toRangeMeter = atools::geo::nmToMeter(nearestStmt.value("range").toInt());

      Pos toPos(nearestStmt.value("lonx").toFloat(), nearestStmt.value("laty").toFloat());
      int distanceMeter = static_cast<int>(pos.distanceMeterTo(toPos));
      // int courseDeg = static_cast<int>(pos.angleDegTo(toPos));
      // TODO use only the nearest ids per octant

      if((toRangeMeter + fromRangeMeter) * rangeScale > distanceMeter)
      {
        toNodeIds.append(nearestStmt.value("node_id"));
        toNodeTypes.append(nearestStmt.value("type"));
      }
    }
  }
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
