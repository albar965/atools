/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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

namespace atools {
namespace routing {

/* Callback class used for secondary stage filtering in radius searches */
struct RadiusCallback
{
  float originToDestDist = 0.f, radiusMin = 0.f, directDistFactor = 0.f;
  atools::geo::Point3D origin, dest;
  const atools::geo::Point3D *points;

  const QSet<int> *excludeIndexes = nullptr;

  bool callback(float dist, int index)
  {
    Q_UNUSED(dist)
    bool ok = true;

    // Do not use nodes in the exclude list
    if(excludeIndexes != nullptr)
      ok &= !excludeIndexes->contains(index);

    if(ok && originToDestDist > 0.f)
    {
      // Current point to destination
      float distToDest = points[index].directDistanceMeter(dest);

      if(originToDestDist < atools::geo::nmToMeter(100.f))
        // Allow nodes after destination if close enough
        ok &= distToDest < originToDestDist + atools::geo::nmToMeter(50.f);
      else
      {
        // Include only if distance is smaller than distance between origin and destination
        // i.e. do not include points behind the origin (search center)
        ok &= distToDest < originToDestDist;

        // Include only points where the total distance (origin -> current -> destination) is not much
        // bigger than the direct connection (origin -> destination * directDistanceFactor)
        if(ok && directDistFactor > 1.f)
          ok &= distToDest + points[index].directDistanceMeter(origin) < originToDestDist * directDistFactor;
      }
    }

    // Filter out anything inside the minimum radius if given
    if(ok && radiusMin > 0.f)
      ok &= points[index].directDistanceMeter(origin) > radiusMin;

    return ok;
  }

};

RouteNetwork::RouteNetwork(atools::sql::SqlDatabase *sqlDb, DataSource dataSource)
  : db(sqlDb), source(dataSource)
{
}

RouteNetwork::~RouteNetwork()
{
  deInit();
}

void RouteNetwork::getNeighbours(Result& result, const Node& from) const
{
  atools::geo::Point3D p3dFrom = nodeToCartesian(from);

  if(source == SOURCE_AIRWAY)
  {
    // Add airway edges =======================================
    result.nodes.reserve(from.edges.size() + 10);
    result.edges.reserve(from.edges.size() + 10);

    float fromToDestDistance = p3dFrom.directDistanceMeter(destinationPoint3D);

    QSet<int> nodeIndexes;
    // Look at all node edges/airways
    for(const Edge& edge : from.edges)
    {
      // Check if edge type matches
      if(matchEdge(edge))
      {
        const Node& node = getNode(edge.toIndex);
        atools::geo::Point3D p3dNode = nodeToCartesian(node);

        // Add only nodes/edges that are ahead of the current node and lead towards the destination
        if(p3dNode.directDistanceMeter(destinationPoint3D) < fromToDestDistance)
        {
          // Check if node type matches
          if(matchNode(node))
          {
            result.nodes.append(node.index);
            result.edges.append(edge);

            if(mode & MODE_WAYPOINT)
              nodeIndexes.insert(node.index);
          }
        }
      }
    }

    if(mode & MODE_WAYPOINT)
      searchNearest(result, from, nmToMeter(minNearestDistanceWpNm), nmToMeter(maxNearestDistanceWpNm), &nodeIndexes);
  }
  else
    // Find nearest navaids =======================================
    searchNearest(result, from, nmToMeter(minNearestDistanceRadioNm), nmToMeter(maxNearestDistanceRadioNm));

  // Add destination node and calculate edges to it if in range ==========================================
  if(p3dFrom.directDistanceMeter(destinationPoint3D) < nmToMeter(nearestDestDistanceNm))
  {
    result.nodes.append(destinationNode.index);
    result.edges.append(Edge(Node::DESTINATION_INDEX, p3dFrom.gcDistanceMeter(destinationPoint3D)));
  }
}

void RouteNetwork::searchNearest(Result& result, const Node& origin,
                                 float minDistanceMeter, float maxDistanceMeter, const QSet<int> *excludeIndexes) const
{
  // Prepare callback with data =========================
  RadiusCallback callbackObj;
  if(destinationNode.isValid())
  {
    callbackObj.originToDestDist = origin.isDeparture() ? 0.f : getDirectDistanceMeter(origin, destinationNode);
    callbackObj.directDistFactor = isAirwayRouting() ? directDistanceFactorWp : directDistanceFactorRadio;
    callbackObj.dest = origin.isDeparture() ? atools::geo::Point3D() : destinationPoint3D;
  }
  callbackObj.radiusMin = origin.isDeparture() ? 0.f : minDistanceMeter;
  callbackObj.origin = nodeToCartesian(origin);
  callbackObj.points = nodeIndex.getPoints3D();
  callbackObj.excludeIndexes = excludeIndexes;

  // Use a lambda instead of std::bind since this is faster
  atools::geo::RadiusCallbackType callbackFunc = [&callbackObj](float dist, int index) -> bool {
                                                   return callbackObj.callback(dist, index);
                                                 };
  QVector<int> indexes;
  nodeIndex.getRadiusIndexes(indexes, origin.pos, maxDistanceMeter, callbackFunc);

  result.nodes.reserve(indexes.size());
  result.edges.reserve(indexes.size());

  atools::geo::Point3D p3dFrom = nodeToCartesian(origin);
  for(int idx : indexes)
  {
    const Node& node = nodeIndex.at(idx);

    if(matchNode(node))
    {
      // Add node and edge leading to it
      result.nodes.append(node.index);
      result.edges.append(Edge(idx, p3dFrom.gcDistanceMeter(nodeIndex.atPoint3D(idx))));
    }
  }
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
    departurePos.toCartesian(departurePoint3D);
  }

