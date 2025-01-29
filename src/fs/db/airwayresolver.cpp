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

#include "fs/db/airwayresolver.h"

#include "sql/sqldatabase.h"
#include "sql/sqlquery.h"
#include "sql/sqlutil.h"
#include "geo/pos.h"
#include "geo/rect.h"
#include "geo/calculations.h"
#include "fs/progresshandler.h"

#include <QDebug>
#include <QString>
#include <QList>
#include <algorithm>
#include <QQueue>
#include <QElapsedTimer>
#include <QStringBuilder>

namespace atools {
namespace fs {
namespace db {

/* Report progress twice a second */
const static int MIN_PROGRESS_REPORT_MS = 500;

using atools::sql::SqlDatabase;
using atools::sql::SqlQuery;
using atools::sql::SqlUtil;
using atools::geo::Pos;
using atools::geo::Rect;

/* Airway segment with from/to position and IDs */
struct AirwayResolver::AirwaySegment
{
  AirwaySegment()
  {

  }

  explicit AirwaySegment(int fromId, int toId, char direction, int minAltitude, int maxAltitude, QString airwayType,
                         const atools::geo::Pos& fromPosition, const atools::geo::Pos& toPosition)
    : type(airwayType), dir(direction), fromWaypointId(fromId), toWaypointId(toId),
    minAlt(minAltitude), maxAlt(maxAltitude), fromPos(fromPosition), toPos(toPosition)
  {
  }

  bool operator==(const AirwaySegment& other) const
  {
    return fromWaypointId == other.fromWaypointId && toWaypointId == other.toWaypointId;

  }

  bool operator<(const AirwaySegment& other) const
  {
    return std::pair<int, int>(fromWaypointId, toWaypointId) < std::pair<int, int>(other.fromWaypointId, other.toWaypointId);
  }

