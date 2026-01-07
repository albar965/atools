/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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

#include "routing/routenetworktypes.h"

namespace atools {
namespace routing {

QDebug operator<<(QDebug out, const Node& obj)
{
  QDebugStateSaver saver(out);
  out.nospace().noquote() << "Node("
                          << "index " << obj.index
                          << ", id " << obj.id
                          << ", " << obj.pos
                          << ", range " << obj.range
                          << ", type " << nodeTypeToStr(obj.type)
                          << ", subtype " << nodeTypeToStr(obj.subtype)
                          << ", connections " << nodeConnectionsToStr(obj.con)
                          << ", num edges " << obj.edges.size()
                          << ")";
  return out;

}

QDebug operator<<(QDebug out, const Edge& obj)
{
  QDebugStateSaver saver(out);
  out.nospace().noquote() << "Edge("
                          << "toIndex " << obj.toIndex
                          << ", airwayId " << obj.id
                          << ", airwayHash " << obj.airwayHash
                          << ", lengthMeter " << obj.lengthMeter
                          << ")";
  return out;
}

QString nodeTypeToStr(atools::routing::NodeType type)
{
  if(type == atools::routing::NODE_NONE)
    return "NODE_NONE";
  else if(type == atools::routing::NODE_DME)
    return "NODE_DME";
  else if(type == atools::routing::NODE_VOR)
    return "NODE_VOR";
  else if(type == atools::routing::NODE_VORDME)
    return "NODE_VORDME";
  else if(type == atools::routing::NODE_NDB)
    return "NODE_NDB";
  else if(type == atools::routing::NODE_WAYPOINT)
    return "NODE_WAYPOINT";
  else if(type == atools::routing::NODE_DEPARTURE)
    return "NODE_DEPARTURE";
  else if(type == atools::routing::NODE_DESTINATION)
    return "NODE_DESTINATION";

  return QString();
}

QString nodeConnectionsToStr(NodeConnection con)
{
  QStringList text;

  if(con == CONNECTION_NONE)
    return "CONNECTION_NONE";

  if(con & CONNECTION_VICTOR)
    text.append("CONNECTION_VICTOR");
  if(con & CONNECTION_JET)
    text.append("CONNECTION_JET");
  if(con & CONNECTION_TRACK)
    text.append("CONNECTION_TRACK");
  if(con & CONNECTION_TRACK_START_END)
    text.append("CONNECTION_TRACK_START_END");

  return text.join(", ");
}

} // namespace route
} // namespace atools
