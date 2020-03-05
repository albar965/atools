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

#ifndef ATOOLS_BGL_AP_APRONLIGHT_H
#define ATOOLS_BGL_AP_APRONLIGHT_H

#include "fs/bgl/record.h"
#include "fs/bgl/ap/rw/runway.h"
#include "fs/bgl/bglposition.h"

namespace atools {
namespace fs {
namespace bgl {

/*
 * Apron edge light geometry. Subrecord of airport.
 */
class ApronEdgeLight :
  public atools::fs::bgl::Record
{
public:
  ApronEdgeLight(const atools::fs::NavDatabaseOptions *options, atools::io::BinaryStream *bs);
  virtual ~ApronEdgeLight();

  /*
   * @return coordinate list that is used with the edge index list
   */
  const QList<atools::fs::bgl::BglPosition>& getVertices() const
  {
    return vertices;
  }

  /*
   * @return edgeindex list where each two index entries refer into the vertex list for one edge
   */
  const QList<int>& getEdgeIndex() const
  {
    return edges;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::ApronEdgeLight& record);

  QList<atools::fs::bgl::BglPosition> vertices;
  QList<int> edges;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_AP_APRONLIGHT_H
