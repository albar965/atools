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

#include "routing/routefinder.h"

#include "routing/routenetwork.h"
#include "routing/routenetworkloader.h"
#include "atools.h"
#include "geo/calculations.h"

#include <QDateTime>
#include <QElapsedTimer>

using atools::geo::Pos;

namespace atools {
namespace routing {

template<typename TYPE>
inline TYPE& at(TYPE *arr, int index)
{
  // Relies on RouteNetwork::DEPARTURE_NODE_INDEX and RouteNetwork::DESTINATION_NODE_INDEX
  return arr[index + 3];
}

RouteFinder::RouteFinder(RouteNetwork *routeNetwork)
  : network(routeNetwork), openNodesHeap(5000)
{
  successors.reserve(500);
}

RouteFinder::~RouteFinder()
{
  freeArrays();
}

bool RouteFinder::calculateRoute(const atools::geo::Pos& from, const atools::geo::Pos& to, int flownAltitude,
                                 atools::routing::Modes mode)
{
  qDebug() << Q_FUNC_INFO << "from" << from << "to" << to << "altitude" << flownAltitude << "mode" << mode;

  allocArrays();

  QElapsedTimer timer;
  timer.start();

  altitude = flownAltitude;
  network->setParameters(from, to, altitude, mode);
  startNode = network->getDepartureNode();
  destNode = network->getDestinationNode();
  int totalDist = atools::roundToInt(network->getDirectDistanceMeter(startNode, destNode));
  int lastDist = totalDist;

  openNodesHeap.pushData(startNode.index, 0.f);
  at(nodeAltRangeMaxArr, startNode.index) = std::numeric_limits<quint16>::max();

  qint64 time = QDateTime::currentMSecsSinceEpoch();

  Node currentNode;
  bool destinationFound = false;
  while(!openNodesHeap.isEmpty())
  {
    // Contains known nodes
    int currentIndex = openNodesHeap.popData();

    if(currentIndex == destNode.index)
    {
      destinationFound = true;
      break;
    }

    currentNode = network->getNode(currentIndex);

    // Invoke user callback if set
    if(callback)
    {
      qint64 now = QDateTime::currentMSecsSinceEpoch();
      if(now > time + 200)
      {
        int dist = atools::roundToInt(network->getDirectDistanceMeter(currentNode, destNode));
        if(dist < lastDist)
          lastDist = dist;
        time = now;

        if(!callback(totalDist, lastDist))
          break;
      }
    }

    // Contains nodes with known shortest path
    at(closedNodes, currentNode.index) = true;

    // Work on successors
    expandNode(currentNode, at(edgePredecessorArr, currentNode.index));
  }

  qDebug() << Q_FUNC_INFO << "found" << destinationFound << "heap size" << openNodesHeap.size()
           << timer.restart() << "ms";

  return destinationFound;
}

void RouteFinder::expandNode(const atools::routing::Node& currentNode, const atools::routing::Edge& prevEdge)
{
  successors.clear();
  network->getNeighbours(successors, currentNode, &prevEdge);

  quint32 currentEdgeAirwayHash = 0;
  if(network->isAirwayRouting())
    currentEdgeAirwayHash = at(edgeNameHashArr, currentNode.index);

  for(int i = 0; i < successors.nodes.size(); i++)
  {
    int successorIndex = successors.nodes.at(i);

    if(at(closedNodes, successorIndex))
      // Already has a shortest path
      continue;

    const Node& successor = network->getNode(successorIndex);
    const Edge& edge = successors.edges.at(i);

    float successorEdgeCosts = calculateEdgeCost(currentNode, successor, edge);

    // Avoid jumping between equal airways or tracks
    if(currentEdgeAirwayHash != edge.airwayHash)
      successorEdgeCosts *= /*edge.isTrack() ? COST_FACTOR_TRACK_CHANGE :*/ COST_FACTOR_AIRWAY_CHANGE;

    float successorNodeCosts = at(nodeCostArr, currentNode.index) + successorEdgeCosts;

    if(successorNodeCosts >= at(nodeCostArr, successorIndex) && openNodesHeap.contains(successorIndex))
      // New path is not cheaper
      continue;

    quint16 successorNodeAltRangeMin = at(nodeAltRangeMinArr, currentNode.index);
    quint16 successorNodeAltRangeMax = at(nodeAltRangeMaxArr, currentNode.index);

    if(!combineRanges(successorNodeAltRangeMin, successorNodeAltRangeMax, edge.minAltFt, edge.maxAltFt))
      continue;

    // New path is cheaper - update node
    at(edgePredecessorArr, successorIndex) = successors.edges.at(i);
    if(network->isAirwayRouting())
      at(edgeNameHashArr, successorIndex) = successors.edges.at(i).airwayHash;
    at(nodePredecessorArr, successorIndex) = currentNode.index;
    at(nodeCostArr, successorIndex) = successorNodeCosts;
    at(nodeAltRangeMinArr, successorIndex) = successorNodeAltRangeMin;
    at(nodeAltRangeMaxArr, successorIndex) = successorNodeAltRangeMax;

    // Costs from start to successor + estimate to destination = sort order in heap
    float totalCost = successorNodeCosts + costEstimate(successor, destNode);

    if(openNodesHeap.contains(successorIndex))
      // Update node and resort heap
      openNodesHeap.change(successorIndex, totalCost);
    else
      openNodesHeap.push(successorIndex, totalCost);
  }
}

bool RouteFinder::combineRanges(quint16 min1, quint16 max1, quint16 min, quint16 max)
{
  if(max1 < min || min1 > max)
    return false;

  min1 = std::max(min1, min);
  max1 = std::min(max1, max);
  return true;
}

float RouteFinder::calculateEdgeCost(const atools::routing::Node& currentNode,
                                     const atools::routing::Node& successorNode,
                                     const atools::routing::Edge& edge)
{
  float costs = edge.lengthMeter;

  if(currentNode.type == NODE_DEPARTURE && successorNode.type == NODE_DESTINATION)
    // Avoid direct connections between departure and destination
    costs *= COST_FACTOR_DIRECT;
  else if(currentNode.type == NODE_DEPARTURE || successorNode.type == NODE_DESTINATION)
  {
    if(network->isAirwayRouting())
    {
      // From departure node to network or from network to destination
      // Calculate transition to next airway
      float airwayTransCost = 1.f;

      if(preferVorToAirway)
      {
        if(currentNode.isDeparture() && (successorNode.subtype == NODE_VOR || successorNode.subtype == NODE_VORDME))
          airwayTransCost *= COST_FACTOR_FORCE_CLOSE_RADIONAV_VOR;
        else if((currentNode.subtype == NODE_VOR || currentNode.subtype == NODE_VORDME) &&
                successorNode.isDestination())
          airwayTransCost *= COST_FACTOR_FORCE_CLOSE_RADIONAV_VOR;
      }

      if(preferNdbToAirway)
      {
        if((currentNode.type == NODE_DEPARTURE && successorNode.subtype == NODE_NDB) ||
           (currentNode.subtype == NODE_NDB && successorNode.type == NODE_DESTINATION))
          airwayTransCost *= COST_FACTOR_FORCE_CLOSE_RADIONAV_NDB;
      }

      if(airwayTransCost > 1.f)
        // Prefer VOR or NDB
        costs *= airwayTransCost;
      else
        // Prefer any waypoint to get to the next airway
        costs *= COST_FACTOR_FORCE_CLOSE_NODES;
    }
  }

  if(network->isAirwayRouting())
  {
    if(edge.isNoConnection())
    {
      // No airway - direct connection ======================
      costs *= costFactorForceAirways;

      if(edge.lengthMeter > atools::geo::nmToMeter(300))
        costs *= COST_FACTOR_FAR_WAYPOINTS;
      else if(edge.lengthMeter < atools::geo::nmToMeter(25))
        costs *= COST_FACTOR_NEAR_WAYPOINTS;
    }
    else if(edge.isTrack() && network->getMode() & MODE_TRACK)
      // Track ======================
      costs *= COST_FACTOR_TRACK;
  }
  else
  {
    if((currentNode.range != 0 || successorNode.range != 0) &&
       currentNode.range + successorNode.range < edge.lengthMeter)
      // Put higher costs on radio navaids that are not within range
      costs *= COST_FACTOR_UNREACHABLE_RADIONAV;

    if(successorNode.type == NODE_VOR)
      // Prefer VOR before NDB
      costs *= COST_FACTOR_VOR;
    else if(successorNode.type == NODE_NDB)
      costs *= COST_FACTOR_NDB;
  }

  return costs;
}

float RouteFinder::costEstimate(const atools::routing::Node& currentNode, const atools::routing::Node& nextNode)
{
  return network->getGcDistanceMeter(currentNode, nextNode);
}

void RouteFinder::extractLegs(QVector<RouteLeg>& routeLegs, float& distanceMeter) const
{
  distanceMeter = 0.f;
  routeLegs.clear();
  routeLegs.reserve(500);

  // Build route
  Node pred = network->getDestinationNode();
  while(pred.index != -1)
  {
    if(pred.type != NODE_DEPARTURE && pred.type != NODE_DESTINATION)
    {
      RouteLeg leg;
      leg.navId = pred.id;
      leg.type = pred.type;
      leg.airwayId = at(edgePredecessorArr, pred.index).id;
      leg.pos = pred.pos;
      routeLegs.prepend(leg);
    }

    Node next = network->getNode(at(nodePredecessorArr, pred.index));
    if(next.pos.isValid())
      distanceMeter += pred.pos.distanceMeterTo(next.pos);
    pred = next;
  }
}

void RouteFinder::allocArrays()
{
  freeArrays();
  // Reserve space at beginning for start and destination node
  // Relies on RouteNetwork::DEPARTURE_NODE_INDEX and RouteNetwork::DESTINATION_NODE_INDEX
  int num = network->getNodes().size() + 3;

  edgeNameHashArr = atools::allocArray<quint32>(num);
  nodeCostArr = atools::allocArray<float>(num);
  nodeAltRangeMinArr = atools::allocArray<quint16>(num);
  nodeAltRangeMaxArr = atools::allocArray<quint16>(num);
  nodePredecessorArr = atools::allocArray<int>(num, -1);
  edgePredecessorArr = atools::allocArray<Edge>(num, Edge());
  closedNodes = atools::allocArray<bool>(num);
}

void RouteFinder::freeArrays()
{
  atools::freeArray(edgeNameHashArr);
  atools::freeArray(nodeCostArr);
  atools::freeArray(nodeAltRangeMinArr);
  atools::freeArray(nodeAltRangeMaxArr);
  atools::freeArray(nodePredecessorArr);
  atools::freeArray(edgePredecessorArr);
  atools::freeArray(closedNodes);
}

QDebug operator<<(QDebug out, const RouteLeg& obj)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << "RouteLeg("
                          << "id " << obj.navId
                          << ", airway " << obj.airwayId
                          << ", " << obj.pos
                          << ", type " << obj.type
                          << ")";
  return out;
}

} // namespace route
} // namespace atools
