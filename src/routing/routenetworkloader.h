/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_ROUTENETWORKLOADER_H
#define ATOOLS_ROUTENETWORKLOADER_H

#include "routing/routenetworktypes.h"

namespace atools {
namespace sql {
class SqlDatabase;
}

namespace routing {

class RouteNetwork;

/*
 * Loader for routing network forming a directed graph by navaid nodes and airway/track edges
 * or generated edges by neares neighbor search.
 *
 * Data is loaded from tables airway, waypoint, vor and ndb as well as optionally a track database
 * depending on source. Target is RouteNetwork with data in memory.
 */
class RouteNetworkLoader
{
public:
  /* Does not load the data */
  RouteNetworkLoader(atools::sql::SqlDatabase *sqlDbNav, sql::SqlDatabase *sqlDbTrack);
  virtual ~RouteNetworkLoader();

  /* Loads network data from databases into memory in RouteNetwork.
   * Not reentrant. */
  void load(atools::routing::RouteNetwork *networkParam);

private:
  /* Read VOR and NDB into index */
  void readNodesRadio(const QString& queryStr, bool vor);

  /* Read waypoints and airways into index */
  void readNodesAirway(QList<Node>& nodes, QHash<int, int>& nodeIdIndexMap, const QString& queryStr,
                       bool vor, bool ndb, bool track, bool filterProc);

  /* Read edges from tables airway or track.
   * nodeEdgeMap receiives a list of node ids mapped to a list of edges. */
  void readEdgesAirway(QMultiHash<int, Edge>& nodeEdgeMap, bool track) const;

  /* Reads metadata and adds CONNECTION_TRACK_START_END flag to nodes if they are a start or end of a track. */
  void readTrackStartEndPoints() const;

  atools::routing::RouteNetwork *network = nullptr;
  atools::sql::SqlDatabase *dbNav = nullptr, *dbTrack = nullptr;
};

} // namespace routing
} // namespace atools

#endif // ATOOLS_ROUTENETWORKLOADER_H
