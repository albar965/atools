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

#ifndef ATOOLS_ROUTENETWORKBASE_H
#define ATOOLS_ROUTENETWORKBASE_H

#include "geo/pos.h"

namespace atools {
namespace routing {

/* Data source for network. Defines which data is loaded from the database. */
enum DataSource
{
  SOURCE_NONE,
  SOURCE_RADIO, /* NDB and VOR only */
  SOURCE_AIRWAY /* Airway waypoints and other waypoints */
};

/* Network mode. Changes which edges and nodes are returned as neighbours. */
enum Mode : unsigned char
{
  MODE_NONE = 0,
  MODE_RADIONAV_VOR = 1 << 0, /* VOR/NDB to VOR/NDB */
  MODE_RADIONAV_NDB = 1 << 1, /* VOR/NDB to VOR/NDB */
  MODE_WAYPOINT = 1 << 2, /* Waypoint (no airway) */
  MODE_VICTOR = 1 << 3, /* Low airways */
  MODE_JET = 1 << 4, /* High airways */

  MODE_NO_RNAV = 1 << 5, /* Do not use RNAV airways */
  MODE_TRACK = 1 << 6, /* Use NAT, PACOTS or AUSOTS tracks */

  MODE_POINT_TO_POINT = 1 << 7, /* Calculate between selected present route legs
                                * instead of airport to airport.
                                * Sets minimum distance at departure to zero. */

  MODE_AIRWAY = MODE_VICTOR | MODE_JET,
  MODE_AIRWAY_WAYPOINT = MODE_VICTOR | MODE_JET | MODE_WAYPOINT,
  MODE_AIRWAY_TRACK = MODE_AIRWAY | MODE_TRACK,
  MODE_AIRWAY_WAYPOINT_TRACK = MODE_AIRWAY_WAYPOINT | MODE_TRACK,

  MODE_JET_WAYPOINT = MODE_JET | MODE_WAYPOINT,
  MODE_VICTOR_WAYPOINT = MODE_VICTOR | MODE_WAYPOINT,

  MODE_NAVAID = MODE_RADIONAV_VOR | MODE_RADIONAV_NDB | MODE_WAYPOINT,
  MODE_RADIONAV = MODE_RADIONAV_VOR | MODE_RADIONAV_NDB,
  MODE_ALL = MODE_AIRWAY | MODE_NAVAID,
};

Q_DECLARE_FLAGS(Modes, Mode);
Q_DECLARE_OPERATORS_FOR_FLAGS(atools::routing::Modes);

/* Type and subtype of a node */
enum NodeType : unsigned char
{
  NODE_NONE = 0,
  NODE_VOR = 1, /* Type or subtype for an airway waypoint */
  NODE_VORDME = 2, /* Type or subtype for an airway waypoint - also used for VORTAC */
  NODE_DME = 3, /* DME and TACAN are not part of the network */
  NODE_NDB = 4, /* Type or subtype for an airway waypoint */
  NODE_WAYPOINT = 5, /* Unamed waypoint */
  NODE_DEPARTURE = 6, /* User defined departure virtual node */
  NODE_DESTINATION = 7 /* User defined destination virtual node */
};

QString nodeTypeToStr(NodeType type);

/* Attached airway or tracks at a node. */
enum NodeConnection : unsigned char
{
  CONNECTION_NONE = 0, /* Single waypoint - not connected to airways */
  CONNECTION_VICTOR = 1 << 0, /* Airway waypoint */
  CONNECTION_JET = 1 << 1, /* Airway waypoint */
  CONNECTION_TRACK = 1 << 2, /* Artificial waypoint in track only */
  CONNECTION_TRACK_START_END = 1 << 3, /* Start or end of one or more tracks */
  CONNECTION_PROC = 1 << 4, /* Part of a procedure or has an airport ID - avoid in routing */

  CONNECTION_AIRWAY_BOTH = CONNECTION_JET | CONNECTION_VICTOR,
  CONNECTION_AIRWAY_TRACK = CONNECTION_AIRWAY_BOTH | CONNECTION_TRACK
};

Q_DECLARE_FLAGS(NodeConnections, NodeConnection);
Q_DECLARE_OPERATORS_FOR_FLAGS(NodeConnections);

QString nodeConnectionsToStr(NodeConnection con);

/* Edge type for airway routing */
enum EdgeType : unsigned char
{
  EDGE_NONE = 0, /* No airway edge. Typically a generated edge. */
  EDGE_VICTOR = 1, /* Airway edge as loaded from the database */
  EDGE_JET = 2, /* " */
  EDGE_BOTH = 3, /* " */
  EDGE_TRACK = 4 /* NAT, PACOTS or AUSOTS track */
};

enum RouteType : unsigned char
{
  NO_ROUTE_TYPE = '\0',
  AIRLINE = 'A', /* A Airline Airway (Tailored Data) */
  CONTROL = 'C', /* C Control (appears in DFD) */
  DIRECT = 'D', /* D Direct Route */
  HELICOPTER = 'H', /* H Helicopter Airways */
  OFFICIAL = 'O', /* O Officially Designated Airways, except RNAV, Helicopter Airways (appears in DFD) */
  RNAV = 'R', /* R RNAV Airways (appears in DFD) */
  UNDESIGNATED = 'S', /* S Undesignated ATS Route */
  TRACK = 'T' /* NAT, PACTOTS or AUSOTS track. Not a real ARINC route type. */
};

/* Network edge that connects two nodes. Is loaded from the database or
 * generated based on a nearesr neighbor query.
 * Edges form a directed graph. Nodes connected by an airway without one-way restriction
 * are connected by one edge for each direction.
 */
struct Edge
{
  static constexpr quint16 MIN_ALTITUDE = 0;
  static constexpr quint16 MAX_ALTITUDE = std::numeric_limits<quint16>::max();

