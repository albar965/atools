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

#include "routing/routefinder.h"

#include "routing/routenetwork.h"
#include "atools.h"

#include <QElapsedTimer>

using atools::geo::Pos;

namespace atools {
namespace routing {

RouteFinder::RouteFinder(RouteNetwork *routeNetwork)
  : network(routeNetwork), openNodesHeap(5000)
{
  closedNodes.reserve(10000);
  nodeCosts.reserve(10000);
  nodeAltRange.reserve(10000);
  nodePredecessor.reserve(10000);
  nodeAirwayId.reserve(10000);
  nodeAirwayHash.reserve(10000);

  successorNodes.reserve(500);
  successorEdges.reserve(500);
}

RouteFinder::~RouteFinder()
{

}

bool RouteFinder::calculateRoute(const atools::geo::Pos& from, const atools::geo::Pos& to, int flownAltitude,
                                 atools::routing::Modes modeParam)
{
  ensureNetworkLoaded();

  QElapsedTimer timer;
  timer.start();

  altitude = flownAltitude;
  mode = modeParam;
  network->ensureLoaded();
  network->setParameters(from, to, altitude, mode);
  startNode = network->getDepartureNode();
  destNode = network->getDestinationNode();
  distMeterToDest = currentDistMeterToDest = atools::roundToInt(network->getDirectDistanceMeter(startNode, destNode));

  int numNodesTotal = network->getNodes().size();

  if(startNode.edges.isEmpty())
  {
    qWarning() << Q_FUNC_INFO << "No edges for start node at" << from;
    return false;
  }

  openNodesHeap.push(startNode, 0.f);
  nodeCosts[startNode.index] = 0.f;
  nodeAltRange[startNode.index] = std::make_pair(0, std::numeric_limits<int>::max());

  Node currentNode;
  bool destinationFound = false;
  while(!openNodesHeap.isEmpty())
  {
    // Contains known nodes
    openNodesHeap.pop(currentNode);

    if(currentNode.index == destNode.index)
    {
      destinationFound = true;
      break;
    }

    // Invoke user callback if set
    if(callback)
    {
      int dist = atools::roundToInt(network->getDirectDistanceMeter(currentNode, destNode));
      // Call only if changed for more than two percent
      if(dist * 100 < currentDistMeterToDest * 98)
      {
        currentDistMeterToDest = dist;
        if(!callback(distMeterToDest, currentDistMeterToDest))
          break;
      }
    }

    // Contains nodes with known shortest path
    closedNodes.insert(currentNode.index);

    if(closedNodes.size() > numNodesTotal / 2)
      // If we read too much nodes routing will fail
      break;

    // Work on successors
    expandNode(currentNode);
  }

  qDebug() << Q_FUNC_INFO << "found" << destinationFound << "heap size" << openNodesHeap.size()
           << "close nodes size" << closedNodes.size() << timer.restart() << "ms";

  return destinationFound;
}

void RouteFinder::ensureNetworkLoaded()
{
  network->ensureLoaded();
}

void RouteFinder::expandNode(const atools::routing::Node& currentNode)
{
  successorNodes.clear();
  successorEdges.clear();
  network->getNeighbours(successorNodes, successorEdges, currentNode);

  quint32 currentNodeAirwayHash = 0;
  if(network->isAirwayRouting())
    currentNodeAirwayHash = nodeAirwayHash[currentNode.index];

  for(int i = 0; i < successorNodes.size(); i++)
  {
    const Node& successor = successorNodes.at(i);

    if(closedNodes.contains(successor.index))
      // Already has a shortest path
      continue;

    const Edge& edge = successorEdges.at(i);

    float successorEdgeCosts = calculateEdgeCost(currentNode, successor, edge.lengthMeter);

    // Avoid jumping between equal airways
    if(currentNodeAirwayHash != edge.airwayHash)
      successorEdgeCosts *= COST_FACTOR_AIRWAY_CHANGE;

    float successorNodeCosts = nodeCosts.value(currentNode.index) + successorEdgeCosts;

    if(successorNodeCosts >= nodeCosts.value(successor.index) && openNodesHeap.contains(successor))
      // New path is not cheaper
      continue;

    std::pair<int, int> successorNodeAltRange = nodeAltRange.value(currentNode.index);

    if(!combineRanges(successorNodeAltRange, edge.minAltFt, edge.maxAltFt))
      continue;

    // New path is cheaper - update node
    nodeAirwayId[successor.index] = successorEdges.at(i).airwayId;
    if(network->isAirwayRouting())
      nodeAirwayHash[successor.index] = successorEdges.at(i).airwayHash;
    nodePredecessor[successor.index] = currentNode.index;
    nodeCosts[successor.index] = successorNodeCosts;
    nodeAltRange[successor.index] = successorNodeAltRange;

    // Costs from start to successor + estimate to destination = sort order in heap
    float totalCost = successorNodeCosts + costEstimate(successor, destNode);

    if(openNodesHeap.contains(successor))
      // Update node and resort heap
      openNodesHeap.change(successor, totalCost);
    else
      openNodesHeap.push(successor, totalCost);
  }
}

bool RouteFinder::combineRanges(std::pair<int, int>& range1, int min, int max)
{
  // qDebug() << "[" << range1.first << "," << range1.second << "]" << "[" << min << "," << max << "]";
  if(range1.second < min || range1.first > max)
    return false;

  range1.first = std::max(range1.first, min);
  range1.second = std::min(range1.second, max);
  // qDebug() << "RESULT [" << range1.first << "," << range1.second << "]";
  return true;
}

float RouteFinder::calculateEdgeCost(const atools::routing::Node& currentNode,
                                     const atools::routing::Node& successorNode,
                                     int lengthMeter)
{
  float costs = lengthMeter;

  if(currentNode.type == atools::routing::DEPARTURE && successorNode.type == atools::routing::DESTINATION)
    // Avoid direct connections between departure and destination
    costs *= COST_FACTOR_DIRECT;
  else if(currentNode.type == atools::routing::DEPARTURE || successorNode.type == atools::routing::DESTINATION)
  {
    if(network->isAirwayRouting())
    {
      // From departure node to network or from network to destination
      // Calculate transition to next airway
      float airwayTransCost = 1.f;

      if(preferVorToAirway)
      {
        if(currentNode.type == atools::routing::DEPARTURE &&
           (successorNode.subtype == atools::routing::VOR || successorNode.subtype == atools::routing::VORDME))
          airwayTransCost *= COST_FACTOR_FORCE_CLOSE_RADIONAV_VOR;
        else if((currentNode.subtype == atools::routing::VOR || currentNode.subtype == atools::routing::VORDME) &&
                successorNode.type == atools::routing::DESTINATION)
          airwayTransCost *= COST_FACTOR_FORCE_CLOSE_RADIONAV_VOR;
      }

      if(preferNdbToAirway)
      {
        if((currentNode.type == atools::routing::DEPARTURE && successorNode.subtype == atools::routing::NDB) ||
           (currentNode.subtype == atools::routing::NDB && successorNode.type == atools::routing::DESTINATION))
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

  if(!network->isAirwayRouting())
  {
    if((currentNode.range != 0 || successorNode.range != 0) &&
       currentNode.range + successorNode.range < lengthMeter)
      // Put higher costs on radio navaids that are not within range
      costs *= COST_FACTOR_UNREACHABLE_RADIONAV;

    if(successorNode.type == atools::routing::VOR)
      // Prefer VOR before NDB
      costs *= COST_FACTOR_VOR;
    else if(successorNode.type == atools::routing::NDB)
      costs *= COST_FACTOR_NDB;
  }

  return costs;
}

float RouteFinder::costEstimate(const atools::routing::Node& currentNode, const atools::routing::Node& nextNode)
{
  return currentNode.pos.distanceMeterTo(nextNode.pos);
}

void RouteFinder::extractLegs(QVector<RouteLeg>& routeLegs, float& distanceMeter) const
{
  distanceMeter = 0.f;
  routeLegs.clear();
  routeLegs.reserve(500);

  // Build route
  atools::routing::Node pred = network->getDestinationNode();
  while(pred.index != -1)
  {
    if(pred.type != atools::routing::DEPARTURE && pred.type != atools::routing::DESTINATION)
    {
      RouteLeg leg;
      leg.navId = pred.id;
      leg.type = pred.type;
      leg.airwayId = nodeAirwayId.value(pred.index, -1);
      leg.pos = pred.pos;
      routeLegs.prepend(leg);
    }

    atools::routing::Node next = network->getNode(nodePredecessor.value(pred.index, -1));
    if(next.pos.isValid())
      distanceMeter += pred.pos.distanceMeterTo(next.pos);
    pred = next;
  }
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
