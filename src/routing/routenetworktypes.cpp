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

#include "routing/routenetworktypes.h"

namespace atools {
namespace routing {

QDebug operator<<(QDebug out, const Node& obj)
{
  QDebugStateSaver saver(out);
  out.nospace().noquote() << "Node("
                          << "index " << obj.index
                          << ", " << obj.pos
                          << ", range " << obj.range
                          << ", type " << nodeTypeToStr(obj.type)
                          << ", subtype " << obj.subtype
                          << ", num edges " << obj.edges.size()
                          << ")";
  return out;

}

QDebug operator<<(QDebug out, const Edge& obj)
{
  QDebugStateSaver saver(out);
  out.nospace().noquote() << "Edge("
                          << "toIndex " << obj.toIndex
                          << ", airwayId " << obj.airwayId
                          << ", airwayHash " << obj.airwayHash
                          << ", lengthMeter " << obj.lengthMeter
                          << ")";
  return out;
}

QString nodeTypeToStr(atools::routing::NodeType type)
{
  switch(type)
  {
    case atools::routing::NONE:
      return "NONE";

    case atools::routing::DME:
      return "DME";

    case atools::routing::VOR:
      return "VOR";

    case atools::routing::VORDME:
      return "VORDME";

    case atools::routing::NDB:
      return "NDB";

    case atools::routing::WAYPOINT_VICTOR:
      return "WAYPOINT_VICTOR";

    case atools::routing::WAYPOINT_JET:
      return "WAYPOINT_JET";

    case atools::routing::WAYPOINT_BOTH:
      return "WAYPOINT_BOTH";

    case atools::routing::WAYPOINT_NAMED:
      return "WAYPOINT_NAMED";

    case atools::routing::WAYPOINT_UNNAMED:
      return "WAYPOINT_UNNAMED";

    case atools::routing::DEPARTURE:
      return "DEPARTURE";

    case atools::routing::DESTINATION:
      return "DESTINATION";
  }
  return QString();
}

} // namespace route
} // namespace atools