  Edge()
    : toIndex(-1), lengthMeter(0), id(-1), airwayHash(0),
    minAltFt(MIN_ALTITUDE), maxAltFt(MAX_ALTITUDE), type(atools::routing::EDGE_NONE), routeType(NO_ROUTE_TYPE),
    hasAltLevels(false)
  {
  }

  Edge(int to, float distance)
    : toIndex(to), lengthMeter(static_cast<int>(distance)), id(-1), airwayHash(0),
    minAltFt(MIN_ALTITUDE), maxAltFt(MAX_ALTITUDE), type(atools::routing::EDGE_NONE), routeType(NO_ROUTE_TYPE),
    hasAltLevels(false)
  {
  }

  bool isVictorAirway() const
  {
    return type == EDGE_VICTOR || type == EDGE_BOTH;
  }

  bool isJetAirway() const
  {
    return type == EDGE_JET || type == EDGE_BOTH;
  }

  bool isTrack() const
  {
    return type == EDGE_TRACK;
  }

  bool isAnyAirway() const
  {
    return type == EDGE_JET || type == EDGE_VICTOR || type == EDGE_BOTH;
  }

  /* Virtual generated connection - no airway and no track */
  bool isNoConnection() const
  {
    return type == EDGE_NONE;
  }

  int toIndex, /* Internal index (not ID) of end node */
      lengthMeter, /* great circle distance */
      id; /* Airway or track ID from the database */
  quint32 airwayHash; /* Airway or track name hash. */
  quint16 minAltFt, maxAltFt; /* Altitude restrictions for airway edges */
  atools::routing::EdgeType type;
  RouteType routeType; /* Route according to ARINC 5.7 */
  bool hasAltLevels;

  friend QDebug operator<<(QDebug out, const atools::routing::Edge& obj);

};

/* Network node. VOR, NDB, waypoint or user defined departure/destination */
struct Node
{
  Node()
    : index(INVALID_INDEX), id(-1), range(0),
    type(atools::routing::NODE_NONE), subtype(atools::routing::NODE_NONE), con(CONNECTION_NONE)
  {
  }

  int index = -1; /* Internal index in array. */
  int id = -1; /* Database id. Either id from vor, ndb, waypoint or trackpoint. */
  int range; /* Range for a radio navaid in meter or 0 if not applicable */
  atools::geo::Pos pos;

  atools::routing::NodeType type /* VOR, NDB, ..., WAYPOINT_VICTOR, ... */,
                            subtype /* VOR, VORDME, NDB, ... for airway network if type is one of WAYPOINT_* */;
  atools::routing::NodeConnection con; /* Flags indicating all connected airways and tracks */

  QVector<Edge> edges; /* Attached outgoing edges on airway only.
                        * Do not use this since edges are already filtered by the RouteNetwork. */

  /* Default unitialized */
  constexpr static int INVALID_INDEX = -1;

  /* Departure virtual node index for nodes added by setParameters.
   * Do not change value since it is used in RouteFinder as index. */
  constexpr static int DEPARTURE_INDEX = -2;

  /* Destination virtual node index for nodes added by setParameters.
   * Do not change value since it is used in RouteFinder as index. */
  constexpr static int DESTINATION_INDEX = -3;

  bool isValid() const
  {
    return index != INVALID_INDEX;
  }

  bool isDeparture() const
  {
    return index == DEPARTURE_INDEX;
  }

  bool isDestination() const
  {
    return index == DESTINATION_INDEX;
  }

  bool hasVictorAirways() const
  {
    return (con & CONNECTION_VICTOR) == CONNECTION_VICTOR;
  }

  bool hasJetAirways() const
  {
    return (con & CONNECTION_JET) == CONNECTION_JET;
  }

  bool hasTracks() const
  {
    return (con & CONNECTION_TRACK) == CONNECTION_TRACK;
  }

  bool isTrackStartEnd() const
  {
    return (con & CONNECTION_TRACK_START_END) == CONNECTION_TRACK_START_END;
  }

  bool hasAnyAirway() const
  {
    return hasVictorAirways() || hasJetAirways();
  }

  bool operator==(const atools::routing::Node& other) const
  {
    return index == other.index;
  }

  bool operator!=(const atools::routing::Node& other) const
  {
    return !operator==(other);
  }

  const atools::geo::Pos& getPosition() const
  {
    return pos;
  }

  /* Set connections using QFlags wrapper */
  void setConnections(atools::routing::NodeConnections connections)
  {
    con = static_cast<NodeConnection>(connections.operator unsigned int());
  }

  /* Add flag to connections */
  void addConnection(atools::routing::NodeConnections connections)
  {
    setConnections(getConnections() | connections);
  }

  /* Get connections using QFlags wrapper */
  atools::routing::NodeConnections getConnections() const
  {
    return con;
  }

  friend QDebug operator<<(QDebug out, const atools::routing::Node& obj);

};

inline uint qHash(const atools::routing::Node& node)
{
  return static_cast<uint>(node.index);
}

struct Result
{
  QVector<int> nodes;
  QVector<Edge> edges;

  void clear()
  {
    nodes.clear();
    edges.clear();
  }

  void reserve(int size)
  {
    nodes.reserve(size);
    edges.reserve(size);
  }

  int size() const
  {
    return nodes.size();
  }

  bool isEmpty() const
  {
    return nodes.isEmpty();
  }

};

} // namespace route
} // namespace atools

Q_DECLARE_TYPEINFO(atools::routing::Node, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(atools::routing::Edge, Q_PRIMITIVE_TYPE);

#endif // ATOOLS_ROUTENETWORKBASE_H