  QString type;
  char dir = '\0';
  int fromWaypointId = 0, toWaypointId = 0, minAlt = 0, maxAlt;
  atools::geo::Pos fromPos, toPos;
};

uint qHash(const AirwayResolver::AirwaySegment& segment)
{
  return static_cast<unsigned int>(segment.fromWaypointId) ^ static_cast<unsigned int>(segment.toWaypointId);
}

AirwayResolver::AirwayResolver(sql::SqlDatabase *sqlDb, atools::fs::ProgressHandler& progress)
  : progressHandler(progress), curAirwayId(1), numAirways(0), airwayInsertStmt(sqlDb), db(sqlDb)
{
  SqlUtil util(sqlDb);
  airwayInsertStmt.prepare(util.buildInsertStatement("airway"));
}

AirwayResolver::~AirwayResolver()
{
}

void AirwayResolver::assignWaypointIds()
{
  SqlQuery query(db);

  query.exec("insert into tmp_waypoint "
             "select waypoint_id, "
             "case when type == 'V' then 'V' when type == 'N' then 'N' else 'O' end as type, "
             "ident, region, lonx, laty "
             "from waypoint");

  query.exec("analyze tmp_waypoint");
}

bool AirwayResolver::run(int numReportSteps)
{
  bool aborted = false;

  // Clean the table
  SqlQuery deleteAirwayQuery(db);
  deleteAirwayQuery.exec("delete from airway");
  int deleted = deleteAirwayQuery.numRowsAffected();
  qInfo() << "Removed" << deleted << "from airway table";

  // Use set to
  QSet<AirwaySegment> airway;
  QString currentAirway;

  int totalRowCount = SqlUtil(db).rowCount("tmp_airway_point");

  int rowsPerStep = static_cast<int>(std::ceil(static_cast<float>(totalRowCount) / static_cast<float>(numReportSteps)));
  int row = 0, steps = 0;

  QElapsedTimer timer;
  timer.start();
  qint64 elapsed = timer.elapsed();

  SqlQuery tmpWaypointQuery(db);
  tmpWaypointQuery.prepare("select waypoint_id, ident, region, type, lonx, laty "
                           "from tmp_waypoint where ident = ? and region = ? and type = ?");

  // Get all tmp_airway_point rows and join previous and next waypoints to the result by ident and region
  // Result is ordered by airway name
  SqlQuery tmpAirwayPointQuery(db);
  tmpAirwayPointQuery.exec("select * from tmp_airway_point order by name"); // where name = 'Y655'
  atools::geo::Pos lastPosition;
  float longestAirwaySegmentMeter = 0.f;
  while(tmpAirwayPointQuery.next())
  {
    QString awName = tmpAirwayPointQuery.value("name").toString();
    QString awType = tmpAirwayPointQuery.value("type").toString();

    if((row++ % rowsPerStep) == 0)
    {
      qint64 elapsed2 = timer.elapsed();

      // Update only every 500 ms - otherwise update only progress count
      bool silent = !(elapsed + MIN_PROGRESS_REPORT_MS < elapsed2);
      if(!silent)
        elapsed = elapsed2;
      steps++;
      if((aborted = progressHandler.reportOther(tr("Creating airways: %1...").arg(awName), -1, silent)) == true)
        break;
    }

    if(awName != currentAirway)
    {
      // A new airway comes from from the query save the current one to the database
      saveAirway(airway, currentAirway);
      currentAirway = awName;
      lastPosition = Pos();
    }

    int midWpId = -1, prevWpId = -1, nextWpId = -1;
    Pos midWpPos, prevWpPos, nextWpPos;
    fetchNavaid(prevWpId, prevWpPos, tmpAirwayPointQuery, tmpWaypointQuery, "previous_", lastPosition);
    if(prevWpPos.isValidRange())
      lastPosition = prevWpPos;

    fetchNavaid(midWpId, midWpPos, tmpAirwayPointQuery, tmpWaypointQuery, "mid_", lastPosition);
    if(midWpPos.isValidRange())
      lastPosition = midWpPos;

    fetchNavaid(nextWpId, nextWpPos, tmpAirwayPointQuery, tmpWaypointQuery, "next_", lastPosition);
    if(nextWpPos.isValidRange())
      lastPosition = nextWpPos;

    if(prevWpId != -1)
    {
      // Previous waypoint found - add segment
      float midPrevDist = midWpPos.distanceMeterTo(prevWpPos);
      if(maxAirwaySegmentLengthNm <= 1.f || midPrevDist < atools::geo::nmToMeter(maxAirwaySegmentLengthNm))
      {
        int prevMinAlt = tmpAirwayPointQuery.value("previous_minimum_altitude").toInt();
        int prevMaxAlt = tmpAirwayPointQuery.value("previous_maximum_altitude").toInt();
        char prevDir = atools::strToChar(tmpAirwayPointQuery.value("previous_direction").toString());
        airway.insert(AirwaySegment(prevWpId, midWpId, prevDir, prevMinAlt, prevMaxAlt, awType, prevWpPos, midWpPos));
      }

      longestAirwaySegmentMeter = std::max(longestAirwaySegmentMeter, midPrevDist);
    }

    if(nextWpId != -1)
    {
      // Next waypoint found - add segment
      float midNextDist = midWpPos.distanceMeterTo(nextWpPos);
      if(maxAirwaySegmentLengthNm <= 1.f || midNextDist < atools::geo::nmToMeter(maxAirwaySegmentLengthNm))
      {
        int nextMinAlt = tmpAirwayPointQuery.value("next_minimum_altitude").toInt();
        int nextMaxAlt = tmpAirwayPointQuery.value("next_maximum_altitude").toInt();
        char nextDir = atools::strToChar(tmpAirwayPointQuery.value("next_direction").toString());
        airway.insert(AirwaySegment(midWpId, nextWpId, nextDir, nextMinAlt, nextMaxAlt, awType, midWpPos, nextWpPos));
      }

      longestAirwaySegmentMeter = std::max(longestAirwaySegmentMeter, midNextDist);
    }
  } // while(query.next())

  // Save last remaining airway
  saveAirway(airway, currentAirway);

  // Eat up any remaining progress steps
  progressHandler.increaseCurrent(numReportSteps - steps);

  qInfo() << Q_FUNC_INFO << "Added " << numAirways << " airway segments";
  qInfo() << Q_FUNC_INFO << "Longest segment is" << atools::geo::meterToNm(longestAirwaySegmentMeter) << "NM";

  if(!aborted)
    db->commit();

  return aborted;
}

void AirwayResolver::fetchNavaid(int& id, atools::geo::Pos& pos, atools::sql::SqlQuery& tmpAirwayPointQuery,
                                 atools::sql::SqlQuery& tmpWaypointQuery, const QString& prefix, const Pos& lastPos)
{
  enum {BIND_IDENT, BIND_REGION, BIND_TYPE};

  enum {WAYPOINT_ID, IDENT, REGION, TYPE, LONX, LATY};

  // waypoint_id integer primary key,
  // type varchar(15) not null,
  // ident varchar(5) not null,
  // region varchar(2) not null,
  // lonx double not null,
  // laty double not null
  tmpWaypointQuery.bindValue(BIND_IDENT, tmpAirwayPointQuery.valueStr(prefix % "ident"));
  tmpWaypointQuery.bindValue(BIND_REGION, tmpAirwayPointQuery.valueStr(prefix % "region"));
  tmpWaypointQuery.bindValue(BIND_TYPE, tmpAirwayPointQuery.valueStr(prefix % "type"));
  tmpWaypointQuery.exec();

  QVector<std::pair<int, Pos> > wpList;
  while(tmpWaypointQuery.next())
    wpList.append(std::make_pair(tmpWaypointQuery.valueInt(WAYPOINT_ID),
                                 Pos(tmpWaypointQuery.valueFloat(LONX), tmpWaypointQuery.valueFloat(LATY))));

  if(lastPos.isValidRange() && wpList.size() > 1)
    std::sort(wpList.begin(), wpList.end(), [&lastPos](const std::pair<int, Pos>& t1, const std::pair<int, Pos>& t2) -> bool {
            return t1.second.distanceMeterTo(lastPos) < t2.second.distanceMeterTo(lastPos);
          });

  if(!wpList.isEmpty())
  {
    id = wpList.constFirst().first;
    pos = wpList.constFirst().second;
  }
  else
  {
    id = -1;
    pos = Pos();
  }
}

void AirwayResolver::saveAirway(QSet<AirwaySegment>& airway, const QString& currentAirway)
{
  if(!airway.empty())
  {
    // Build airway fragments
    QVector<Fragment> fragments;
    buildAirway(currentAirway, airway, fragments);

    // Remove all fragments that are contained by others
    cleanFragments(fragments);

    for(const Fragment& fragment : qAsConst(fragments))
    {
      for(const TypeRowValueVector& bindRow : fragment.boundValues)
      {
        airwayInsertStmt.bindValues(bindRow);
        airwayInsertStmt.exec();
        numAirways += airwayInsertStmt.numRowsAffected();
      }
    }
    airway.clear();
  }
}

void AirwayResolver::buildAirway(const QString& airwayName, QSet<AirwaySegment>& airway, QVector<Fragment>& fragments)
{
  // Queue of waypoints that will get waypoints in order prependend and appendend
  QQueue<AirwaySegment> newAirway;

  // Segments indexed by from waypoint ID
  QHash<int, AirwaySegment> segsByFromWpId;
  // Segments indexed by to waypoint ID
  QHash<int, AirwaySegment> segsByToWpId;

  // Fill the index
  for(const AirwaySegment& segment : qAsConst(airway))
  {
    segsByFromWpId[segment.fromWaypointId] = segment;
    segsByToWpId[segment.toWaypointId] = segment;
  }

  int fragmentNum = 1;
  AirwaySegment segment;

  // Iterator over all waypoints in the airway which are neither ordered nor connected yet
  // All waypoints in airway have same airway name
  while(!airway.empty())
  {
    newAirway.clear();

    // Take a random waypoint from the unordered airway and add it to the queue
    segment = *airway.constBegin();
    airway.erase(airway.constBegin());
    newAirway.append(segment);

    bool foundTo, foundFrom;

    // Now collect predecessors and successors for all waypoints
    do
    {
      foundTo = false;
      foundFrom = false;

      // Take a segment from the front of the queue and find predecessors
      segment = newAirway.front();
      auto it = segsByToWpId.constFind(segment.fromWaypointId);
      if(it != segsByToWpId.constEnd() && airway.constFind(it.value()) != airway.constEnd())
      {
        // Found a predecessor in the index - add it to the new airway and remove it from the queue
        segment = it.value();
        newAirway.prepend(segment);

        airway.erase(airway.find(segment));
        foundTo = true;
      }

      // Take a segment from the end of the queue and find successors
      segment = newAirway.back();
      it = segsByFromWpId.constFind(segment.toWaypointId);
      if(it != segsByFromWpId.constEnd() && airway.constFind(it.value()) != airway.constEnd())
      {
        // Found a successor in the index - add it to the new airway and remove it from the queue
        segment = it.value();
        newAirway.append(segment);

        airway.erase(airway.constFind(segment));
        foundFrom = true;
      }
    } while(foundTo || foundFrom);

    // Write airway fragment - there may be more fragments for the same airway name
    AirwaySegment last;
    int seqNo = 1;
    Fragment fragment;

    for(const AirwaySegment& newSegment : qAsConst(newAirway))
    {
      last = newSegment;

      fragment.waypoints.insert(newSegment.fromWaypointId);
      fragment.waypoints.insert(newSegment.toWaypointId);

      // Create bounding rect for this segment
      Rect bounding(newSegment.fromPos);
      bounding.extend(newSegment.toPos);

      TypeRowValueVector row;

      row.append(std::make_pair(":airway_id", curAirwayId));
      row.append(std::make_pair(":airway_name", airwayName));
      row.append(std::make_pair(":airway_type", newSegment.type));
      // route_type varchar(5), unused

      row.append(std::make_pair(":airway_fragment_no", fragmentNum));
      row.append(std::make_pair(":sequence_no", seqNo));

      row.append(std::make_pair(":from_waypoint_id", newSegment.fromWaypointId));
      row.append(std::make_pair(":to_waypoint_id", newSegment.toWaypointId));

      row.append(std::make_pair(":direction", atools::charToStr(newSegment.dir)));
      row.append(std::make_pair(":minimum_altitude", newSegment.minAlt));
      row.append(std::make_pair(":maximum_altitude", newSegment.maxAlt));
      row.append(std::make_pair(":left_lonx", bounding.getTopLeft().getLonX()));
      row.append(std::make_pair(":top_laty", bounding.getTopLeft().getLatY()));
      row.append(std::make_pair(":right_lonx", bounding.getBottomRight().getLonX()));
      row.append(std::make_pair(":bottom_laty", bounding.getBottomRight().getLatY()));

      // Write start and end coordinates for this segment
      row.append(std::make_pair(":from_lonx", newSegment.fromPos.getLonX()));
      row.append(std::make_pair(":from_laty", newSegment.fromPos.getLatY()));
      row.append(std::make_pair(":to_lonx", newSegment.toPos.getLonX()));
      row.append(std::make_pair(":to_laty", newSegment.toPos.getLatY()));

      fragment.boundValues.append(row);

      seqNo++;
      curAirwayId++;
    }
    fragments.append(fragment);

    fragmentNum++;
  }
}

void AirwayResolver::cleanFragments(QVector<Fragment>& fragments)
{
  // Erase empty segments
  fragments.erase(std::remove_if(fragments.begin(), fragments.end(), [](const Fragment& f) -> bool {
          return f.waypoints.size() < 2;
        }), fragments.end());

  // Erase all segments that are contained by another
  for(int i = 0; i < fragments.size(); i++)
  {
    Fragment& f1 = fragments[i];
    for(int j = 0; j < fragments.size(); j++)
    {
      if(j == i)
        continue;

      Fragment& f2 = fragments[j];

      if(!f2.waypoints.isEmpty() && f1.waypoints.contains(f2.waypoints))
        f2.waypoints.clear();
    }
  }

  // Remove the marked segments
  fragments.erase(std::remove_if(fragments.begin(), fragments.end(), [](const Fragment& f) -> bool {
          return f.waypoints.isEmpty();
        }), fragments.end());
}

} // namespace writer
} // namespace fs
} // namespace atools
