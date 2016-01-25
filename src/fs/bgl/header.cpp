/*
 * Header.cpp
 *
 *  Created on: 19.04.2015
 *      Author: alex
 */

#include "fs/bgl/header.h"
#include "fs/bgl/converter.h"
#include "io/binarystream.h"
#include "fs/bgl/bglexception.h"

#include <QtDebug>

#include <time.h>

namespace atools {
namespace fs {
namespace bgl {
using atools::io::BinaryStream;

Header::Header(BinaryStream *bs)
  : BglBase(bs)
{
  magicNumber1 = bs->readInt();

  if(magicNumber1 != 0x19920201)
    qWarning().nospace().noquote() << "Invalid magic number: 0x" << hex << magicNumber1
                                   << ". File \"" << bs->getFilename() << "\" at offset "
                                   << bs->tellg() << ".";

  headerSize = bs->readInt();
  lowDateTime = bs->readInt();
  highDateTime = bs->readInt();
  creationTimestamp = converter::filetime(lowDateTime, highDateTime);

  magicNumber2 = bs->readInt();
  if(magicNumber2 != 0x08051803)
    qWarning().nospace().noquote() << "Invalid magic number 2: 0x" << hex << magicNumber2
                                   << ". File \"" << bs->getFilename() << "\" at offset "
                                   << bs->tellg() << ".";

  numSections = bs->readInt();

  // QMIDs
  bs->skip(4 * 8);
}

Header::~Header()
{
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
