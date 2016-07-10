/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_BGL_AP_APRON2_H
#define ATOOLS_BGL_AP_APRON2_H

#include "fs/bgl/record.h"
#include "fs/bgl/ap/rw/runway.h"
#include "fs/bgl/bglposition.h"

namespace atools {
namespace fs {
namespace bgl {

/*
 * Second of two apron records. Subrecord of airport. This one contains a vertex list and triangle index.
 */
class Apron2 :
  public atools::fs::bgl::Record
{
public:
  Apron2(const atools::fs::BglReaderOptions *options, atools::io::BinaryStream *bs);
  virtual ~Apron2();

  atools::fs::bgl::rw::Surface getSurface() const
  {
    return surface;
  }

  /*
   * @return coordinate list that is used with the triangle index list
   */
  const QList<atools::fs::bgl::BglPosition>& getVertices() const
  {
    return vertices;
  }

  /*
   * @return Triangle index list where each three index entries refer into the vertex list for one triangle
   */
  const QList<int>& getTriangleIndex() const
  {
    return triangles;
  }

  /*
   * @return if true draw the underlying surface
   */
  bool isDrawSurface() const
  {
    return drawSurface;
  }

  /*
   * @return if true draw the detail texture surface
   */
  bool isDrawDetail() const
  {
    return drawDetail;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Apron2& record);

  atools::fs::bgl::rw::Surface surface = atools::fs::bgl::rw::UNKNOWN;
  QList<atools::fs::bgl::BglPosition> vertices;
  QList<int> triangles;

  bool drawSurface = false, drawDetail = false;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_AP_APRON2_H
