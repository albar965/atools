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

#include "fs/db/routeresolver.h"

#include "sql/sqldatabase.h"
#include "sql/sqlquery.h"
#include "sql/sqlutil.h"

#include <QString>
#include <QList>
#include <algorithm>
#include "logging/loggingdefs.h"

#include <QQueue>

namespace atools {
namespace fs {
namespace db {
using atools::sql::SqlDatabase;
using atools::sql::SqlQuery;
using atools::sql::SqlUtil;

uint qHash(const RouteResolver::Leg& leg)
{
  return leg.from ^ leg.to ^ leg.minAlt ^ qHash(leg.type);
}

QDebug operator<<(QDebug out, const RouteResolver::Leg& l)
{
  out << "Leg[from " << l.from << ", to " << l.to << ", alt " << l.minAlt << ", type " << l.type << "]";
  return out;
}

RouteResolver::RouteResolver(SqlDatabase& sqlDb)
  : curRouteId(0), numRoutes(0), routeInsertStmt(sqlDb), db(sqlDb)
{
  SqlUtil util(&sqlDb);
  routeInsertStmt.prepare(util.buildInsertStatement("route"));
}

void RouteResolver::writeRoute(const QString& routeName, QSet<Leg>& route)
{
  QQueue<Leg> newRoute;

  QHash<int, Leg> legByFrom;
  QHash<int, Leg> legByTo;

  for(Leg it : route)
  {
    legByFrom[it.from] = it;
    legByTo[it.to] = it;
  }

  int fragmentNum = 0;
  QHash<int, Leg>::const_iterator it;
  Leg leg;

  while(!route.empty())
  {
    newRoute.clear();

    leg = *route.begin();
    route.erase(route.begin());
    newRoute.push_back(leg);

    bool foundTo, foundFrom;

    do
    {
      foundTo = false;
      foundFrom = false;

      leg = newRoute.front();
      it = legByTo.find(leg.from);
      if(it != legByTo.end() && route.find(it.value()) != route.end())
      {
        leg = it.value();
        newRoute.push_front(leg);

        route.erase(route.find(leg));
        foundTo = true;
      }
      leg = newRoute.back();
      it = legByFrom.find(leg.to);
      if(it != legByFrom.end() && route.find(it.value()) != route.end())
      {
        leg = it.value();
        newRoute.push_back(leg);

        route.erase(route.find(leg));
        foundFrom = true;
      }
    } while(foundTo || foundFrom);

    Leg last;
    int seqNo = 0;
    ++fragmentNum;
    for(Leg rt : newRoute)
    {
      last = rt;

      routeInsertStmt.bindValue(":route_id", ++curRouteId);
      routeInsertStmt.bindValue(":route_name", routeName);
      routeInsertStmt.bindValue(":route_type", rt.type);
      routeInsertStmt.bindValue(":route_fragment_no", fragmentNum);
      routeInsertStmt.bindValue(":sequence_no", ++seqNo);
      routeInsertStmt.bindValue(":from_waypoint_id", rt.from);
      routeInsertStmt.bindValue(":to_waypoint_id", rt.to);
      routeInsertStmt.bindValue(":minimum_altitude", rt.minAlt);

      routeInsertStmt.exec();
      numRoutes += routeInsertStmt.numRowsAffected();
    }
  }
}

RouteResolver::~RouteResolver()
{
}

void RouteResolver::run()
{
  static QString queryStr(
    "select r.name, r.type, "
    "  prev.waypoint_id as prev_waypoint_id, "
    "  r.previous_minimum_altitude, "
    "  r.waypoint_id, "
    "  next.waypoint_id as next_waypoint_id, "
    "  r.next_minimum_altitude "
    "from route_point r join waypoint w on r.waypoint_id = w.waypoint_id "
    "  left outer join waypoint prev on r.previous_ident = prev.ident and r.previous_region = prev.region "
    "  left outer join waypoint next on r.next_ident = next.ident and r.next_region = next.region "
    "order by r.name");

  SqlQuery stmt(db);
  stmt.exec("delete from route");
  int deleted = stmt.numRowsAffected();
  qInfo() << "Removed" << deleted << "from route table";

  QSet<Leg> route;
  QString curRoute;

  stmt.exec(queryStr);
  while(stmt.next())
  {
    QString rName = stmt.value("name").toString();
    QString rType = stmt.value("type").toString();

    if(curRoute.isEmpty() || rName.at(0) != curRoute.at(0))
    {
      db.commit();
      qInfo() << rName << "...";
    }
    if(rName != curRoute)
    {
      if(!route.empty())
      {
        writeRoute(curRoute, route);
        route.clear();
      }
      curRoute = rName;
    }

    int id = stmt.value("waypoint_id").toInt();

    QVariant prevIdColVal = stmt.value("prev_waypoint_id");
    int prevMinAltColVal = stmt.value("previous_minimum_altitude").toInt();

    QVariant nextIdColVal = stmt.value("next_waypoint_id");
    int nextMinAltColVal = stmt.value("next_minimum_altitude").toInt();

    if(!prevIdColVal.isNull())
      route.insert(Leg(prevIdColVal.toInt(), id, prevMinAltColVal, rType));

    if(!nextIdColVal.isNull())
      route.insert(Leg(id, nextIdColVal.toInt(), nextMinAltColVal, rType));
  }

  qInfo() << "Added " << numRoutes << " route legs";
}

} // namespace writer
} // namespace fs
} // namespace atools
