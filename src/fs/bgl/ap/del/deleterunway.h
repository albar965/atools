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

#ifndef BGL_AP_DEL_DELETERUNWAY_H_
#define BGL_AP_DEL_DELETERUNWAY_H_

#include "fs/bgl/bglbase.h"
#include "fs/bgl/ap/rw/runway.h"

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {

class DeleteRunway :
  public atools::fs::bgl::BglBase
{
public:
  DeleteRunway(const BglReaderOptions *options, atools::io::BinaryStream *bs);
  virtual ~DeleteRunway();

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::DeleteRunway& record);

  atools::fs::bgl::rw::Surface surface;
  QString primaryName;
  QString secondaryName;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_AP_DEL_DELETERUNWAY_H_ */
