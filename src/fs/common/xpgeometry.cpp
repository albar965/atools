/*****************************************************************************
* Copyright 2015-2023 Alexander Barthel alex@littlenavmap.org
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

#include "fs/common/xpgeometry.h"

#include <QDataStream>

using atools::geo::Pos;

namespace atools {
namespace fs {
namespace common {

XpGeometry::XpGeometry(const QByteArray& bytes)
{
  readFromByteArray(bytes);
}

XpGeometry::XpGeometry()
{

}

void XpGeometry::addBoundaryNode(const geo::Pos& node, const geo::Pos& control)
{
  geometry.boundary.append({node, control});
}

void XpGeometry::addHoleNode(const geo::Pos& node, const geo::Pos& control, bool newHole)
{
  if(geometry.holes.isEmpty() || newHole)
    geometry.holes.append(Boundary());

  geometry.holes.last().append({node, control});
}

void XpGeometry::readFromByteArray(const QByteArray& bytes)
{
  clear();

  QDataStream in(bytes);
  in.setVersion(QDataStream::Qt_5_5);
  in.setFloatingPointPrecision(QDataStream::SinglePrecision);

  quint32 numNodes;
  quint16 numHoles;
  in >> numNodes;
  for(quint32 i = 0; i < numNodes; i++)
  {
    Node node;
    readNode(in, node);
    geometry.boundary.append(node);
  }

  in >> numHoles;
  for(quint16 i = 0; i < numHoles; i++)
  {
    geometry.holes.append(Boundary());
    in >> numNodes;
    for(quint32 j = 0; j < numNodes; j++)
    {
      Node node;
      readNode(in, node);
      geometry.holes.last().append(node);
    }
  }
}

QByteArray XpGeometry::writeToByteArray()
{
  QByteArray bytes;
  QDataStream out(&bytes, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_5);
  out.setFloatingPointPrecision(QDataStream::SinglePrecision);

  out << static_cast<quint32>(geometry.boundary.size());
  for(const Node& node : qAsConst(geometry.boundary))
    writeNode(out, node);

  out << static_cast<quint16>(geometry.holes.size());
  for(const Boundary& hole : qAsConst(geometry.holes))
  {
    out << static_cast<quint32>(hole.size());
    for(const Node& node : hole)
      writeNode(out, node);
  }

  return bytes;

}

void XpGeometry::writeNode(QDataStream& out, const atools::fs::common::Node& node)
{
  if(node.control.isValid())
    out << NODE_TYPE_CURVE << node.node.getLonX() << node.node.getLatY()
        << node.control.getLonX() << node.control.getLatY();
  else
    out << NODE_TYPE_LINE << node.node.getLonX() << node.node.getLatY();
}

void XpGeometry::readNode(QDataStream& in, atools::fs::common::Node& node)
{
  qint8 type;
  float lonx, laty;

  in >> type;

  in >> lonx >> laty;
  node.node = Pos(lonx, laty);

  if(type == NODE_TYPE_CURVE)
  {
    in >> lonx >> laty;
    node.control = Pos(lonx, laty);
  }
}

void XpGeometry::clear()
{
  geometry.boundary.clear();
  geometry.holes.clear();
}

} // namespace common
} // namespace fs
} // namespace atools
