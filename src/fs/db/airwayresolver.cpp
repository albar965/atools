/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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

#include "fs/progresshandler.h"
#include "geo/calculations.h"
#include "geo/linestring.h"
#include "geo/pos.h"
#include "geo/rect.h"
#include "sql/sqldatabase.h"
#include "sql/sqlquery.h"
#include "sql/sqlutil.h"

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

// airwayPointQuery "select name, type, mid_type, mid_ident, mid_region, ..."
enum AirwayPointQueryColumns
{
  NAME, TYPE, MID_TYPE, MID_IDENT, MID_REGION,
  NEXT_DIRECTION, NEXT_TYPE, NEXT_IDENT, NEXT_REGION, NEXT_MINIMUM_ALTITUDE, NEXT_MAXIMUM_ALTITUDE,
  PREVIOUS_DIRECTION, PREVIOUS_TYPE, PREVIOUS_IDENT, PREVIOUS_REGION, PREVIOUS_MINIMUM_ALTITUDE, PREVIOUS_MAXIMUM_ALTITUDE
};

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

inline size_t qHash(const AirwayResolver::AirwaySegment& segment, size_t seed)
{
  return qHashMulti(seed, segment.fromWaypointId, segment.toWaypointId);
}

AirwayResolver::AirwayResolver(SqlDatabase& sqlDb, atools::fs::ProgressHandler& progress)
  : progressHandler(progress), curAirwayId(1), numAirways(0), airwayInsertStmt(sqlDb), db(sqlDb)
{
  SqlUtil util(sqlDb);
  airwayInsertStmt.prepare(util.buildInsertStatement(QStringLiteral("airway")));
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

// Sort waypoint list if it contains more than one point and if the reference positions have only one
inline void sortWaypointIdPosList(QList<WaypointIdPos>& waypointIdPosList, const QList<WaypointIdPos>& positions)
{
  if(waypointIdPosList.size() > 1 && positions.size() == 1)
  {
    std::sort(waypointIdPosList.begin(), waypointIdPosList.end(),
              [&positions](const WaypointIdPos& idPos1, const WaypointIdPos& idPos2) -> bool {
            return idPos1.second.distanceMeterTo(positions.constFirst().second) <
                   idPos2.second.distanceMeterTo(positions.constFirst().second);
          });
  }
}

bool AirwayResolver::run(int numReportSteps)
{
  bool aborted = false;

  // Clean the table
  SqlQuery deleteAirwayQuery(db);
  deleteAirwayQuery.exec(QStringLiteral("delete from airway"));
  int deleted = deleteAirwayQuery.numRowsAffected();
  qInfo() << "Removed" << deleted << "from airway table";

  // Use set to
  QSet<AirwaySegment> airway;
  QString currentAirway;

  int totalRowCount = SqlUtil(db).rowCount(QStringLiteral("tmp_airway_point"));

  int rowsPerStep = static_cast<int>(std::ceil(static_cast<float>(totalRowCount) / static_cast<float>(numReportSteps)));
  int row = 0, steps = 0;

  QElapsedTimer timer;
  timer.start();
  qint64 elapsed = timer.elapsed();

  SqlQuery waypointQuery(db);
  waypointQuery.prepare("select waypoint_id, ident, region, type, lonx, laty "
                        "from tmp_waypoint where ident = ? and region = ? and type = ?");

  // Get all tmp_airway_point rows and join previous and next waypoints to the result by ident and region
  // Result is ordered by airway name
  SqlQuery airwayPointQuery(db);
  airwayPointQuery.
  exec(QStringLiteral("select name, type, mid_type, mid_ident, mid_region, "
                      "next_direction, next_type, next_ident, next_region, "
                      "next_minimum_altitude, next_maximum_altitude, "
                      "previous_direction, previous_type, previous_ident, previous_region, "
                      "previous_minimum_altitude, previous_maximum_altitude "
                      "from tmp_airway_point order by name"));
  float longestAirwaySegmentMeter = 0.f;
  while(airwayPointQuery.next())
  {
    QString airwayName = airwayPointQuery.value(QStringLiteral("name")).toString();
    QString airwayType = airwayPointQuery.value(QStringLiteral("type")).toString();

    if((row++ % rowsPerStep) == 0)
    {
      qint64 elapsed2 = timer.elapsed();

      // Update only every 500 ms - otherwise update only progress count
      bool silent = !(elapsed + MIN_PROGRESS_REPORT_MS < elapsed2);
      if(!silent)
        elapsed = elapsed2;
      steps++;
      if((aborted = progressHandler.reportOther(tr("Creating airways: %1...").arg(airwayName), -1, silent)) == true)
        break;
    }

    if(airwayName != currentAirway)
    {
      // A new airway comes from from the query save the current one to the database
      saveAirway(airway, currentAirway);
      currentAirway = airwayName;
    }

    QList<WaypointIdPos> midWaypoints, prevWaypoints, nextWaypoints;
    fetchNavaids(prevWaypoints, airwayPointQuery, waypointQuery, PREVIOUS_TYPE, airwayName);
    fetchNavaids(midWaypoints, airwayPointQuery, waypointQuery, MID_TYPE, airwayName);
    fetchNavaids(nextWaypoints, airwayPointQuery, waypointQuery, NEXT_TYPE, airwayName);

    // Sort waypoint lists if they contain more than one point and if the reference positions have only one
    // This is needed to resolve ambiguities when processing waypoints at the start or end of the list
    sortWaypointIdPosList(prevWaypoints, midWaypoints);
    sortWaypointIdPosList(prevWaypoints, nextWaypoints);

    sortWaypointIdPosList(nextWaypoints, prevWaypoints);
    sortWaypointIdPosList(nextWaypoints, midWaypoints);

    sortWaypointIdPosList(midWaypoints, prevWaypoints);
    sortWaypointIdPosList(midWaypoints, nextWaypoints);

    // Get uniqe id and position
    int midWpId = -1, prevWpId = -1, nextWpId = -1;
    Pos midWpPos, prevWpPos, nextWpPos;
    if(!prevWaypoints.isEmpty())
    {
      prevWpId = prevWaypoints.constFirst().first;
      prevWpPos = prevWaypoints.constFirst().second;
    }

    if(!nextWaypoints.isEmpty())
    {
      nextWpId = nextWaypoints.constFirst().first;
      nextWpPos = nextWaypoints.constFirst().second;
    }

    if(!midWaypoints.isEmpty())
    {
      midWpId = midWaypoints.constFirst().first;
      midWpPos = midWaypoints.constFirst().second;
    }

    if(prevWpId != -1)
    {
      // Previous waypoint found - add segment
      float midPrevDist = midWpPos.distanceMeterTo(prevWpPos);
      if(maxAirwaySegmentLengthNm <= 1.f || midPrevDist < atools::geo::nmToMeter(maxAirwaySegmentLengthNm))
      {
        int prevMinAlt = airwayPointQuery.valueInt(PREVIOUS_MINIMUM_ALTITUDE);
        int prevMaxAlt = airwayPointQuery.valueInt(PREVIOUS_MAXIMUM_ALTITUDE);
        char prevDir = atools::strToChar(airwayPointQuery.valueStr(PREVIOUS_DIRECTION));
        airway.insert(AirwaySegment(prevWpId, midWpId, prevDir, prevMinAlt, prevMaxAlt, airwayType, prevWpPos, midWpPos));
      }

      longestAirwaySegmentMeter = std::max(longestAirwaySegmentMeter, midPrevDist);
    }

    if(nextWpId != -1)
    {
      // Next waypoint found - add segment
      float midNextDist = midWpPos.distanceMeterTo(nextWpPos);
      if(maxAirwaySegmentLengthNm <= 1.f || midNextDist < atools::geo::nmToMeter(maxAirwaySegmentLengthNm))
      {
        int nextMinAlt = airwayPointQuery.valueInt(NEXT_MINIMUM_ALTITUDE);
        int nextMaxAlt = airwayPointQuery.valueInt(NEXT_MAXIMUM_ALTITUDE);
        char nextDir = atools::strToChar(airwayPointQuery.valueStr(NEXT_DIRECTION));
        airway.insert(AirwaySegment(midWpId, nextWpId, nextDir, nextMinAlt, nextMaxAlt, airwayType, midWpPos, nextWpPos));
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
    db.commit();

  return aborted;
}

void AirwayResolver::fetchNavaids(QList<WaypointIdPos>& waypointList, atools::sql::SqlQuery& airwayPointQuery,
                                  atools::sql::SqlQuery& waypointQuery, int columnOffset, const QString& airwayName)
{
  // waypointQuery - select waypoint_id, ident, region, type, lonx, laty  from tmp_waypoint where ident = ? and region = ? and type = ?
  enum {BIND_IDENT, BIND_REGION, BIND_TYPE};

  // waypointQuery
  enum {WAYPOINT_ID, IDENT, REGION, TYPE, LONX, LATY};

  const QString type = airwayPointQuery.valueStr(columnOffset);
  const QString ident = airwayPointQuery.valueStr(columnOffset + 1);
  const QString region = airwayPointQuery.valueStr(columnOffset + 2);
  waypointQuery.bindValue(BIND_TYPE, type);
  waypointQuery.bindValue(BIND_IDENT, ident);
  waypointQuery.bindValue(BIND_REGION, region);
  waypointQuery.exec();

  while(waypointQuery.next())
    waypointList.append(std::make_pair(waypointQuery.valueInt(WAYPOINT_ID),
                                       Pos(waypointQuery.valueFloat(LONX), waypointQuery.valueFloat(LATY))));

  if(waypointList.size() > 1)
  {
    QString seqType;
    if(columnOffset == PREVIOUS_TYPE)
      seqType = QStringLiteral("PREV");
    else if(columnOffset == MID_TYPE)
      seqType = QStringLiteral("MID");
    else if(columnOffset == NEXT_TYPE)
      seqType = QStringLiteral("NEXT");

    qDebug() << Q_FUNC_INFO << "Ambigious waypoints" << airwayName << seqType << ident << region;
  }
}

void AirwayResolver::saveAirway(QSet<AirwaySegment>& airway, const QString& currentAirway)
{
  if(!airway.empty())
  {
    // Build airway fragments
    QList<Fragment> fragments;
    buildAirway(currentAirway, airway, fragments);

    // Remove all fragments that are contained by others
    cleanFragments(fragments);

    for(const Fragment& fragment : std::as_const(fragments))
    {
      for(const TypeRowValueList& bindRow : fragment.boundValues)
      {
        airwayInsertStmt.bindValues(bindRow);
        airwayInsertStmt.exec();
        numAirways += airwayInsertStmt.numRowsAffected();
      }
    }
    airway.clear();
  }
}

void AirwayResolver::buildAirway(const QString& airwayName, QSet<AirwaySegment>& airway, QList<Fragment>& fragments)
{
  // Queue of waypoints that will get waypoints in order prependend and appendend
  QQueue<AirwaySegment> newAirway;

  // Segments indexed by from waypoint ID
  QHash<int, AirwaySegment> segsByFromWpId;
  // Segments indexed by to waypoint ID
  QHash<int, AirwaySegment> segsByToWpId;

  // Fill the index
  for(const AirwaySegment& segment : std::as_const(airway))
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

    for(const AirwaySegment& newSegment : std::as_const(newAirway))
    {
      last = newSegment;

      fragment.waypoints.insert(newSegment.fromWaypointId);
      fragment.waypoints.insert(newSegment.toWaypointId);

      // Create bounding rect for this segment
      Rect bounding(newSegment.fromPos);
      bounding.extend(newSegment.toPos);

      TypeRowValueList row;

      row.append(std::make_pair(QStringLiteral(":airway_id"), curAirwayId));
      row.append(std::make_pair(QStringLiteral(":airway_name"), airwayName));
      row.append(std::make_pair(QStringLiteral(":airway_type"), newSegment.type));
      // route_type varchar(5), unused

      row.append(std::make_pair(QStringLiteral(":airway_fragment_no"), fragmentNum));
      row.append(std::make_pair(QStringLiteral(":sequence_no"), seqNo));

      row.append(std::make_pair(QStringLiteral(":from_waypoint_id"), newSegment.fromWaypointId));
      row.append(std::make_pair(QStringLiteral(":to_waypoint_id"), newSegment.toWaypointId));

      row.append(std::make_pair(QStringLiteral(":direction"), atools::charToStr(newSegment.dir)));
      row.append(std::make_pair(QStringLiteral(":minimum_altitude"), newSegment.minAlt));
      row.append(std::make_pair(QStringLiteral(":maximum_altitude"), newSegment.maxAlt));
      row.append(std::make_pair(QStringLiteral(":left_lonx"), bounding.getTopLeft().getLonX()));
      row.append(std::make_pair(QStringLiteral(":top_laty"), bounding.getTopLeft().getLatY()));
      row.append(std::make_pair(QStringLiteral(":right_lonx"), bounding.getBottomRight().getLonX()));
      row.append(std::make_pair(QStringLiteral(":bottom_laty"), bounding.getBottomRight().getLatY()));

      // Write start and end coordinates for this segment
      row.append(std::make_pair(QStringLiteral(":from_lonx"), newSegment.fromPos.getLonX()));
      row.append(std::make_pair(QStringLiteral(":from_laty"), newSegment.fromPos.getLatY()));
      row.append(std::make_pair(QStringLiteral(":to_lonx"), newSegment.toPos.getLonX()));
      row.append(std::make_pair(QStringLiteral(":to_laty"), newSegment.toPos.getLatY()));

      fragment.boundValues.append(row);

      seqNo++;
      curAirwayId++;
    }
    fragments.append(fragment);

    fragmentNum++;
  }
}

void AirwayResolver::cleanFragments(QList<Fragment>& fragments)
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
