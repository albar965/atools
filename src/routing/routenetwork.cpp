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
            nodeIndexes.insert(node.index);
          }
        }
      }
    }

    searchNearest(result, from, nmToMeter(100), nmToMeter(300), &nodeIndexes);
  }
  else
    // Find nearest navaids =======================================
    searchNearest(result, from, nmToMeter(minNearestDistanceNm), nmToMeter(maxNearestDistanceRadioNm));

  // Add destination node and calculate edges to it if in range ==========================================
  if(p3dFrom.directDistanceMeter(destinationPoint3D) < nmToMeter(nearestDestDistanceNm))
  {
    result.nodes.append(destinationNode.index);
    result.edges.append(Edge(Node::DESTINATION_INDEX, p3dFrom.gcDistanceMeter(destinationPoint3D)));
  }
}

void RouteNetwork::searchNearest(Result& result, const Node& from,
                                 float minDistanceMeter, float maxDistanceMeter, const QSet<int> *excludeIndexes) const
{
  const atools::geo::Pos *destPos = &destinationNode.pos;

  if(from.isDeparture())
  {
    minDistanceMeter = 0.f;
    destPos = nullptr;
  }

  // Nodes are already removed if they are not ahead towards destination
  QVector<int> indexes;
  nodeIndex.getRadiusIndexes(indexes, from.pos,
                             // Allow to use closest navaid at departure
                             minDistanceMeter, maxDistanceMeter, directDistanceFactor,
                             false /* sort */, destPos, excludeIndexes);

  result.nodes.reserve(indexes.size() + 10);
  result.edges.reserve(indexes.size() + 10);

  atools::geo::Point3D p3dFrom = nodeToCartesian(from);
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

void RouteNetwork::setParameters(const geo::Pos& from, const geo::Pos& to, int altitudeParam, Modes modeParam)
{
  clearParameters();

  altitude = altitudeParam;
  mode = modeParam;

  // Add departure node to network ====================
  departureNode.index = Node::DEPARTURE_INDEX;
  departureNode.pos = from;
  departureNode.type = DEPARTURE;
  departureNode.range = 0;
  departureNode.subtype = NONE;
  from.toCartesian(departurePoint3D);

  // Add destination node to network ====================
  destinationNode.index = Node::DESTINATION_INDEX;
  destinationNode.pos = to;
  destinationNode.type = DESTINATION;
  destinationNode.range = 0;
  destinationNode.subtype = NONE;
  to.toCartesian(destinationPoint3D);
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
    readNodesRadio("select v.vor_id as id, v.type, "
                   "case when v.dme_altitude is null then 0 else 1 end as has_dme, v.range, v.lonx, v.laty "
                   "from vor v where type <> 'TC' and dme_only = 0", currentIndex, true);

    // Load all NDB =================================================
    readNodesRadio("select n.ndb_id as id, n.range, n.lonx, n.laty from ndb n", currentIndex, false);
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
  SqlQuery query("select * from airway a", db);
  query.exec();
  while(query.next())
  {
    Edge edge;
    edge.airwayId = query.valueInt("airway_id");
    edge.minAltFt =
      static_cast<quint16>(std::min(query.valueInt("minimum_altitude"), static_cast<int>(Edge::MAX_ALTITUDE)));
    edge.maxAltFt =
      static_cast<quint16>(std::min(query.valueInt("maximum_altitude"), static_cast<int>(Edge::MAX_ALTITUDE)));
    edge.airwayHash = qHash(query.valueStr("airway_name")) + 1; // Avoid null

    char routeType = atools::strToChar(query.valueStr("route_type", QString()));
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

    char type = atools::strToChar(query.valueStr("airway_type"));
    if(type == 'J')
      edge.type = AIRWAY_JET;
    else if(type == 'V')
      edge.type = AIRWAY_VICTOR;
    else if(type == 'B')
      edge.type = AIRWAY_BOTH;

    // Add one edge for each allowed direction
    char dir = atools::strToChar(query.valueStr("direction"));
    int from = query.valueInt("from_waypoint_id");
    int to = query.valueInt("to_waypoint_id");

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
  SqlQuery query(queryStr, db);
  query.exec();
  while(query.next())
  {
    Node node;
    node.index = idx;
    node.id = query.valueInt("id");
    node.pos.setLonX(query.valueFloat("lonx"));
    node.pos.setLatY(query.valueFloat("laty"));
    node.range = nmToMeter(query.valueInt("range"));

    if(vor)
      node.type = query.valueBool("has_dme") ? VORDME : VOR;
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
