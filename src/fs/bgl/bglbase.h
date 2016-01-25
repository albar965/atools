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

#ifndef BGL_BGLBASE_H_
#define BGL_BGLBASE_H_

#include "logging/loggingdefs.h"

namespace atools {

namespace io {
class BinaryStream;
}

namespace fs {
namespace bgl {

class BglBase
{
public:
  virtual ~BglBase();

  void seekToStart();

  qint64 getStartOffset() const
  {
    return startOffset;
  }

protected:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::BglBase& base);

  BglBase();

  BglBase(atools::io::BinaryStream *stream);

  qint64 startOffset;

  atools::io::BinaryStream *bs;
};

} // namespace fs
} // namespace atools
} // namespace bgl

#endif /* BGL_BGLBASE_H_ */
