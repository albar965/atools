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

#ifndef ATOOLS_BGL_HEADER_H
#define ATOOLS_BGL_HEADER_H

#include "fs/bgl/bglbase.h"

#include <QDateTime>

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {

class Header :
  public atools::fs::bgl::BglBase
{
public:
  Header()
    : magicNumber1(0), headerSize(0), lowDateTime(0), highDateTime(0), magicNumber2(0), numSections(0),
      creationTimestamp(0)
  {
  }

  Header(const BglReaderOptions *options, atools::io::BinaryStream *bs);
  virtual ~Header();

  QString getCreationTimestampString() const
  {
    QDateTime dt;
    dt.setTime_t(creationTimestamp);
    return dt.toString(Qt::ISODate);
  }

  time_t getCreationTimestamp() const
  {
    return creationTimestamp;
  }

  unsigned int getNumSections() const
  {
    return numSections;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Header& header);

  unsigned int magicNumber1, headerSize, lowDateTime, highDateTime, magicNumber2, numSections;
  time_t creationTimestamp;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_HEADER_H
