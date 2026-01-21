/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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
#include "fs/navdatabaseoptions.h"

#include <QtDebug>

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

Header::Header(const atools::fs::NavDatabaseOptions *options, BinaryStream *stream)
  : BglBase(options, stream)
{
  magicNumber1 = stream->readUInt();
  if(magicNumber1 != MAGIC_NUMBER1)
    validMagicNumber = false;

  headerSize = stream->readUInt();
  if(headerSize != HEADER_SIZE)
  {
    if(options->getSimulatorType() != atools::fs::FsPaths::SimulatorType::MSFS)
      qWarning().nospace().noquote() << "Invalid header size: 0x" << Qt::hex << headerSize << Qt::dec;
    // Disabled for www.fsaerodata.com
    // validSize = false;
  }

  lowDateTime = stream->readUInt();
  highDateTime = stream->readUInt();

  magicNumber2 = stream->readUInt();
  if(magicNumber2 != MAGIC_NUMBER2)
    validMagicNumber = false;

  if(!validMagicNumber && options->getSimulatorType() != atools::fs::FsPaths::SimulatorType::MSFS)
    qWarning().nospace().noquote() << "Invalid magic number: 0x" << Qt::hex << magicNumber1 << ", 0x" << magicNumber2 << Qt::dec;

  // if(!validSize)
  // qWarning().nospace().noquote() << "Invalid header size: 0x" << Qt::hex << headerSize << Qt::dec;

  if(!validMagicNumber || !validSize)
    // Stop reading here if anything is wrong
    return;

  creationTimestamp = converter::filetime(lowDateTime, highDateTime);
  numSections = stream->readUInt();

  // QMIDs
  stream->skip(4 * 8);

  read = true;
}

Header::~Header()
{
}

QString Header::getCreationTimestampString() const
{
  QDateTime dt;
  dt.setSecsSinceEpoch(static_cast<uint>(creationTimestamp));
  return dt.toString(Qt::ISODate);
}

QDebug operator<<(QDebug out, const Header& header)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const BglBase&>(header)
                          << Qt::hex << " Header[magic number 1 0x" << header.magicNumber1 << Qt::dec
                          << ", size " << header.headerSize
                          << Qt::hex << ", low timestamp 0x" << header.lowDateTime
                          << ", high timestamp 0x" << header.highDateTime << Qt::dec
                          << ", timestamp " << header.getCreationTimestampString()
                          << Qt::hex << ", magic number 2 0x" << header.magicNumber2 << Qt::dec
                          << ", sections " << header.numSections
                          << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
