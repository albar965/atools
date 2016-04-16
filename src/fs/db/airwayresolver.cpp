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

#include "fs/db/airwayresolver.h"

#include "sql/sqldatabase.h"
#include "sql/sqlquery.h"
#include "sql/sqlutil.h"
#include "geo/pos.h"
#include "geo/rect.h"
#include "logging/loggingdefs.h"

#include <QString>
#include <QList>
#include <algorithm>

#include <QQueue>

namespace atools {
namespace fs {
namespace db {

using atools::sql::SqlDatabase;
using atools::sql::SqlQuery;
using atools::sql::SqlUtil;
using atools::geo::Pos;
using atools::geo::Rect;

uint qHash(const AirwayResolver::Leg& leg)
{
  return leg.from ^ leg.to ^ leg.minAlt ^ qHash(leg.type);
}

QDebug operator<<(QDebug out, const AirwayResolver::Leg& l)
{
  out << "Leg[from " << l.from << ", to " << l.to << ", alt " << l.minAlt << ", type " << l.type << "]";
  return out;
}

AirwayResolver::AirwayResolver(sql::SqlDatabase *sqlDb)
  : curAirwayId(0), numAirways(0), airwayInsertStmt(sqlDb), db(sqlDb)
{
  SqlUtil util(sqlDb);
  airwayInsertStmt.prepare(util.buildInsertStatement("airway"));
}

void AirwayResolver::writeAirway(const QString& airwayName, QSet<Leg>& airway)
{
  QQueue<Leg> newAirway;

  QHash<int, Leg> legByFrom;
  QHash<int, Leg> legByTo;

  for(Leg it : airway)
  {
    legByFrom[it.from] = it;
    legByTo[it.to] = it;
  }

  int fragmentNum = 0;
  QHash<int, Leg>::const_iterator it;
  Leg leg;

  while(!airway.empty())
  {
    newAirway.clear();

    leg = *airway.begin();
    airway.erase(airway.begin());
    newAirway.push_back(leg);

    bool foundTo, foundFrom;

    do
    {
      foundTo = false;
      foundFrom = false;

      leg = newAirway.front();
      it = legByTo.find(leg.from);
      if(it != legByTo.end() && airway.find(it.value()) != airway.end())
      {
        leg = it.value();
        newAirway.push_front(leg);

        airway.erase(airway.find(leg));
        foundTo = true;
      }
      leg = newAirway.back();
      it = legByFrom.find(leg.to);
      if(it != legByFrom.end() && airway.find(it.value()) != airway.end())
      {
        leg = it.value();
        newAirway.push_back(leg);

        airway.erase(airway.find(leg));
        foundFrom = true;
      }
    } while(foundTo || foundFrom);

    Leg last;
    int seqNo = 0;
    ++fragmentNum;
    for(Leg rt : newAirway)
    {
      last = rt;

      airwayInsertStmt.bindValue(":airway_id", ++curAirwayId);
      airwayInsertStmt.bindValue(":airway_name", airwayName);
      airwayInsertStmt.bindValue(":airway_type", rt.type);
      airwayInsertStmt.bindValue(":airway_fragment_no", fragmentNum);
      airwayInsertStmt.bindValue(":sequence_no", ++seqNo);
      airwayInsertStmt.bindValue(":from_waypoint_id", rt.from);
      airwayInsertStmt.bindValue(":to_waypoint_id", rt.to);
      airwayInsertStmt.bindValue(":minimum_altitude", rt.minAlt);

      Rect bounding(rt.fromPos);
      bounding.extend(rt.toPos);
      airwayInsertStmt.bindValue(":left_lonx", bounding.getTopLeft().getLonX());
      airwayInsertStmt.bindValue(":top_laty", bounding.getTopLeft().getLatY());
      airwayInsertStmt.bindValue(":right_lonx", bounding.getBottomRight().getLonX());
      airwayInsertStmt.bindValue(":bottom_laty", bounding.getBottomRight().getLatY());

      airwayInsertStmt.bindValue(":from_lonx", rt.fromPos.getLonX());
      airwayInsertStmt.bindValue(":from_laty", rt.fromPos.getLatY());
      airwayInsertStmt.bindValue(":to_lonx", rt.toPos.getLonX());
      airwayInsertStmt.bindValue(":to_laty", rt.toPos.getLatY());

      airwayInsertStmt.exec();
      numAirways += airwayInsertStmt.numRowsAffected();
    }
  }
}

AirwayResolver::~AirwayResolver()
{
}

void AirwayResolver::run()
{
  static QString queryStr(
    "select r.name, r.type, "
    "  prev.waypoint_id as prev_waypoint_id, "
    "  r.previous_minimum_altitude, "
    "  prev.lonx as prev_lonx, "
    "  prev.laty as prev_laty, "
    "  r.waypoint_id, "
    "  w.lonx as lonx, "
    "  w.laty as laty, "
    "  next.waypoint_id as next_waypoint_id, "
    "  r.next_minimum_altitude, "
    "  next.lonx as next_lonx, "
    "  next.laty as next_laty "
    "from airway_point r join waypoint w on r.waypoint_id = w.waypoint_id "
    "  left outer join waypoint prev on r.previous_ident = prev.ident and r.previous_region = prev.region "
    "  left outer join waypoint next on r.next_ident = next.ident and r.next_region = next.region "
    "order by r.name");

  SqlQuery stmt(db);
  stmt.exec("delete from airway");
  int deleted = stmt.numRowsAffected();
  qInfo() << "Removed" << deleted << "from airway table";

  QSet<Leg> airway;
  QString curAirway;

  stmt.exec(queryStr);
  while(stmt.next())
  {
    QString rName = stmt.value("name").toString();
    QString rType = stmt.value("type").toString();

    if(curAirway.isEmpty() || rName.at(0) != curAirway.at(0))
    {
      db->commit();
      qInfo() << rName << "...";
    }
    if(rName != curAirway)
    {
      if(!airway.empty())
      {
        writeAirway(curAirway, airway);
        airway.clear();
      }
      curAirway = rName;
    }

    int curId = stmt.value("waypoint_id").toInt();
    Pos curPos(stmt.value("lonx").toFloat(), stmt.value("laty").toFloat());

    QVariant prevIdColVal = stmt.value("prev_waypoint_id");
    int prevMinAlt = stmt.value("previous_minimum_altitude").toInt();

    QVariant nextIdColVal = stmt.value("next_waypoint_id");
    int nextMinAlt = stmt.value("next_minimum_altitude").toInt();

    if(!prevIdColVal.isNull())
    {
      Pos prevPos(stmt.value("prev_lonx").toFloat(), stmt.value("prev_laty").toFloat());
      airway.insert(Leg(prevIdColVal.toInt(), curId, prevMinAlt, rType, prevPos, curPos));
    }

    if(!nextIdColVal.isNull())
    {
      Pos nextPos(stmt.value("next_lonx").toFloat(), stmt.value("next_laty").toFloat());
      airway.insert(Leg(curId, nextIdColVal.toInt(), nextMinAlt, rType, curPos, nextPos));
    }
  }

  qInfo() << "Added " << numAirways << " airway legs";
}

} // namespace writer
} // namespace fs
} // namespace atools
