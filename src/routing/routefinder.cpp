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
#include "geo/calculations.h"
#include "atools.h"

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
  nodeAirwayName.reserve(10000);

  successorNodes.reserve(500);
  successorEdges.reserve(500);
}

RouteFinder::~RouteFinder()
{

}

bool RouteFinder::calculateRoute(const atools::geo::Pos& from, const atools::geo::Pos& to, int flownAltitude)
{
  altitude = flownAltitude;
  network->addDepartureAndDestinationNodes(from, to);
  Node startNode = network->getDepartureNode();
  Node destNode = network->getDestinationNode();

  int numNodesTotal = network->getNumberOfNodesDatabase();

  if(startNode.edges.isEmpty())
    return false;

  openNodesHeap.push(startNode, 0.f);
  nodeCosts[startNode.id] = 0.f;
  nodeAltRange[startNode.id] = std::make_pair(0, std::numeric_limits<int>::max());

  Node currentNode;
  bool destinationFound = false;
  while(!openNodesHeap.isEmpty())
  {
    // Contains known nodes
    openNodesHeap.pop(currentNode);

    if(currentNode.id == destNode.id)
    {
      destinationFound = true;
      break;
    }

    // Contains nodes with known shortest path
    closedNodes.insert(currentNode.id);

    if(closedNodes.size() > numNodesTotal / 2)
      // If we read too much nodes routing will fail
      break;

    // Work on successors
    expandNode(currentNode, destNode);
  }

  qDebug() << "found" << destinationFound << "heap size" << openNodesHeap.size()
           << "close nodes size" << closedNodes.size();

  qDebug() << "num nodes database" << network->getNumberOfNodesDatabase()
           << "num nodes cache" << network->getNumberOfNodesCache();

  return destinationFound;
}

void RouteFinder::extractLegs(QVector<RouteLeg>& routeLegs, float& distanceMeter) const
{
  distanceMeter = 0.f;
  routeLegs.reserve(500);

  // Build route
  atools::routing::Node pred = network->getDestinationNode();
  while(pred.id != -1)
  {
    int navId;
    atools::routing::NodeType type;
    network->getNavIdAndTypeForNode(pred.id, navId, type);

    if(type != atools::routing::DEPARTURE && type != atools::routing::DESTINATION)
    {
      RouteLeg leg;
      leg.navId = navId;
      leg.type = type;
      leg.airwayId = nodeAirwayId.value(pred.id, -1);
      leg.pos = pred.pos;
      routeLegs.prepend(leg);
    }

    atools::routing::Node next = network->getNode(nodePredecessor.value(pred.id, -1));
    if(next.pos.isValid())
      distanceMeter += pred.pos.distanceMeterTo(next.pos);
    pred = next;
  }
}

void RouteFinder::expandNode(const atools::routing::Node& currentNode, const atools::routing::Node& destNode)
{
  successorNodes.clear();
  successorEdges.clear();
  network->getNeighbours(currentNode, successorNodes, successorEdges);

  QString currentNodeAirway;
  if(network->isAirwayRouting())
    currentNodeAirway = nodeAirwayName[currentNode.id];

  for(int i = 0; i < successorNodes.size(); i++)
  {
    const Node& successor = successorNodes.at(i);

    if(closedNodes.contains(successor.id))
      // Already has a shortest path
      continue;

    const Edge& edge = successorEdges.at(i);

    // Calculate set altitude if altitude > 0
    if(altitude > 0 && !(altitude >= edge.minAltFt && altitude <= edge.maxAltFt))
      // Altitude restrictions do not match - ignore this edge to the node
      continue;

    // if(edge.airwayName == "UJ3")
    // qDebug() << Q_FUNC_INFO;

    if(edge.direction == atools::routing::BACKWARD)
      // Do not travel against a one-way airway
      continue;

    int lengthMeter = edge.lengthMeter;

    if(lengthMeter == 0)
      // No distance given for airways - have to calculate this here
      lengthMeter = static_cast<int>(currentNode.pos.distanceMeterTo(successor.pos));

    float successorEdgeCosts = calculateEdgeCost(currentNode, successor, lengthMeter);

    // Avoid jumping between equal airways
    if(!currentNodeAirway.isEmpty() && !edge.airwayName.isEmpty() && currentNodeAirway != edge.airwayName)
      successorEdgeCosts *= COST_FACTOR_AIRWAY_CHANGE;

    float successorNodeCosts = nodeCosts.value(currentNode.id) + successorEdgeCosts;

    if(successorNodeCosts >= nodeCosts.value(successor.id) && openNodesHeap.contains(successor))
      // New path is not cheaper
      continue;

    std::pair<int, int> successorNodeAltRange = nodeAltRange.value(currentNode.id);

    if(!combineRanges(successorNodeAltRange, edge.minAltFt, edge.maxAltFt))
      continue;

    // New path is cheaper - update node
    nodeAirwayId[successor.id] = successorEdges.at(i).airwayId;
    if(network->isAirwayRouting())
      nodeAirwayName[successor.id] = successorEdges.at(i).airwayName;
    nodePredecessor[successor.id] = currentNode.id;
    nodeCosts[successor.id] = successorNodeCosts;
    nodeAltRange[successor.id] = successorNodeAltRange;

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

  if(network->isAirwayRouting())
  {
    if(lengthMeter > DISTANCE_LONG_AIRWAY_METER)
      // Avoid certain airway segments that have an excessive length
      costs *= COST_FACTOR_LONG_AIRWAY;
  }
  else
  {
    if((currentNode.range != 0 || successorNode.range != 0) &&
       currentNode.range + successorNode.range < lengthMeter)
      // Put higher costs on radio navaids that are not withing range
      costs *= COST_FACTOR_UNREACHABLE_RADIONAV;

    if(successorNode.type == atools::routing::VOR)
      // Prefer VOR before NDB
      costs *= COST_FACTOR_VOR;
    else if(successorNode.type == atools::routing::NDB)
      costs *= COST_FACTOR_NDB;
  }

  return costs;
}

float RouteFinder::costEstimate(const atools::routing::Node& currentNode, const atools::routing::Node& destNode)
{
  return currentNode.pos.distanceMeterTo(destNode.pos);
}

} // namespace route
} // namespace atools
