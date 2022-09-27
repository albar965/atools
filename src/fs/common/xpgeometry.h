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

#ifndef ATOOLS_XPGEOMETRY_H
#define ATOOLS_XPGEOMETRY_H

#include "geo/pos.h"

namespace atools {
namespace fs {
namespace common {

struct Node
{
  atools::geo::Pos node, control;
};

typedef QVector<atools::fs::common::Node> Boundary;
typedef QVector<Boundary> Holes;

struct XpGeo
{
  atools::fs::common::Boundary boundary;
  atools::fs::common::Holes holes;
};

/*
 * X-Plane pavement geometry for common use in database and client code.
 * Writes a simple lat/long array in single floating point precision into an byte array which can be used
 * to write it into a database BLOB.
 */
class XpGeometry
{
public:
  XpGeometry(const QByteArray& bytes);
  XpGeometry();

  void addBoundaryNode(const atools::geo::Pos& node, const atools::geo::Pos& control);

  void addHoleNode(const atools::geo::Pos& node, const atools::geo::Pos& control, bool newHole);

  void readFromByteArray(const QByteArray& bytes);
  QByteArray writeToByteArray();

  bool isEmpty() const
  {
    return geometry.boundary.isEmpty();
  }

  void clear();

  const atools::fs::common::XpGeo& getGeometry() const
  {
    return geometry;
  }

private:
  void writeNode(QDataStream& out, const Node& node);
  void readNode(QDataStream& in, atools::fs::common::Node& node);

  static constexpr qint8 NODE_TYPE_LINE = 0x01;
  static constexpr qint8 NODE_TYPE_CURVE = 0x02;

  atools::fs::common::XpGeo geometry;

};

} // namespace common
} // namespace fs
} // namespace atools

#endif // ATOOLS_XPGEOMETRY_H
