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

#ifndef ATOOLS_ROUTENETWORKWP_H
#define ATOOLS_ROUTENETWORKWP_H

#include "geo/spatialindex.h"
#include "routing/routenetworktypes.h"

namespace atools {
namespace sql {
class SqlDatabase;
}

namespace routing {

/*
 * Network forming a directed graph by navaid nodes and airway edges or generated edges by neares neighbor search.
 * The class already applies various filtering mechanisms (e.g. distance to destination) when looking for nearest nodes.
 *
 * Data is loaded from tables airway, waypoint, vor and ndb depending on source.
 *
 * The class has a state (i.e. start and destination) and is not re-entrant.
 *
 * A call to setParameters with valid departure and destination is required before using any other methods.
 */
class RouteNetwork
{
public:
  /* Does not load the data */
  RouteNetwork(atools::sql::SqlDatabase *sqlDb, atools::routing::DataSource dataSource);
  virtual ~RouteNetwork();

  /* Loads network data if not already done. */
  void ensureLoaded();
  bool isLoaded() const;

  /* Set up and prepare all queries. Data is reloaded if already done. */
  void init();

  /* Disconnect queries from database and remove departure and destination nodes */
  void deInit();

  /* Get all adjacent nodes and attached edges for the given node. Edges might be different than Node::edges.
   * Adjacent objects are filtered based on distance and type criteria like airway types.
   * Edges may be airways or generated edges by nearest neighbor search.
   * Nodes/edges having a longer distance to the destination than the origin are filtered out .*/
  void getNeighbours(atools::routing::Result& result, const atools::routing::Node& origin) const;

  /* Same as above but uses a the nearest node for the position. */
  void getNeighbours(atools::routing::Result& result, const atools::geo::Pos& origin) const
  {
    getNeighbours(result, getNearestNode(origin));
  }

  /* Get great circle distance between two nodes. The calculation in euclidian 3D space
   * which is used here is faster than the usual haversine formula. */
  float getGcDistanceMeter(const atools::routing::Node& node1, const atools::routing::Node& node2) const
  {
    return nodeToCartesian(node1).gcDistanceMeter(nodeToCartesian(node2));
  }

  /* Get direct euclidian distance (tunnel-through distance) in 3D space between two nodes.
   * The calculation is more efficient than getGcDistanceMeter
   * but underestimates the distance. */
  float getDirectDistanceMeter(const atools::routing::Node& node1, const atools::routing::Node& node2) const
  {
    return nodeToCartesian(node1).directDistanceMeter(nodeToCartesian(node2));
  }

  /* Integrate departure and destination positions into the network as virtual nodes/edges.
   * Altitude is used to filter airway edges if > 0. Modes provides and additional neighbor filter. */
  void setParameters(const atools::geo::Pos& departurePos, const atools::geo::Pos& destinationPos,
                     int altitudeParam, atools::routing::Modes modeParam);

  /* Reset all parameters set by above method*/
  void clearParameters();

  /* Get the virtual departure node that was added using setParameters */
  const atools::routing::Node& getDepartureNode() const
  {
    return departureNode;
  }

  /* Get the virtual destination node that was added using setParameters */
  const atools::routing::Node& getDestinationNode() const
  {
    return destinationNode;
  }

  /* Get a node by routing network node index. If index is -1 an invalid node with id -1 is returned */
  const atools::routing::Node& getNode(int index) const;

  /* Get a single nearest node to the position. */
  const atools::routing::Node& getNearestNode(const atools::geo::Pos& pos) const
  {
    return nodeIndex.getNearest(pos);
  }

  /* Get a node by database id ("waypoint_id" or other navaid id) */
  const atools::routing::Node& getNodeById(int navId) const
  {
    return nodeIndex.at(nodeIdIndexMap.value(navId));
  }

  /* Get nodes vector. The index parameter can be used to access nodes fast.*/
  const QVector<atools::routing::Node>& getNodes() const
  {
    return nodeIndex;
  }

