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
  {
  }

  /*
   * Reads the BGL file header
   */
  Header(const NavDatabaseOptions *options, atools::io::BinaryStream *bs);

  virtual ~Header();

  Header(const atools::fs::bgl::Header& other)
    : atools::fs::bgl::BglBase(other)
  {
    this->operator=(other);

  }

  Header& operator=(const atools::fs::bgl::Header& other)
  {
    magicNumber1 = other.magicNumber1;
    headerSize = other.headerSize;
    lowDateTime = other.lowDateTime;
    highDateTime = other.highDateTime;
    magicNumber2 = other.magicNumber2;
    numSections = other.numSections;
    creationTimestamp = other.creationTimestamp;
    validMagicNumber = other.validMagicNumber;
    validSize = other.validSize;
    read = other.read;

    return *this;
  }

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
  bool hasValidMagicNumber() const
  {
    return validMagicNumber;
  }

  /*
   * @return true if the header size is valid (0x38)
   */
  bool hasValidSize() const
  {
    return validSize;
  }

  bool isValid() const
  {
    return hasValidMagicNumber() && hasValidSize() && wasRead();
  }

  bool wasRead() const
  {
    return read;
  }

  static const unsigned int HEADER_SIZE = 0x38;

private:
  const unsigned int MAGIC_NUMBER1 = 0x19920201;
  const unsigned int MAGIC_NUMBER2 = 0x08051803;

  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Header& header);

  unsigned int magicNumber1 = 0, headerSize = 0, lowDateTime = 0, highDateTime = 0, magicNumber2 = 0,
               numSections = 0;
  time_t creationTimestamp = 0;
  bool validMagicNumber = true, validSize = true, read = false;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_HEADER_H
