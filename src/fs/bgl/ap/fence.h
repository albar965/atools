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

#ifndef ATOOLS_BGL_AP_FENCE_H
#define ATOOLS_BGL_AP_FENCE_H

#include "fs/bgl/record.h"
#include "fs/bgl/ap/rw/runway.h"
#include "fs/bgl/bglposition.h"

namespace atools {
namespace fs {
namespace bgl {

namespace fence {
enum Type
{
  UNKNOWN,
  BLAST,
  BOUNDARY
};

}

class Fence :
  public atools::fs::bgl::Record
{
public:
  Fence(const atools::fs::BglReaderOptions *options, atools::io::BinaryStream *bs);

  virtual ~Fence();

  const QList<atools::fs::bgl::BglPosition>& getVertices() const
  {
    return vertices;
  }

  atools::fs::bgl::fence::Type getType() const;

  static QString fenceTypeToStr(atools::fs::bgl::fence::Type type);

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Fence& record);

  QList<atools::fs::bgl::BglPosition> vertices;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_AP_FENCE_H
