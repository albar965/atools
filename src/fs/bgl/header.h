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

/*
 * BGL file header
 */
class Header :
  public atools::fs::bgl::BglBase
{
public:
  Header()
    : magicNumber1(0), headerSize(0), lowDateTime(0), highDateTime(0), magicNumber2(0), numSections(0),
      creationTimestamp(0)
  {
  }

  /*
   * Reads the BGL file header
   */
  Header(const BglReaderOptions *options, atools::io::BinaryStream *bs);
  virtual ~Header();

  /*
   * @return Timestamp string in ISO format from the timestamp inside the BGL
   */
  QString getCreationTimestampString() const;

  /*
   * @return Seconds since epoch from the timestamp inside the BGL
   */
  time_t getCreationTimestamp() const
  {
    return creationTimestamp;
  }

  /*
   * @return Number of sections in this BGL
   */
  unsigned int getNumSections() const
  {
    return numSections;
  }

  /*
   * @return true if the magic numbers match
   */
  bool isValid() const
  {
    return valid;
  }

  static Q_DECL_CONSTEXPR unsigned int MAGIC_NUMBER1 = 0x19920201;
  static Q_DECL_CONSTEXPR unsigned int MAGIC_NUMBER2 = 0x08051803;

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Header& header);

  unsigned int magicNumber1, headerSize, lowDateTime, highDateTime, magicNumber2, numSections;
  time_t creationTimestamp;
  bool valid = false;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_HEADER_H
