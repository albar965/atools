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

#include "routing/routenetwork.h"

#include "sql/sqldatabase.h"
#include "sql/sqlquery.h"
#include "sql/sqlrecord.h"
#include "geo/calculations.h"

#include <QElapsedTimer>

using atools::sql::SqlQuery;
using atools::geo::nmToMeter;
using atools::geo::Point3D;

namespace atools {
namespace routing {

RouteNetwork::RouteNetwork(atools::sql::SqlDatabase *sqlDb, DataSource dataSource)
  : db(sqlDb), source(dataSource)
{
  nearestDepartureDistanceM = nmToMeter(500.f);
  nearestDestDistanceM = nmToMeter(500.f);

  minNearestDistanceRadioM = nmToMeter(20.f);
  maxNearestDistanceRadioM = nmToMeter(1000.f);

  minNearestDistanceWpM = nmToMeter(100.f);
  maxNearestDistanceWpM = nmToMeter(200.f);

  directDistanceFactorRadio = 1.2f;
  directDistanceFactorWp = 1.05f;
  directDistanceFactorAirway = 1.2f;
}

RouteNetwork::~RouteNetwork()
{
  deInit();
}

void RouteNetwork::getNeighbours(Result& result, const Node& origin) const
{
  Q_ASSERT(destinationNode.isValid());
  Q_ASSERT(departureNode.isValid());

  // Node might be also departure or destination
  Point3D originPoint = point3D(origin.index);
  float originToDestDist = originPoint.directDistanceMeter(destinationPoint);

  if(source == SOURCE_AIRWAY)
  {
    // Add airway edges =======================================
    result.nodes.reserve(origin.edges.size());
    result.edges.reserve(origin.edges.size());

    // Avoid duplicates with direct neighbor search
    QSet<int> nodeIndexes;

    if(mode & MODE_AIRWAY)
    {
      // Look at all node edges/airways
      for(const Edge& edge : origin.edges)
      {
        // Check if edge type matches criteria (altitude, RNAV and airway type)
        if(matchEdge(edge))
        {
          // Check if node type matches like airway type
          if(matchNode(edge.toIndex))
          {
            // Edge can have only another node - not departure or destination
            Point3D curPoint = nodeIndex.atPoint3D(edge.toIndex);
            float curToDestDist = curPoint.directDistanceMeter(destinationPoint);

            // Add only nodes/edges that are ahead of the current node and lead towards the destination
            if(curToDestDist < originToDestDist)
            {
              float curToOriginDist = curPoint.directDistanceMeter(originPoint);
              if(curToDestDist + curToOriginDist < originToDestDist * directDistanceFactorAirway)
              {
                result.nodes.append(edge.toIndex);
                result.edges.append(edge);

                if(mode & MODE_WAYPOINT)
                  nodeIndexes.insert(edge.toIndex);
              }
            }
          }
        }
      }
    }

    // Additionally search for direct waypoint connections
    if((mode & MODE_WAYPOINT && result.size() < 2) || origin.isDeparture())
    {
      int found = searchNearest(result, origin, minNearestDistanceWpM, maxNearestDistanceWpM, &nodeIndexes);

      if(found < 2)
        searchNearest(result, origin, minNearestDistanceWpM * 2, maxNearestDistanceWpM * 8, &nodeIndexes);
    }
  }
  else
    // Find nearest navaids =======================================
    searchNearest(result, origin, minNearestDistanceRadioM, maxNearestDistanceRadioM);

  // Add destination node and calculate edges to it if in range ==========================================
  if(originToDestDist < nearestDestDistanceM)
  {
    result.nodes.append(destinationNode.index);
    result.edges.append(Edge(Node::DESTINATION_INDEX, originPoint.gcDistanceMeter(destinationPoint)));
  }
}

int RouteNetwork::searchNearest(Result& result, const Node& origin,
                                float minDistanceMeter, float maxDistanceMeter, const QSet<int> *excludeIndexes) const
{
  /* Callback class used for secondary stage filtering in radius searches.
   * Mainly used to keep all local variables accessible for the callback method. */
  struct RadiusCallback
  {
    bool callback(float dist, int index) const
    {
      Q_UNUSED(dist) // Squared manhattan distance is unusable here

      bool ok = true;

      // Current back to origin distance - calculate later
      float curToOriginDist = -1.f, curToDestDist = -1.f;

      // Do not use nodes in the exclude list
      if(excludeIndexes != nullptr)
        ok &= !excludeIndexes->contains(index);

      const Point3D& curPt = points[index];

      if(ok)
      {
        // Current to destination
        if(curToDestDist < 0.f)
          curToDestDist = curPt.directDistanceMeter(dest);

        // Include only if distance is smaller than distance between origin and destination
        // i.e. include points ahead but not behind the origin (search center)
        ok &= curToDestDist < originToDestDist;
      }

      if(ok)
      {
        // Current to destination
        if(curToDestDist < 0.f)
          curToDestDist = curPt.directDistanceMeter(dest);

        // Current to origin
        if(curToOriginDist < 0.f)
          curToOriginDist = curPt.directDistanceMeter(origin);

        // Include only points where the total distance (origin -> current -> destination) is not much
        // bigger than the direct connection (origin -> destination * directDistanceFactor)
        ok &= curToDestDist + curToOriginDist < originToDestDist * directDistFactor;
      }

      // Filter out anything inside the minimum radius if given
      if(ok && radiusMin > 0.f)
      {
        if(curToOriginDist < 0.f)
          curToOriginDist = curPt.directDistanceMeter(origin);
        ok &= curToOriginDist > radiusMin;
      }

      return ok;
    }

    float originToDestDist = 0.f, radiusMin = 0.f, directDistFactor = 1.f;
    Point3D origin, dest;
    const Point3D *points;
    bool radionav = false;
    const QSet<int> *excludeIndexes = nullptr;
  };

  // Prepare callback with data =========================
  RadiusCallback callbackObj;
  callbackObj.origin = nodeToCartesian(origin);
  callbackObj.points = nodeIndex.getPoints3D();
  callbackObj.excludeIndexes = (excludeIndexes == nullptr || excludeIndexes->isEmpty()) ? nullptr : excludeIndexes;
  callbackObj.radionav = isRadionavRouting();

  callbackObj.directDistFactor = isAirwayRouting() ? directDistanceFactorWp : directDistanceFactorRadio;
  callbackObj.originToDestDist = getDirectDistanceMeter(origin, destinationNode);
  callbackObj.dest = destinationPoint;

  if(callbackObj.radionav)
    // Do not use minimum near near destination or at departure
    callbackObj.radiusMin = (origin.isDeparture() || maxDistanceMeter > callbackObj.originToDestDist)
                            ? 0.f : minDistanceMeter;
  else
    // No minimum near departure and destination
    callbackObj.radiusMin = (origin.isDeparture() || maxDistanceMeter > callbackObj.originToDestDist)
                            ? minDistanceMeter / 4.f : minDistanceMeter;

  // Limit search radius by distance to destination for airway and waypoint search
  if(!callbackObj.radionav)
    maxDistanceMeter = std::min(maxDistanceMeter, callbackObj.originToDestDist);

  // Use a lambda instead of std::bind since this is faster
  atools::geo::RadiusCallbackType callbackFunc = [&callbackObj](float dist, int index) -> bool {
                                                   return callbackObj.callback(dist, index);
                                                 };
  QVector<int> indexes;
  nodeIndex.getRadiusIndexes(indexes, origin.pos, maxDistanceMeter, callbackFunc);

  result.nodes.reserve(indexes.size());
  result.edges.reserve(indexes.size());

  // Copy node indexes and edges to result ======================
  int numFound = 0;
  Point3D originPoint = nodeToCartesian(origin);
  for(int idx : indexes)
  {
    if(matchNode(idx))
    {
      // Add node and edge leading to it
      result.nodes.append(idx);
      result.edges.append(Edge(idx, originPoint.gcDistanceMeter(nodeIndex.atPoint3D(idx))));
      numFound++;
    }
  }
  return numFound;
}

void RouteNetwork::setParameters(const geo::Pos& departurePos, const geo::Pos& destinationPos, int altitudeParam,
                                 Modes modeParam)
{
  clearParameters();

  altitude = altitudeParam;
  mode = modeParam;

  if(departurePos.isValid())
  {
    // Add departure node to network ====================
    departureNode.index = Node::DEPARTURE_INDEX;
    departureNode.pos = departurePos;
    departureNode.type = DEPARTURE;
    departureNode.range = 0;
    departureNode.subtype = NONE;
    departurePos.toCartesian(departurePoint);
  }

  if(destinationPos.isValid())
  {
    // Add destination node to network ====================
    destinationNode.index = Node::DESTINATION_INDEX;
    destinationNode.pos = destinationPos;
    destinationNode.type = DESTINATION;
    destinationNode.range = 0;
    destinationNode.subtype = NONE;
    destinationPos.toCartesian(destinationPoint);

    if(departurePos.isValid())
    {
      routeDirectDistance = getDirectDistanceMeter(departureNode, destinationNode);
      routeGcDistance = getGcDistanceMeter(departureNode, destinationNode);
    }
  }
}

void RouteNetwork::clearParameters()
{
  altitude = 0;
  mode = MODE_ALL;
  departureNode = Node();
  departurePoint = Point3D();

  destinationNode = Node();
  destinationPoint = Point3D();
  routeDirectDistance = routeGcDistance = 0.f;
}

const Node& RouteNetwork::getNode(int index) const
{
  const static atools::routing::Node INVALID;

  if(index >= 0)
    return nodeIndex.at(index);
  else if(index == Node::DEPARTURE_INDEX)
    return departureNode;
  else if(index == Node::DESTINATION_INDEX)
    return destinationNode;
  else
    return INVALID;
}

void RouteNetwork::setMinNearestDistanceRadioNm(float value)
{
  minNearestDistanceRadioM = nmToMeter(value);
}

void RouteNetwork::setMinNearestDistanceWpNm(float value)
{
  minNearestDistanceWpM = nmToMeter(value);
}

void RouteNetwork::setMaxNearestDistanceRadioNm(float value)
{
  maxNearestDistanceRadioM = nmToMeter(value);
}

void RouteNetwork::setMaxNearestDistanceWpNm(float value)
{
  maxNearestDistanceWpM = nmToMeter(value);
}

void RouteNetwork::setNearestDepartureDistanceNm(float value)
{
  nearestDepartureDistanceM = nmToMeter(value);
}

void RouteNetwork::setNearestDestDistanceNm(float value)
{
  nearestDestDistanceM = nmToMeter(value);
}

const geo::Point3D& RouteNetwork::point3D(int index) const
{
  const static Point3D INVALID;

  if(index >= 0)
    return nodeIndex.atPoint3D(index);
  else if(index == Node::DEPARTURE_INDEX)
    return departurePoint;
  else if(index == Node::DESTINATION_INDEX)
    return destinationPoint;
  else
    return INVALID;
}

bool RouteNetwork::matchNode(int index) const
{
  switch(nodeIndex.at(index).type)
  {
    case atools::routing::VOR:
    case atools::routing::VORDME:
    case atools::routing::DME:
      return mode & MODE_RADIONAV_VOR;

    case atools::routing::NDB:
      return mode & MODE_RADIONAV_NDB;

    case atools::routing::WAYPOINT_VICTOR:
      return mode & MODE_WAYPOINT || mode & MODE_VICTOR;

    case atools::routing::WAYPOINT_JET:
      return mode & MODE_WAYPOINT || mode & MODE_JET;

    case atools::routing::WAYPOINT_BOTH:
      return mode & MODE_WAYPOINT || mode & MODE_JET || mode & MODE_VICTOR;

    case atools::routing::WAYPOINT_UNNAMED:
    case atools::routing::WAYPOINT_NAMED:
      return mode & MODE_WAYPOINT;

    case atools::routing::NONE:
    case atools::routing::DEPARTURE:
    case atools::routing::DESTINATION:
      break;
  }
  return true;
}

bool RouteNetwork::matchEdge(const Edge& edge) const
{
  bool ok = (altitude == 0 || (altitude >= edge.minAltFt && altitude <= edge.maxAltFt));

  // Check if RNAV has to be excluded
  ok &= !(mode & MODE_NO_RNAV) || edge.routeType != RNAV;

  ok &= (edge.isJetAirway() && mode & MODE_JET) ||
        (edge.isVictorAirway() && mode & MODE_VICTOR) ||
        (edge.isNoAirway() && mode & MODE_WAYPOINT);

  return ok;
}

void RouteNetwork::clearIndexes()
{
  nodeIndex.clear();
  nodeIndex.updateIndex();
  nodeIdIndexMap.clear();
}

void RouteNetwork::ensureLoaded()
{
  if(!isLoaded())
    initInternal();
}

bool RouteNetwork::isLoaded() const
{
  return !nodeIndex.isEmpty();
}

void RouteNetwork::init()
{
  // Clear all for the next call to ensureLoaded
  clearIndexes();
}

void RouteNetwork::initInternal()
{
  QElapsedTimer timer;
  timer.start();

  deInit();

  int currentIndex = 0;

  if(source == SOURCE_RADIO)
  {
    // Load VOR, VORDME and VORTAC. No DME and no TACAN. ==========================================
    readNodesRadio("select v.vor_id, v.lonx, v.laty, v.range, "
                   "case when v.dme_altitude is null then 0 else 1 end as has_dme "
                   "from vor v where type <> 'TC' and dme_only = 0", currentIndex, true);

    // Load all NDB =================================================
    readNodesRadio("select n.ndb_id, n.lonx, n.laty, n.range, null as has_dme from ndb n", currentIndex, false);
  }
  else if(source == SOURCE_AIRWAY)
  {
    // Load waypoints and airways ====================================

    // Outgoing edges for node id
    QMultiHash<int, Edge> nodeEdgeMap;
    nodeEdgeMap.reserve(200000);

    // Read all airways from database into map. Edge::toIndex gets database id temporarily
    readEdgesAirway(nodeEdgeMap);

    nodeIdIndexMap.reserve(300000);

    QVector<Node> nodesTemp;
    nodesTemp.reserve(300000);

    // Column order is important in the queries

    // Read navaids into nodeIdIndexMap and into nodesTemp
    // Named waypoints without airways
    // Unnamed degree confluence waypoints
    readNodesAirway(nodesTemp,
                    "select w.waypoint_id, w.ident, w.type, w.lonx, w.laty "
                    "from waypoint w "
                    "where (w.type = 'WN' or (w.type = 'WU' and w.airport_id is null)) and "
                    "w.num_jet_airway = 0 and w.num_victor_airway = 0",
                    currentIndex, false, false, false, true /* filterUnnamed */);
    // "where w.type = 'WN' and "

    // Airway waypoints
    readNodesAirway(nodesTemp,
                    "select w.waypoint_id, w.ident, w.type, w.lonx, w.laty, w.num_jet_airway, w.num_victor_airway "
                    "from waypoint w "
                    "where w.type like 'W%' and (w.num_jet_airway > 0 or w.num_victor_airway > 0)",
                    currentIndex, false, false, true /* airways */, false);

    // Airway VOR waypoints
    readNodesAirway(nodesTemp,
                    "select w.waypoint_id, w.ident, w.type, w.lonx, w.laty, w.num_jet_airway, w.num_victor_airway, "
                    "v.range, v.type as radiotype, v.dme_altitude,  v.dme_only "
                    "from waypoint w join vor v on w.nav_id = v.vor_id "
                    "where w.type = 'V' and (w.num_jet_airway > 0 or w.num_victor_airway > 0)",
                    currentIndex, true /* VOR */, false, true /* airways */, false);

    // Airway NDB waypoints
    readNodesAirway(nodesTemp,
                    "select w.waypoint_id, w.ident, w.type, w.lonx, w.laty, w.num_jet_airway, w.num_victor_airway, "
                    "n.range "
                    "from waypoint w join ndb n on w.nav_id = n.ndb_id "
                    "where w.type = 'N' and (w.num_jet_airway > 0 or w.num_victor_airway > 0)",
                    currentIndex, false, true /* NDB */, true /* airways */, false);

    // Insert outgoing edges to each node and copy them to the index
    nodeIndex.reserve(nodesTemp.size());
    for(Node n : nodesTemp)
    {
      n.edges = nodeEdgeMap.values(n.id).toVector();

      // Replace database ids in Edge::toIndex with array indexes
      for(Edge& e : n.edges)
        e.toIndex = nodeIdIndexMap.value(e.toIndex);

      nodeIndex.append(n);
    }
  }
  nodeIndex.updateIndex();

  // Calculate great circle distance for all edges
  for(Node& n : nodeIndex)
  {
    for(Edge& e : n.edges)
      e.lengthMeter =
        atools::roundToInt(nodeIndex.atPoint3D(n.index).gcDistanceMeter(nodeIndex.atPoint3D(e.toIndex)));
  }

  qDebug() << Q_FUNC_INFO << timer.restart() << "ms";
}

void RouteNetwork::readEdgesAirway(QMultiHash<int, Edge>& nodeEdgeMap) const
{
  QString cols;
  atools::sql::SqlRecord rec = db->record("airway");
  if(rec.contains("route_type"))
  {
    cols = "airway_id, minimum_altitude, maximum_altitude, airway_name, "
           "route_type, airway_type, direction, from_waypoint_id, to_waypoint_id";
  }
  else
    cols = "airway_id, minimum_altitude, maximum_altitude, airway_name, "
           "null as route_type, airway_type, direction, from_waypoint_id, to_waypoint_id";

  // Column indexes
  enum
  {
    ID = 0,
    MIN = 1,
    MAX = 2,
    NAME = 3,
    ROUTE_TYPE = 4,
    TYPE = 5,
    DIRECTION = 6,
    FROM = 7,
    TO = 8
  };

  SqlQuery query("select  " + cols + " from airway", db);
  query.exec();
  while(query.next())
  {
    Edge edge;
    edge.airwayId = query.valueInt(ID);
    edge.minAltFt =
      static_cast<quint16>(std::min(query.valueInt(MIN), static_cast<int>(Edge::MAX_ALTITUDE)));
    edge.maxAltFt =
      static_cast<quint16>(std::min(query.valueInt(MAX), static_cast<int>(Edge::MAX_ALTITUDE)));
    edge.airwayHash = qHash(query.valueStr(NAME)) + 1; // Avoid null

    char routeType = atools::strToChar(query.valueStr(ROUTE_TYPE));
    if(routeType == 'A')
      edge.routeType = AIRLINE; /* A Airline Airway (Tailored Data) */
    else if(routeType == 'C')
      edge.routeType = CONTROL; /* C Control */
    else if(routeType == 'D')
      edge.routeType = DIRECT; /* D Direct Route */
    else if(routeType == 'H')
      edge.routeType = HELICOPTER; /* H Helicopter Airways */
    else if(routeType == 'O')
      edge.routeType = OFFICIAL; /* O Officially Designated Airways, except RNAV, Helicopter Airways */
    else if(routeType == 'R')
      edge.routeType = RNAV; /* R RNAV Airways */
    else if(routeType == 'S')
      edge.routeType = UNDESIGNATED; /* S Undesignated ATS Route */
    else
      edge.routeType = NO_ROUTE_TYPE;

    char type = atools::strToChar(query.valueStr(TYPE));
    if(type == 'J')
      edge.type = AIRWAY_JET;
    else if(type == 'V')
      edge.type = AIRWAY_VICTOR;
    else if(type == 'B')
      edge.type = AIRWAY_BOTH;

    // Add one edge for each allowed direction
    char dir = atools::strToChar(query.valueStr(DIRECTION));
    int from = query.valueInt(FROM);
    int to = query.valueInt(TO);

    if(dir == 'F' || dir == 'N')
    {
      // Forward or both directions allowed
      edge.toIndex = to;
      nodeEdgeMap.insert(from, edge);
    }

    if(dir == 'B' || dir == 'N')
    {
      // Backward or both directions allowed
      edge.toIndex = from;
      nodeEdgeMap.insert(to, edge);
    }
  }
}

void RouteNetwork::readNodesAirway(QVector<Node>& nodes, const QString& queryStr, int& idx, bool vor, bool ndb,
                                   bool airway, bool filterUnnamed)
{
  // Column indexes
  // 0            1      2     3     4               5                  6      7        8             9
  // waypoint_id, ident, type, lonx, laty, num_jet_airway, num_victor_airway
  // waypoint_id, ident, type, lonx, laty, num_jet_airway, num_victor_airway, range, radiotype, dme_altitude, dme_only
  // waypoint_id, ident, type, lonx, laty, num_jet_airway, num_victor_airway, range
  enum
  {
    ID = 0,
    IDENT = 1,
    TYPE = 2,
    LONX = 3,
    LATY = 4,
    NUM_JET_AIRWAY = 5,
    NUM_VICTOR_AIRWAY = 6,
    RANGE = 7,
    RADIO_TYPE = 8,
    DME_ALTITUDE = 9,
    DME_ONLY = 10
  };

  SqlQuery query(queryStr, db);
  query.exec();
  while(query.next())
  {
    QString type = query.valueStr(TYPE);
    atools::geo::Pos pos(query.valueFloat(LONX), query.valueFloat(LATY));

    if(filterUnnamed && type == "WU")
    {
      // Include all one degree grid confluence points
      bool ok = pos.nearGrid(atools::geo::Pos::POS_EPSILON_10M);

      if(!ok)
      {
        // Include unnamed oceanic waypoints like 2230N 6900E 51N50 32W20 02E60
        QString ident = query.valueStr(IDENT);
        // First two characters need to be digits
        if(atools::charAt(ident, 0).isDigit() && atools::charAt(ident, 1).isDigit())
        {
          // Compass direction or digit followed by a digit
          char at2 = atools::latin1CharAt(ident, 2);
          if((std::isdigit(at2) || atools::contains(at2, {'N', 'S', 'E', 'W'})) && atools::charAt(ident, 3).isDigit())
          {
            char at4 = atools::latin1CharAt(ident, 4);
            ok = std::isdigit(at4) || atools::contains(at4, {'N', 'S', 'E', 'W'});
          }
        }
      }

      if(!ok)
        continue;
    }

    Node node;
    node.index = idx;
    node.id = query.valueInt(ID);
    node.pos = pos;

    int numJet = airway ? query.valueInt(NUM_JET_AIRWAY) : 0;
    int numVictor = airway ? query.valueInt(NUM_VICTOR_AIRWAY) : 0;

    // Use number of attached airways to determine type
    if(numJet > 0 && numVictor > 0)
      node.type = WAYPOINT_BOTH;
    else if(numJet > 0)
      node.type = WAYPOINT_JET;
    else if(numVictor > 0)
      node.type = WAYPOINT_VICTOR;
    else if(type == "WN")
      node.type = WAYPOINT_NAMED;
    else if(type == "WU")
      node.type = WAYPOINT_UNNAMED;

    if(vor || ndb)
      node.range = nmToMeter(query.valueInt(RANGE));

    if(vor)
    {
      // Query uses VOR table ====================
      QString vortype = query.valueStr(RADIO_TYPE);
      if(vortype == "H" || vortype == "L" || vortype == "T" || vortype.startsWith("VT"))
      {
        if(query.valueBool(DME_ONLY))
          node.subtype = DME;
        else
          node.subtype = query.isNull(DME_ALTITUDE) ? VOR : VORDME;
      }
    }

    if(ndb)
      // Query uses NDB table ====================
      node.subtype = NDB;

    if(node.type == NONE)
      qWarning() << "No node type" << query.record();

    nodes.append(node);
    nodeIdIndexMap.insert(node.id, idx);
    idx++;
  }
}

void RouteNetwork::readNodesRadio(const QString& queryStr, int& idx, bool vor)
{
  // Column indexes
  // id, lonx, laty, range, has_dme
  enum
  {
    ID = 0,
    LONX = 1,
    LATY = 2,
    RANGE = 3,
    HAS_DME = 4
  };

  SqlQuery query(queryStr, db);
  query.exec();
  while(query.next())
  {
    Node node;
    node.index = idx;
    node.id = query.valueInt(ID);
    node.pos.setLonX(query.valueFloat(LONX));
    node.pos.setLatY(query.valueFloat(LATY));
    node.range = nmToMeter(query.valueInt(RANGE));

    if(vor)
      node.type = query.valueBool(HAS_DME) ? VORDME : VOR;
    else
      node.type = NDB;

    nodeIndex.append(node);
    idx++;
  }
}

void RouteNetwork::deInit()
{
  clearIndexes();
}

} // namespace route
} // namespace atools