  if(destinationPos.isValid())
  {
    // Add destination node to network ====================
    destinationNode.index = Node::DESTINATION_INDEX;
    destinationNode.pos = destinationPos;
    destinationNode.type = DESTINATION;
    destinationNode.range = 0;
    destinationNode.subtype = NONE;
    destinationPos.toCartesian(destinationPoint3D);
  }
}

void RouteNetwork::clearParameters()
{
  altitude = 0;
  mode = MODE_ALL;
  departureNode = Node();
  departurePoint3D = atools::geo::Point3D();

  destinationNode = Node();
  destinationPoint3D = atools::geo::Point3D();
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

const Node& RouteNetwork::getNearestNode(const geo::Pos& pos) const
{
  return nodeIndex.getNearest(pos);
}

const geo::Point3D& RouteNetwork::point3D(int index) const
{
  const static atools::geo::Point3D INVALID;

  if(index >= 0)
    return nodeIndex.atPoint3D(index);
  else if(index == Node::DEPARTURE_INDEX)
    return departurePoint3D;
  else if(index == Node::DESTINATION_INDEX)
    return destinationPoint3D;
  else
    return INVALID;
}

bool RouteNetwork::matchNode(const atools::routing::Node& node) const
{
  switch(node.type)
  {
    case atools::routing::VOR:
    case atools::routing::VORDME:
    case atools::routing::DME:
    case atools::routing::NDB:
      return mode & MODE_RADIONAV;

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

    // Read navaids into nodeIdIndexMap and into nodesTemp
    // Named waypoints without airways
    readNodesAirway(nodesTemp,
                    "select w.waypoint_id, w.type, w.lonx, w.laty "
                    "from waypoint w "
                    "where w.type = 'WN' and "
                    "w.num_jet_airway = 0 and w.num_victor_airway = 0",
                    currentIndex);
    // "where (w.type = 'WN' or (w.type = 'WU' and w.airport_id is null)) and "

    // Airway waypoints
    readNodesAirway(nodesTemp,
                    "select w.waypoint_id, w.type, w.num_jet_airway, w.num_victor_airway, w.lonx, w.laty "
                    "from waypoint w "
                    "where w.type like 'W%' and (w.num_jet_airway > 0 or w.num_victor_airway > 0)",
                    currentIndex);

    // Airway VOR waypoints
    readNodesAirway(nodesTemp,
                    "select w.waypoint_id, w.type, v.type as vortype, v.dme_altitude, "
                    "v.dme_only, v.range, w.num_jet_airway, w.num_victor_airway, w.lonx, w.laty "
                    "from waypoint w join vor v on w.nav_id = v.vor_id "
                    "where w.type = 'V' and (w.num_jet_airway > 0 or w.num_victor_airway > 0)",
                    currentIndex);

    // Airway NDB waypoints
    readNodesAirway(nodesTemp,
                    "select w.waypoint_id, w.type, n.type as ndbtype, n.range, "
                    "w.num_jet_airway, w.num_victor_airway, w.lonx, w.laty "
                    "from waypoint w join ndb n on w.nav_id = n.ndb_id "
                    "where w.type = 'N' and (w.num_jet_airway > 0 or w.num_victor_airway > 0)",
                    currentIndex);

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
    cols = "airway_id, minimum_altitude, maximum_altitude, airway_name, "
           "route_type, airway_type, direction, from_waypoint_id, to_waypoint_id";
  else
    cols = "airway_id, minimum_altitude, maximum_altitude, airway_name, "
           "null as route_type, airway_type, direction, from_waypoint_id, to_waypoint_id";

  SqlQuery query("select  " + cols + " from airway", db);
  query.exec();
  while(query.next())
  {
    Edge edge;
    edge.airwayId = query.valueInt(0);
    edge.minAltFt =
      static_cast<quint16>(std::min(query.valueInt(1), static_cast<int>(Edge::MAX_ALTITUDE)));
    edge.maxAltFt =
      static_cast<quint16>(std::min(query.valueInt(2), static_cast<int>(Edge::MAX_ALTITUDE)));
    edge.airwayHash = qHash(query.valueStr(3)) + 1; // Avoid null

    char routeType = atools::strToChar(query.valueStr(4));
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

    char type = atools::strToChar(query.valueStr(5));
    if(type == 'J')
      edge.type = AIRWAY_JET;
    else if(type == 'V')
      edge.type = AIRWAY_VICTOR;
    else if(type == 'B')
      edge.type = AIRWAY_BOTH;

    // Add one edge for each allowed direction
    char dir = atools::strToChar(query.valueStr(6));
    int from = query.valueInt(7);
    int to = query.valueInt(8);

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

void RouteNetwork::readNodesAirway(QVector<Node>& nodes, const QString& queryStr, int& idx)
{
  SqlQuery query(queryStr, db);
  query.exec();
  while(query.next())
  {
    Node node;
    node.index = idx;
    node.id = query.valueInt("waypoint_id");

    node.pos.setLonX(query.valueFloat("lonx"));
    node.pos.setLatY(query.valueFloat("laty"));
    node.range = nmToMeter(query.valueInt("range", 0));

    // Use number of attached airways to determine type
    QString type = query.valueStr("type");
    int numJet = query.valueInt("num_jet_airway", 0);
    int numVictor = query.valueInt("num_victor_airway", 0);
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

    if(query.hasField("vortype"))
    {
      // Query uses VOR table ====================
      QString vortype = query.valueStr("vortype");
      if(vortype == "H" || vortype == "L" || vortype == "T" || vortype.startsWith("VT"))
      {
        if(query.valueBool("dme_only"))
          node.subtype = DME;
        else
          node.subtype = query.isNull("dme_altitude") ? VOR : VORDME;
      }
    }
    else if(query.hasField("ndbtype"))
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
  // id, lonx, laty, range, has_dme
  SqlQuery query(queryStr, db);
  query.exec();
  while(query.next())
  {
    Node node;
    node.index = idx;
    node.id = query.valueInt(0);
    node.pos.setLonX(query.valueFloat(1));
    node.pos.setLatY(query.valueFloat(2));
    node.range = nmToMeter(query.valueInt(3));

    if(vor)
      node.type = query.valueBool(4) ? VORDME : VOR;
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
