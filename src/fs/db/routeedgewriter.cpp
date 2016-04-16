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

RouteEdgeWriter::RouteEdgeWriter(atools::sql::SqlDatabase *sqlDb)
  : db(sqlDb)
{

}

void RouteEdgeWriter::run()
{
  SqlQuery selectStmt("select node_id, range, lonx, laty from route_node", db);

  SqlQuery insertStmt(db);
  insertStmt.prepare("insert into route_edge (from_node_id, to_node_id) "
                     "values(:fromNodeId, :toNodeId)");

  SqlQuery nearestStmt(db);
  nearestStmt.prepare(
    "select node_id, range, lonx, laty from route_node "
    "where lonx between :leftx and :rightx and laty between :bottomy and :topy");

  selectStmt.exec();

  int maxRange = atools::geo::nmToMeter(200);

  int average = 0, total = 0, maximum = 0, numEmpty = 0;
  while(selectStmt.next())
  {
    int range = selectStmt.value("range").toInt();
    Pos pos(selectStmt.value("lonx").toFloat(), selectStmt.value("laty").toFloat());
    int queryRectRadius = atools::geo::nmToMeter(range) + maxRange;
    Rect queryRect(pos, queryRectRadius);

    int num = nearest(nearestStmt, pos, queryRect, selectStmt.value("node_id").toInt(), range, insertStmt);
    if(num == 0)
    {
      queryRect = Rect(pos, queryRectRadius * 4);
      num = nearest(nearestStmt, pos, queryRect, selectStmt.value("node_id").toInt(), range * 4, insertStmt);
    }

    total += num;
    average = (average + num) / 2;
    maximum = std::max(maximum, num);

    if(num == 0)
      numEmpty++;
  }

  qDebug() << "Edge writer: total" << total << "average" << average
           << "max" << maximum << "numEmpty" << numEmpty;
}

int RouteEdgeWriter::nearest(SqlQuery& nearestStmt, const Pos& pos, const Rect& queryRect,
                             int fromNodeId, int range, SqlQuery& insertStmt)
{
  int num = 0;
  for(const Rect& rect : queryRect.splitAtAntiMeridian())
  {
    bindCoordinatePointInRect(rect, &nearestStmt);

    nearestStmt.exec();

    while(nearestStmt.next())
    {
      int rng = nearestStmt.value("range").toInt();

      float dist = atools::geo::meterToNm(pos.distanceMeterTo(Pos(nearestStmt.value("lonx").toFloat(),
                                                                  nearestStmt.value("laty").toFloat())));

      if(rng + range < dist)
      {
        insertStmt.bindValue(":fromNodeId", fromNodeId);
        insertStmt.bindValue(":toNodeId", nearestStmt.value("node_id").toInt());
        insertStmt.exec();
        num++;
      }
    }
  }

  return num;
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
