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

#include "routing/routenetworkloader.h"

#include "sql/sqldatabase.h"
#include "sql/sqlquery.h"
#include "sql/sqlrecord.h"
#include "sql/sqlutil.h"
#include "geo/calculations.h"
#include "routing/routenetwork.h"
#include "track/tracktypes.h"
#include "io/binaryutil.h"

#include <QElapsedTimer>

using atools::sql::SqlUtil;
using atools::sql::SqlQuery;
using atools::geo::nmToMeter;
using atools::geo::Point3D;
using atools::charAt;

// Calculate hash for airway name for quick comparison in routing algorithm
inline quint32 airwayHash(const QString& name)
{
  return qHash(name) + 1; // Avoid null
}

// Calculate hash for track name and type for quick comparison in routing algorithm
inline quint32 trackHash(const QString& name, const QString& type)
{
  return (qHash(name) ^ (qHash(type) << 8) ^ 0x55555555) + 1; // Avoid null and add pattern to difference for airway
}

namespace atools {
namespace routing {

RouteNetworkLoader::RouteNetworkLoader(atools::sql::SqlDatabase *sqlDbNav, atools::sql::SqlDatabase *sqlDbTrack)
  : dbNav(sqlDbNav), dbTrack(sqlDbTrack)
{
}

RouteNetworkLoader::~RouteNetworkLoader()
{
}

void RouteNetworkLoader::load(atools::routing::RouteNetwork *networkParam)
{
  QElapsedTimer timer;
  timer.start();

  network = networkParam;
  network->clear();

  bool hasTracks = dbTrack != nullptr && SqlUtil(dbTrack).hasTableAndRows("track");
  bool hasNav = dbNav != nullptr && SqlUtil(dbNav).hasTableAndRows("waypoint");

  if(network->source == SOURCE_RADIO && dbNav != nullptr)
  {
    // Load VOR, VORDME and VORTAC. No DME and no TACAN. ==========================================
    readNodesRadio("select v.vor_id, v.lonx, v.laty, v.range, "
                   "case when v.dme_altitude is null then 0 else 1 end as has_dme "
                   "from vor v where type <> 'TC' and dme_only = 0", true);

    // Load all NDB =================================================
    readNodesRadio("select n.ndb_id, n.lonx, n.laty, n.range, null as has_dme from ndb n", false);
  }
  else if(network->source == SOURCE_AIRWAY)
  {
    // Load waypoints and airways ====================================

    // Read all airways from database into map. Edge::toIndex gets database id temporarily
    // Outgoing edges for node id
    QMultiHash<int, Edge> nodeEdgeMap;
    nodeEdgeMap.reserve(200000);

    // Read navdata edges ==========================================
    if(hasNav)
      readEdgesAirway(nodeEdgeMap, false);

    // Read track edges ==========================================
    if(hasTracks)
      readEdgesAirway(nodeEdgeMap, true /* track */);

    // Maps the database node id to index position in vector
    QHash<int, int> nodeIdIndexMap;
    nodeIdIndexMap.reserve(300000);

    // List of created nodes
    QVector<Node> nodeVector;
    nodeVector.reserve(300000);

    // Column order is important in the queries

    // Read navaids into nodeIdIndexMap and into nodesTemp ====================
    // Named and unnamed waypoints without airways as well as degree confluence waypoints
    if(hasNav)
      readNodesAirway(nodeVector, nodeIdIndexMap,
                      "select w.waypoint_id, w.ident, w.type, w.lonx, w.laty "
                      "from waypoint w "
                      "where w.type in ('WN', 'WU') and w.airport_id is null and "
                      "w.num_jet_airway = 0 and w.num_victor_airway = 0",
                      false, false, false, true /* filterProc */);

    // Airway waypoints ====================
    // No filter - all waypoints are taken
    if(hasNav)
      readNodesAirway(nodeVector, nodeIdIndexMap,
                      "select w.waypoint_id, w.ident, w.type, w.lonx, w.laty, w.num_jet_airway, w.num_victor_airway "
                      "from waypoint w "
                      "where w.type like 'W%' and (w.num_jet_airway > 0 or w.num_victor_airway > 0)",
                      false, false, false, false);

    // Track waypoints ====================
    // No filter - all waypoints are taken
    if(hasTracks)
    {
      QString where = hasNav ?
                      (" where w.trackpoint_id >= " + QString::number(atools::track::TRACKPOINT_ID_OFFSET)) :
                      QString();

      readNodesAirway(nodeVector,
                      nodeIdIndexMap,
                      "select w.trackpoint_id, w.ident, w.type, w.lonx, w.laty, w.num_jet_airway, w.num_victor_airway "
                      "from trackpoint w " + where,
                      false, false, true /* track */, false);
    }

    // Airway VOR waypoints ====================
    if(hasNav)
      readNodesAirway(nodeVector, nodeIdIndexMap,
                      "select w.waypoint_id, w.ident, w.type, w.lonx, w.laty, w.num_jet_airway, w.num_victor_airway, "
                      "v.range, v.type as radiotype, v.dme_altitude,  v.dme_only "
                      "from waypoint w join vor v on w.nav_id = v.vor_id "
                      "where w.type = 'V' and (w.num_jet_airway > 0 or w.num_victor_airway > 0)",
                      true /* VOR */, false, false, false);

    // Airway NDB waypoints ====================
    if(hasNav)
      readNodesAirway(nodeVector, nodeIdIndexMap,
                      "select w.waypoint_id, w.ident, w.type, w.lonx, w.laty, w.num_jet_airway, w.num_victor_airway, "
                      "n.range "
                      "from waypoint w join ndb n on w.nav_id = n.ndb_id "
                      "where w.type = 'N' and (w.num_jet_airway > 0 or w.num_victor_airway > 0)",
                      false, true /* NDB */, false, false);

    // Insert outgoing edges to each node and copy node to the index ========================
    network->nodeIndex.reserve(nodeVector.size());
    for(Node& node : nodeVector)
    {
      for(auto it = nodeEdgeMap.find(node.id); it != nodeEdgeMap.end() && it.key() == node.id; ++it)
        node.edges.append(it.value());

      // Replace database ids in Edge::toIndex with array indexes
      for(Edge& edge : node.edges)
        edge.toIndex = nodeIdIndexMap.value(edge.toIndex);

      network->nodeIndex.append(node);
    }
  } // else if(network->source == SOURCE_AIRWAY)

  // Update spatial index
  network->nodeIndex.updateIndex();

  // Calculate distance for all edges of all nodes and set node connection flags ================
  for(Node& node : network->nodeIndex)
  {
    atools::routing::NodeConnections connections = CONNECTION_NONE;
    for(Edge& edge : node.edges)
    {
      // Fill connection flags based on outgoing edges
      switch(edge.type)
      {
        case atools::routing::EDGE_NONE:
          break;

        case atools::routing::EDGE_VICTOR:
          connections |= CONNECTION_VICTOR;
          break;

        case atools::routing::EDGE_JET:
          connections |= CONNECTION_JET;
          break;

        case atools::routing::EDGE_BOTH:
          connections |= CONNECTION_AIRWAY_BOTH;
          break;

        case atools::routing::EDGE_TRACK:
          connections |= CONNECTION_TRACK;
          break;
      }

      // Calculate great circle distance for all edges ====================
      edge.lengthMeter = atools::roundToInt(network->nodeIndex.atPoint3D(node.index).
                                            gcDistanceMeter(network->nodeIndex.atPoint3D(edge.toIndex)));
    }

    node.setConnections(connections);
  }

  // Assign CONNECTION_TRACK_START_END to all nodes which are track end or start points
  if(hasTracks)
    readTrackStartEndPoints();

  qDebug() << Q_FUNC_INFO << timer.restart() << "ms" << "nodes" << network->getNodes().size();
}

void RouteNetworkLoader::readTrackStartEndPoints() const
{
  enum
  {
    STARTPOINT_ID,
    ENDPOINT_ID
  };

  // Build a temporary index mapping id to array index
  QHash<int, int> nodeIdIndexMap;
  for(const Node& node : network->getNodes())
    nodeIdIndexMap.insert(node.id, node.index);

  SqlQuery query("select startpoint_id, endpoint_id from trackmeta", dbTrack);
  query.exec();
  while(query.next())
  {
    network->nodeIndex[nodeIdIndexMap.value(query.valueInt(STARTPOINT_ID))].addConnection(CONNECTION_TRACK_START_END);
    network->nodeIndex[nodeIdIndexMap.value(query.valueInt(ENDPOINT_ID))].addConnection(CONNECTION_TRACK_START_END);
  }
}

void RouteNetworkLoader::readEdgesAirway(QMultiHash<int, Edge>& nodeEdgeMap, bool track) const
{
  atools::sql::SqlRecord rec;
  QString queryTxt;

  if(track)
  {
    rec = dbTrack->record("track");
    queryTxt = "select track_id, from_waypoint_id, to_waypoint_id, airway_minimum_altitude, airway_maximum_altitude, "
               "track_name, 'T' as route_type, null as airway_type, 'F' as direction, "
               "altitude_levels_east, altitude_levels_west, track_type "
               "from track";
  }
  else
  {
    rec = dbNav->record("airway");
    if(rec.contains("route_type"))
      queryTxt = "select airway_id, from_waypoint_id, to_waypoint_id, minimum_altitude, maximum_altitude, airway_name, "
                 "route_type, airway_type, direction from airway";
    else
      queryTxt = "select airway_id, from_waypoint_id, to_waypoint_id, minimum_altitude, maximum_altitude, airway_name, "
                 "null as route_type, airway_type, direction from airway";
  }

  // Column indexes
  enum
  {
    ID,
    FROM,
    TO,
    MIN,
    MAX,
    NAME,
    ROUTE_TYPE,
    AIRWAY_TYPE,
    DIRECTION,
    ALT_LEVELS_EAST,
    ALT_LEVELS_WEST,
    TRACK_TYPE
  };

  SqlQuery query(queryTxt, track ? dbTrack : dbNav);
  query.exec();
  while(query.next())
  {
    Edge edge;
    edge.id = query.valueInt(ID);

    edge.minAltFt = static_cast<quint16>(std::min(query.valueInt(MIN), static_cast<int>(Edge::MAX_ALTITUDE)));

    // Assign max altitude if given - otherwise max
    if(query.valueInt(MAX) > 0)
      edge.maxAltFt = static_cast<quint16>(std::min(query.valueInt(MAX), static_cast<int>(Edge::MAX_ALTITUDE)));
    else
      edge.maxAltFt = Edge::MAX_ALTITUDE;

    // From and to waypoint ids
    int fromId = query.valueInt(FROM);
    int toId = query.valueInt(TO);

    if(!track)
    {
      // Assign route type ======================================
      QString typeStr = query.valueStr(AIRWAY_TYPE);
      edge.airwayHash = airwayHash(query.valueStr(NAME));
      char routeType = atools::strToChar(query.valueStr(ROUTE_TYPE));
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

      // Assign airway type ======================================
      char type = atools::strToChar(typeStr);
      if(type == 'J')
        edge.type = EDGE_JET;
      else if(type == 'V')
        edge.type = EDGE_VICTOR;
      else if(type == 'B')
        edge.type = EDGE_BOTH;

      // Assign towards node ======================================
      // Add one edge for each allowed direction
      char dir = atools::strToChar(query.valueStr(DIRECTION));

      if(dir == '\0' || dir == 'F' || dir == 'N')
      {
        // Forward or both directions allowed
        // Use id temporarily - will be replaced with index later
        edge.toIndex = toId;
        nodeEdgeMap.insert(fromId, edge);
      }

      if(dir == '\0' || dir == 'B' || dir == 'N')
      {
        // Backward or both directions allowed
        // Use id temporarily - will be replaced with index later
        edge.toIndex = fromId;
        nodeEdgeMap.insert(toId, edge);
      }
    }
    else
    {
      // Calculate hash including type to avoid jumping between tracks
      edge.airwayHash = trackHash(query.valueStr(NAME), query.valueStr(TRACK_TYPE));

      /* T NAT, PACOTS or AUSOTS track */
      edge.routeType = TRACK;
      edge.type = EDGE_TRACK;

      // Read altitude levels from array ====================
      if(!query.isNull(ALT_LEVELS_EAST))
      {
        edge.hasAltLevels = true;
        network->altLevelsEast.insert(edge.id,
                                      atools::io::readVector<quint16, quint16>(
                                        query.value(ALT_LEVELS_EAST).toByteArray()));
      }

      if(!query.isNull(ALT_LEVELS_WEST))
      {
        edge.hasAltLevels = true;
        network->altLevelsWest.insert(edge.id,
                                      atools::io::readVector<quint16, quint16>(
                                        query.value(ALT_LEVELS_WEST).toByteArray()));
      }

      // Forward only track is always running from/to
      // Use id temporarily - will be replaced with index later
      edge.toIndex = toId;

      nodeEdgeMap.insert(fromId, edge);
    }
  } // while(query.next())
}

void RouteNetworkLoader::readNodesAirway(QVector<Node>& nodes, QHash<int, int>& nodeIdIndexMap,
                                         const QString& queryStr, bool vor, bool ndb,
                                         bool track, bool filterProc)
{
  // Column indexes
  // -> Required                          <- ->       if airway             <-  -> Optional
  // 0              1      2     3     4     5               6                  7      8          9             10
  // waypoint_id,   ident, type, lonx, laty, num_jet_airway, num_victor_airway, range, radiotype, dme_altitude, dme_only
  // waypoint_id,   ident, type, lonx, laty, num_jet_airway, num_victor_airway, range
  // waypoint_id,   ident, type, lonx, laty, num_jet_airway, num_victor_airway
  // trackpoint_id, ident, type, lonx, laty, num_jet_airway, num_victor_airway
  enum
  {
    ID,
    IDENT,
    AIRWAY_TYPE,
    LONX,
    LATY,
    NUM_JET_AIRWAY,
    NUM_VICTOR_AIRWAY,
    RANGE,
    RADIO_TYPE,
    DME_ALTITUDE,
    DME_ONLY
  };

  QSet<int> procNodeIds;
  if(filterProc && dbNav != nullptr && !track)
  {
    // Ignore non airway waypoints which belong to an airport and/or are part of a procedure
    // Collect ids here and filter out later to avoid costly join
    SqlQuery procWpQuery(dbNav);
    if(SqlUtil(dbNav).hasTable("approach_leg") && SqlUtil(dbNav).hasTable("transition_leg"))
    {
      procWpQuery.exec("select waypoint_id from waypoint w join "
                       " (select fix_ident, fix_region from approach_leg where fix_type in ('TW', 'W') union "
                       "  select fix_ident, fix_region from transition_leg where fix_type in ('TW', 'W')) as sub "
                       " on w.ident = sub.fix_ident and w.region = sub.fix_region "
                       " where w.num_jet_airway = 0 and w.num_victor_airway = 0 and w.airport_id is null");
      while(procWpQuery.next())
        procNodeIds.insert(procWpQuery.valueInt(0));
    }
  }

  SqlQuery query(queryStr, track ? dbTrack : dbNav);
  query.exec();
  while(query.next())
  {
    int nodeId = query.valueInt(ID);
    if(procNodeIds.contains(nodeId))
      // Not part of an airway but part of a procedure or part of an airport (terminal waypoint) - ignore
      continue;

    atools::geo::Pos pos(query.valueFloat(LONX), query.valueFloat(LATY));

    // No name and grid filter for NDB and VOR waypoints
    if(!ndb && !vor && filterProc)
    {
      // Include all one degree grid confluence points
      // Ignore half degee points
      bool ok = pos.nearGrid(1.f, atools::geo::Pos::POS_EPSILON_10M);

      // Check for visual reporting points like "VP123" as well as other obscure numbered points and ignore these
      // if not confluence points
      if(!ok)
      {
        QString ident = query.valueStr(IDENT);
        if(charAt(ident, 2).isDigit() && charAt(ident, 3).isDigit() && charAt(ident, 4).isDigit())
          continue;
      }
    } // if(!ndb && !vor)

    Node node;
    node.index = nodes.size();
    node.id = nodeId;
    node.pos = pos;
    node.type = NODE_WAYPOINT;

    if(vor || ndb)
      node.range = nmToMeter(query.valueInt(RANGE));

    if(vor)
    {
      // Query uses VOR table ====================
      QString vortype = query.valueStr(RADIO_TYPE);
      if(vortype == "H" || vortype == "L" || vortype == "T" || vortype.startsWith("VT"))
      {
        if(query.valueBool(DME_ONLY))
          node.subtype = NODE_DME;
        else
          node.subtype = query.isNull(DME_ALTITUDE) ? NODE_VOR : NODE_VORDME;
      }
    }

    if(ndb)
      // Query uses NDB table ====================
      node.subtype = NODE_NDB;

    // Connection flags are populated later by analyzing edges

    if(node.type == NODE_NONE)
      qWarning() << Q_FUNC_INFO << "No node type" << query.record();

    nodes.append(node);
    nodeIdIndexMap.insert(node.id, node.index);
  } // while(query.next())
}

void RouteNetworkLoader::readNodesRadio(const QString& queryStr, bool vor)
{
  // Column indexes
  // id, lonx, laty, range, has_dme
  enum
  {
    ID,
    LONX,
    LATY,
    RANGE,
    HAS_DME
  };

  SqlQuery query(queryStr, dbNav);
  query.exec();
  while(query.next())
  {
    Node node;
    node.index = network->nodeIndex.size();
    node.id = query.valueInt(ID);
    node.pos.setLonX(query.valueFloat(LONX));
    node.pos.setLatY(query.valueFloat(LATY));
    node.range = nmToMeter(query.valueInt(RANGE));

    if(vor)
      node.type = query.valueBool(HAS_DME) ? NODE_VORDME : NODE_VOR;
    else
      node.type = NODE_NDB;

    network->nodeIndex.append(node);
  }
}

} // namespace route
} // namespace atools