  /* true if airways and other navaids are used as data source. Otherwise radio navaid only. */
  bool isAirwayRouting() const
  {
    return source == SOURCE_AIRWAY;
  }

  bool isRadionavRouting() const
  {
    return source == SOURCE_RADIO;
  }

  /* Minimum distance for neares neighbor search. Not for airways. */
  void setMinNearestDistanceRadioNm(float value);

  /* Maximum distance for neares neighbor search. Not for airways (SOURCE_AIRWAY) but for waypoints. */
  void setMinNearestDistanceWpNm(float value);

  /* Maximum distance for nearest neighbor search. Only for radionav search (SOURCE_RADIO). */
  void setMaxNearestDistanceRadioNm(float value);

  /* Maximum distance for nearest neighbor search. Only for airway/waypoint search (SOURCE_AIRWAY). */
  void setMaxNearestDistanceWpNm(float value);

  /* Search distance for nearest nodes around start node added using setParameters */
  void setNearestDepartureDistanceNm(float value);

  /* Search distance for destination node added using setParameters */
  void setNearestDestDistanceNm(float value);

  /* Include only points where the total distance (origin->current->destination) is not
   * bigger than the direct connection (origin->destination * directDistanceFactor).
   * Value must be bigger than 1 */
  void setDirectDistanceFactorWp(float value)
  {
    directDistanceFactorWp = value;
  }

  /* Same as above but for radionav search (SOURCE_RADIO). */
  void setDirectDistanceFactorRadio(float value)
  {
    directDistanceFactorRadio = value;
  }

  /* Same as above but for airways (SOURCE_AIRWAY). */
  void setDirectDistanceFactorAirway(float value)
  {
    directDistanceFactorAirway = value;
  }

private:
  void clearIndexes();
  void initInternal();

  /* Read VOR and NDB into index */
  void readNodesRadio(const QString& queryStr, int& idx, bool vor);

  /* Read waypoints and airways into index */
  void readNodesAirway(QVector<Node>& nodes, const QString& queryStr, int& idx, bool vor, bool ndb, bool airway);
  void readEdgesAirway(QMultiHash<int, Edge>& nodeEdgeMap) const;

  /* Get nearest nodes and edges */
  void searchNearest(atools::routing::Result& result, const Node& origin, float minDistanceMeter,
                     float maxDistanceMeter, const QSet<int> *excludeIndexes = nullptr) const;

  /* Check node filter based on mode. */
  bool matchNode(int index) const;

  atools::geo::Point3D nodeToCartesian(const atools::routing::Node& node) const
  {
    return node.index >= 0 ? nodeIndex.atPoint3D(node.index) : node.pos.toCartesian();
  }

  /* Check if altitude and RNAV constraints allow to use this edge */
  bool matchEdge(const atools::routing::Edge& edge) const;

  /* Get point in 3D space. Returns destination or departure for appropriate indexes. */
  const atools::geo::Point3D& point3D(int index) const;

  /* All distances in meter */
  float minNearestDistanceRadioM, maxNearestDistanceRadioM,
        minNearestDistanceWpM, maxNearestDistanceWpM,
        nearestDepartureDistanceM, nearestDestDistanceM;

  float directDistanceFactorRadio, directDistanceFactorWp, directDistanceFactorAirway;

  /* Used to filter airway edges by altitude restrictions. */
  int altitude = 0;

  /* Filter for getNeighbours */
  atools::routing::Modes mode = atools::routing::MODE_ALL;
  atools::sql::SqlDatabase *db;

  atools::routing::Node departureNode, destinationNode;
  atools::geo::Point3D departurePoint, destinationPoint;
  float routeDirectDistance = 0.f, routeGcDistance = 0.f;

  /* Spatial index for nearest neighbor search using KD-tree internally */
  atools::geo::SpatialIndex<Node> nodeIndex;

  /* Maps database ids to index */
  QHash<int, int> nodeIdIndexMap;

  atools::routing::DataSource source = atools::routing::SOURCE_NONE;
};

} // namespace routing
} // namespace atools

#endif // ATOOLS_ROUTENETWORKWP_H
