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

#include "geo/calculations.h"

using atools::geo::nmToMeter;
using atools::geo::Point3D;

namespace atools {
namespace routing {

RouteNetwork::RouteNetwork(atools::routing::DataSource dataSource)
  : source(dataSource)
{
  // Default values
  nearestDepartureDistanceM = nmToMeter(500.f);
  nearestDestDistanceM = nmToMeter(500.f);

  minNearestDistanceRadioM = nmToMeter(20.f);
  maxNearestDistanceRadioM = nmToMeter(1000.f);

  minNearestDistanceWpM = nmToMeter(50.f);
  maxNearestDistanceWpM = nmToMeter(200.f);

  directDistanceFactorRadio = 1.2f;
  directDistanceFactorWp = 1.05f;
  directDistanceFactorAirway = 1.2f;
}

RouteNetwork::~RouteNetwork()
{
}

void RouteNetwork::getNeighbours(Result& result, const Node& origin, const Edge *prevEdge) const
{
  Q_ASSERT(destinationNode.isValid());
  Q_ASSERT(departureNode.isValid());

  // Node might be also departure or destination
  Point3D originPoint = point3D(origin.index);
  float originToDestDist = originPoint.directDistanceMeter(destinationPoint);

  // Check for track/non-track or non-track/track tansition if true
  // Limits neighbours if origin is in the middle of a track and not an endpoint
  bool originNotTrackEnd = source == SOURCE_AIRWAY && mode & MODE_TRACK &&
                           prevEdge != nullptr && !origin.isTrackStartEnd();

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
        if(!matchEdge(edge))
          continue;

        const Node& node = nodeIndex.at(edge.toIndex);
        // Check if node type matches like airway type
        if(!matchNode(node))
          continue;

        // Avoid track transitions at the wrong points
        if(originNotTrackEnd &&
           // Do not traverse between track and airway
           (prevEdge->isTrack() != edge.isTrack() ||
            // and not between different tracks
            (prevEdge->isTrack() && edge.isTrack() && prevEdge->airwayHash != edge.airwayHash)))
          continue;

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

    // Additionally search for direct waypoint connections if result is limited
    if((mode & MODE_WAYPOINT && result.size() < 2) || origin.isDeparture())
    {
      int found = searchNearest(result, origin, minNearestDistanceWpM, maxNearestDistanceWpM, &nodeIndexes);

      if(found < 6)
        // Not enough results - try with larger search radius
        searchNearest(result, origin, minNearestDistanceWpM * 2, maxNearestDistanceWpM * 5, &nodeIndexes);

      // Check for track transitions and remove any edges/nodes beginning from the end of the list
      if(originNotTrackEnd)
      {
        int size = result.edges.size();
        for(int i = size - 1; i >= 0; i--)
        {
          if(prevEdge->isTrack() != result.edges.at(i).isTrack())
          {
            result.nodes.removeAt(i);
            result.edges.removeAt(i);
          }
        }
      }
    }
  }
  else
    // Find nearest navaids =======================================
    searchNearest(result, origin, minNearestDistanceRadioM, maxNearestDistanceRadioM);

  // Add destination node and calculate edges to it if in range ==========================================
  if(originToDestDist < nearestDestDistanceM)
  {
    // Avoid jumping directly into a track
    if(!(originNotTrackEnd && prevEdge->isTrack()))
    {
      result.nodes.append(destinationNode.index);
      result.edges.append(Edge(Node::DESTINATION_INDEX, originPoint.gcDistanceMeter(destinationPoint)));
    }
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
        // Allow 500 meters more for waypoints at or behind position
        ok &= curToDestDist < originToDestDist + 100.f;
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

  if(origin.isDeparture())
    // Allow all points close to departure
    callbackObj.radiusMin = 0.f;
  else if(callbackObj.radionav)
    // Do not use minimum near near destination or at departure
    callbackObj.radiusMin = maxDistanceMeter > callbackObj.originToDestDist ? 0.f : minDistanceMeter;
  else
    // No minimum near departure and destination
    callbackObj.radiusMin = maxDistanceMeter > callbackObj.originToDestDist ? minDistanceMeter / 4.f : minDistanceMeter;

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
    if(matchNode(nodeIndex.at(idx)))
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
    departureNode.type = NODE_DEPARTURE;
    departureNode.range = 0;
    departureNode.subtype = NODE_NONE;
    departureNode.con = CONNECTION_NONE;
    departurePos.toCartesian(departurePoint);
  }

  if(destinationPos.isValid())
  {
    // Add destination node to network ====================
    destinationNode.index = Node::DESTINATION_INDEX;
    destinationNode.pos = destinationPos;
    destinationNode.type = NODE_DESTINATION;
    destinationNode.range = 0;
    destinationNode.subtype = NODE_NONE;
    destinationNode.con = CONNECTION_NONE;
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

bool RouteNetwork::matchNode(const Node& node) const
{
  atools::routing::NodeType nodeType = node.type;
  bool ok = true;

  switch(nodeType)
  {
    case atools::routing::NODE_VOR:
    case atools::routing::NODE_VORDME:
    case atools::routing::NODE_DME:
      ok &= mode & MODE_RADIONAV_VOR;
      break;

    case atools::routing::NODE_NDB:
      ok &= mode & MODE_RADIONAV_NDB;
      break;

    case atools::routing::NODE_WAYPOINT:
      {
        // Check if track or airway type matches filter mode
        atools::routing::NodeConnections con = node.con;
        bool wp = mode.testFlag(MODE_WAYPOINT);
        ok &= con.testFlag(CONNECTION_JET) ? mode.testFlag(MODE_JET) | wp : true;
        ok &= con.testFlag(CONNECTION_VICTOR) ? mode.testFlag(MODE_VICTOR) | wp : true;
        ok &= con.testFlag(CONNECTION_TRACK) ? mode.testFlag(MODE_TRACK) | wp : true;
        ok &= con == CONNECTION_NONE ? wp : true;
      }
      break;

    case atools::routing::NODE_NONE:
    case atools::routing::NODE_DEPARTURE:
    case atools::routing::NODE_DESTINATION:
      break;
  }
  return ok;
}

bool RouteNetwork::matchEdge(const Edge& edge) const
{
  bool ok = (altitude == 0 || (altitude >= edge.minAltFt && altitude <= edge.maxAltFt));

  // Check if RNAV has to be excluded
  if(mode & MODE_NO_RNAV)
    ok &= edge.routeType != RNAV;

  // Check if track or airway type matches filter mode
  if(ok)
    ok &= (edge.isJetAirway() && mode & MODE_JET) ||
          (edge.isVictorAirway() && mode & MODE_VICTOR) ||
          (edge.isNoConnection() && mode & MODE_WAYPOINT) ||
          (edge.isTrack() && mode & MODE_TRACK);

  // Test altitude levels if attached - independent of direction
  if(ok && altitude > 0 && edge.hasAltLevels)
  {
    int level = altitude / 100;

    if(altLevelsEast.contains(edge.id))
      ok &= altLevelsEast.value(edge.id).contains(static_cast<quint16>(level));
    if(altLevelsWest.contains(edge.id))
      ok &= altLevelsWest.value(edge.id).contains(static_cast<quint16>(level));
  }

  return ok;
}

void RouteNetwork::clear()
{
  clearParameters();
  nodeIndex.clear();
  nodeIndex.updateIndex();
  altLevelsEast.clear();
  altLevelsWest.clear();
}

bool RouteNetwork::isLoaded() const
{
  return !nodeIndex.isEmpty();
}

} // namespace route
} // namespace atools
