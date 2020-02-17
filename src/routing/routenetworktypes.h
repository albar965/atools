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

#ifndef ATOOLS_ROUTENETWORKBASE_H
#define ATOOLS_ROUTENETWORKBASE_H

#include "geo/pos.h"

namespace  atools {
}

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
  MODE_RADIONAV = 1 << 0, /* VOR/NDB to VOR/NDB */
  MODE_WAYPOINT = 1 << 1, /* Waypoint (no airway) */
  MODE_VICTOR = 1 << 2, /* Low airways */
  MODE_JET = 1 << 3, /* High airways */

  MODE_ALL_AIRWAY = MODE_VICTOR | MODE_JET,
  MODE_ALL_NAVAID = MODE_RADIONAV | MODE_WAYPOINT,
  MODE_ALL = MODE_ALL_AIRWAY | MODE_ALL_NAVAID,
};

Q_DECLARE_FLAGS(Modes, Mode);
Q_DECLARE_OPERATORS_FOR_FLAGS(atools::routing::Modes);

/* Type and subtype of a node */
enum NodeType : unsigned char
{
  NONE = 0,
  VOR = 1, /* Type or subtype for an airway waypoint */
  VORDME = 2, /* Type or subtype for an airway waypoint - also used for VORTAC */
  DME = 3, /* DME and TACAN are not part of the network */
  NDB = 4, /* Type or subtype for an airway waypoint */
  WAYPOINT_VICTOR = 5, /* Airway waypoint */
  WAYPOINT_JET = 6, /* Airway waypoint */
  WAYPOINT_BOTH = 7, /* Airway waypoint */
  WAYPOINT_UNNAMED = 8, /* Waypoint in airway network */
  WAYPOINT_NAMED = 9, /* Waypoint in airway network */
  DEPARTURE = 10, /* User defined departure virtual node */
  DESTINATION = 11 /* User defined destination virtual node */
};

QString nodeTypeToStr(NodeType type);

/* Edge type for airway routing */
enum EdgeType : unsigned char
{
  AIRWAY_NONE = 0, /* No airway edge. Typically a generated edge. */
  AIRWAY_VICTOR = 5, /* Airway edge as loaded from the database */
  AIRWAY_JET = 6, /* " */
  AIRWAY_BOTH = 7 /* " */
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
    : toIndex(-1), lengthMeter(0), airwayId(-1), airwayHash(0),
    minAltFt(MIN_ALTITUDE), maxAltFt(MAX_ALTITUDE), type(atools::routing::AIRWAY_NONE)
  {
  }

  Edge(int to, float distance)
    : toIndex(to), lengthMeter(static_cast<int>(distance)), airwayId(-1), airwayHash(0),
    minAltFt(MIN_ALTITUDE), maxAltFt(MAX_ALTITUDE), type(atools::routing::AIRWAY_NONE)
  {
  }

  int toIndex, /* Internal index (not ID) of end node */
      lengthMeter, /* great circle distance */
      airwayId; /* Airway ID from the database */
  quint32 airwayHash; /* Airway name hash. */
  quint16 minAltFt, maxAltFt; /* Altitude restrictions for airway edges */
  atools::routing::EdgeType type;

  friend QDebug operator<<(QDebug out, const atools::routing::Edge& obj);

};

/* Network node. VOR, NDB, waypoint or user defined departure/destination */
struct Node
{
  Node()
    : index(-1), id(-1), range(0), type(atools::routing::NONE), subtype(atools::routing::NONE)
  {
  }

  int index = -1; /* Internal index */
  int id = -1; /* Database id */
  int range; /* Range for a radio navaid in meter or 0 if not applicable */
  atools::geo::Pos pos;

  atools::routing::NodeType type /* VOR, NDB, ..., WAYPOINT_VICTOR, ... */,
                            subtype /* VOR, VORDME, NDB, ... for airway network if type is one of WAYPOINT_* */;

  QVector<Edge> edges; /* Attached outgoing edges */

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

  friend QDebug operator<<(QDebug out, const atools::routing::Node& obj);

};

inline int qHash(const atools::routing::Node& node)
{
  return node.index;
}

} // namespace route
} // namespace atools

Q_DECLARE_TYPEINFO(atools::routing::Node, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(atools::routing::Edge, Q_PRIMITIVE_TYPE);

#endif // ATOOLS_ROUTENETWORKBASE_H
