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

#include "fs/bgl/header.h"
#include "fs/bgl/converter.h"
#include "io/binarystream.h"

#include <QtDebug>

#include <time.h>

namespace atools {
namespace fs {
namespace bgl {
using atools::io::BinaryStream;

Header::Header(const atools::fs::NavDatabaseOptions *options, BinaryStream *bs)
  : BglBase(options, bs)
{
  magicNumber1 = bs->readUInt();
  if(magicNumber1 != MAGIC_NUMBER1)
    validMagicNumber = false;

  headerSize = bs->readUInt();
  if(headerSize != HEADER_SIZE)
  {
    qWarning().nospace().noquote() << "Invalid header size: 0x" << hex << headerSize << dec;
    // Disabled for www.fsaerodata.com
    // validSize = false;
  }

  lowDateTime = bs->readUInt();
  highDateTime = bs->readUInt();

  magicNumber2 = bs->readUInt();
  if(magicNumber2 != MAGIC_NUMBER2)
    validMagicNumber = false;

  if(!validMagicNumber)
    qWarning().nospace().noquote() << "Invalid magic number: 0x" << hex << magicNumber1
                                   << ", 0x" << magicNumber2 << dec;

  // if(!validSize)
  // qWarning().nospace().noquote() << "Invalid header size: 0x" << hex << headerSize << dec;

  if(!validMagicNumber || !validSize)
    // Stop reading here if anything is wrong
    return;

  creationTimestamp = converter::filetime(lowDateTime, highDateTime);
  numSections = bs->readUInt();

  // QMIDs
  bs->skip(4 * 8);

  read = true;
}

Header::~Header()
{
}

QString Header::getCreationTimestampString() const
{
  QDateTime dt;
  dt.setTime_t(static_cast<uint>(creationTimestamp));
  return dt.toString(Qt::ISODate);
}

QDebug operator<<(QDebug out, const Header& header)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const BglBase&>(header)
  << hex << " Header[magic number 1 0x" << header.magicNumber1 << dec
  << ", size " << header.headerSize
  << hex << ", low timestamp 0x" << header.lowDateTime
  << ", high timestamp 0x" << header.highDateTime << dec
  << ", timestamp " << header.getCreationTimestampString()
  << hex << ", magic number 2 0x" << header.magicNumber2 << dec
  << ", sections " << header.numSections
  << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
