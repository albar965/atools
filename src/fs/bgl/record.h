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

#ifndef BGL_RECORD_H_
#define BGL_RECORD_H_

#include "fs/bgl/bglbase.h"

namespace atools {
namespace io {
class BinaryStream;
}

namespace fs {
namespace bgl {

class Subsection;

class Record :
  public atools::fs::bgl::BglBase
{
public:
  Record()
    : id(0), size(0)
  {
  }

  Record(const atools::fs::BglReaderOptions *options, atools::io::BinaryStream *bs);

  virtual ~Record();

  void seekToEnd() const;

  int getSize() const
  {
    return size;
  }

  template<typename ENUM>
  ENUM getId() const
  {
    return static_cast<ENUM>(id);
  }

protected:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Record& record);

  int id, size;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_RECORD_H_ */
