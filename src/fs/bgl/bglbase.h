/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_BGL_BGLBASE_H
#define ATOOLS_BGL_BGLBASE_H

#include <QDebug>

namespace atools {

namespace io {
class BinaryStream;
}

namespace fs {
class NavDatabaseOptions;
namespace bgl {

/*
 * Base class for all objects/records that are read from BGL files. Does not read anything from
 * the stream but just stores the current stream offset.
 */
class BglBase
{
public:
  BglBase();
  BglBase(const atools::fs::NavDatabaseOptions *options, atools::io::BinaryStream *stream);

  virtual ~BglBase();

  BglBase(const atools::fs::bgl::BglBase& other)
  {
    this->operator=(other);

  }

  BglBase& operator=(const atools::fs::bgl::BglBase& other);

  /*
   * Seek stream back to record start offset.
   */
  void seekToStart();

  /*
   * Get start offset of the record.
   */
  qint64 getStartOffset() const
  {
    return startOffset;
  }

protected:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::BglBase& base);

  qint64 startOffset;

  atools::io::BinaryStream *bs = nullptr;
  const atools::fs::NavDatabaseOptions *opts = nullptr;
};

} // namespace fs
} // namespace atools
} // namespace bgl

#endif // ATOOLS_BGL_BGLBASE_H
