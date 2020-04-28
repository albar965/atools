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

#ifndef ATOOLS_ROUTEFINDER_H
#define ATOOLS_ROUTEFINDER_H

#include "util/heap.h"
#include "routing/routenetworktypes.h"

namespace atools {
namespace routing {

class RouteNetwork;

struct RouteLeg
{
  int navId, /* Network ID as used as node id in the network */
      airwayId; /* Airway ID as used in the network or -1 if not applicable */
  atools::routing::NodeType type; /* Network type */
  atools::geo::Pos pos;

  friend QDebug operator<<(QDebug out, const atools::routing::RouteLeg& obj);

};

/*
 * Calculates flight plans within a route network which can be an airway or radio navaid network.
 * Uses A* algorithm and several cost factor adjustments to get reasonable routes.
 *
 * The class has a state (i.e. start and destination) and is not re-entrant.
 */
class RouteFinder
{
public:
  /* Creates a route finder that uses the given network */
  RouteFinder(RouteNetwork *routeNetwork);
  virtual ~RouteFinder();

  /*
   * Calculates a flight plan between two points. The points are added to the network but will not be returned
   * in extractRoute.
   * @param from departure position
   * @param to destination position
   * @param flownAltitude create a flight plan using airways for the given altitude. Set to 0 to ignore.
   * @return true if a route was found and callback did not cancel
   */
  bool calculateRoute(const atools::geo::Pos& from, const atools::geo::Pos& to, int flownAltitude, Modes mode);

  /* Prefer VORs to transition from departure to airway network */
  void setPreferVorToAirway(bool value)
  {
    preferVorToAirway = value;
  }

  /* Prefer NDBs to transition from departure to airway network */
  void setPreferNdbToAirway(bool value)
  {
    preferNdbToAirway = value;
  }

  /* Extract legs of shortest route and distance not including departure and destination. */
  void extractLegs(QVector<RouteLeg>& routeLegs, float& distanceMeter) const;

  const RouteNetwork *getNetwork() const
  {
    return network;
  }

  /* Callback for progress reporting. distToDest is the direct euclidian distance in 3D space between
   * departure and destination. curDistToDest is the direct euclidian distance in 3D space of
   * the current node processed to the destination.
   * return false to stop calculation. */
  typedef std::function<bool (int distToDest, int curDistToDest)> RouteFinderCallbackType;

  void setProgressCallback(RouteFinderCallbackType progressCallback)
  {
    callback = progressCallback;
  }

  void setCostFactorForceAirways(float value)
  {
    costFactorForceAirways = value;
  }

private:
  /* Expands a node by investigating all successors */
  void expandNode(const atools::routing::Node& node, const Edge& prevEdge);

  /* Calculates the costs to travel from current to successor. Base is the distance between the nodes in meter that
   * will have several factors applied to get reasonable routes */
  int calculateEdgeCost(const atools::routing::Node& node, const atools::routing::Node& successorNode,
                        const Edge& edge, quint32 currentEdgeAirwayHash);

  bool combineRanges(quint16& min1, quint16& max1, quint16 min, quint16 max)
  {
    if(max1 < min || min1 > max)
      return false;

    min1 = std::max(min1, min);
    max1 = std::min(max1, max);
    return true;
  }

  void freeArrays();
  void allocArrays();

  /* Avoid direct waypoint connections when using airways */
  float costFactorForceAirways = 1.3f;

  /* Force algortihm to avoid direct route from start to destination */
  static Q_DECL_CONSTEXPR float COST_FACTOR_DIRECT = 2.f;
  static Q_DECL_CONSTEXPR float COST_FACTOR_NEAR_WAYPOINTS = 1.1f;
  static Q_DECL_CONSTEXPR float COST_FACTOR_FAR_WAYPOINTS = 1.1f;

  /* Prefer tracks if enabled */
  static Q_DECL_CONSTEXPR float COST_FACTOR_TRACK = 0.8f;

  /* Force algortihm to use close waypoints near start and destination */
  static Q_DECL_CONSTEXPR float COST_FACTOR_FORCE_CLOSE_NODES = 1.5f;

  /* Force algortihm to use closest radio navaids near start and destination before waypoints */
  /* Has to be smaller than COST_FACTOR_FORCE_CLOSE_NODES */
  static Q_DECL_CONSTEXPR float COST_FACTOR_FORCE_CLOSE_RADIONAV_VOR = 1.1f;
  static Q_DECL_CONSTEXPR float COST_FACTOR_FORCE_CLOSE_RADIONAV_NDB = 1.2f;

  /* Increase costs to force reception of at least one radio navaid along the route */
  static Q_DECL_CONSTEXPR float COST_FACTOR_UNREACHABLE_RADIONAV = 1.2f;

  /* Try to avoid NDBs */
  static Q_DECL_CONSTEXPR float COST_FACTOR_NDB = 1.5f;

  /* Try to avoid VORs (no DME) */
  static Q_DECL_CONSTEXPR float COST_FACTOR_VOR = 1.2f;

  /* Avoid airway changes during routing */
  static Q_DECL_CONSTEXPR float COST_FACTOR_AIRWAY_CHANGE = 1.1f;

  /* Altitude to use  for airway selection of 0 if not used */
  int altitude = 0;

  /* Used network */
  atools::routing::RouteNetwork *network;

  /* Heap structure storing the index of open nodes. Costs are based on meters plus factors as integer.
   * Sort order is defined by costs from start to node + estimate to destination */
  atools::util::Heap<int, int> openNodesHeap;

  /* Using plain arrays below to speed up access compared to hash tables
   * Positions 0 and 1 are reserved for departure and destination. 2 is invalid.
   * 3 corresponds to first index in nodeIndex.*/

  /* Nodes that have been processed already and have a known shortest path */
  bool *closedNodes = nullptr;

  /* Costs from start to this node. Maps node id to costs. Costs are distance in meter
   * adjusted by factors. */
  int *nodeCostArr = nullptr;

  /* Min and maximum altitude range of airways to this node so far */
  quint16 *nodeAltRangeMinArr = nullptr;
  quint16 *nodeAltRangeMaxArr = nullptr;

  /* Maps node index to predecessor node id */
  int *nodePredecessorArr = nullptr;

  /* Maps node index to predecessor edge - similar as above */
  atools::routing::Edge *edgePredecessorArr = nullptr;

  /* Airway name hash value for edge at index */
  quint32 *edgeNameHashArr = nullptr;

  atools::routing::Node startNode, destNode;

  /* For RouteNetwork::getNeighbours to avoid instantiations */
  atools::routing::Result successors;

  bool preferVorToAirway = false, preferNdbToAirway = false;

  RouteFinderCallbackType callback;
};

} // namespace route
} // namespace atools

#endif // ATOOLS_ROUTEFINDER_H
