/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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

#include "fs/bgl/nav/tacan.h"

#include "fs/bgl/nav/dme.h"
#include "io/binarystream.h"
#include "fs/bgl/converter.h"
#include "fs/bgl/recordtypes.h"
#include "fs/navdatabaseoptions.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
using Qt::hex;
using Qt::dec;
#endif

Tacan::Tacan(const NavDatabaseOptions *options, BinaryStream *stream)
  : NavBase(options, stream)
{
  position = BglPosition(stream, true, 1000.f);
  channel = stream->readInt();

  qint8 flags = stream->readByte();
  channelId = (flags & 0x1) ? 'Y' : 'X';
  dmeOnly = (flags & 0x2) == 0;
  range = stream->readFloat();
  magVar = converter::adjustMagvar(stream->readFloat());
  ident = converter::intToIcao(stream->readUInt());
  unsigned int regionFlags = stream->readUInt();
  region = converter::intToIcao(regionFlags & 0x7ff, true);
  airportIdent = converter::intToIcao((regionFlags >> 11) & 0x1fffff, true);

  atools::io::Encoding encoding = options->getSimulatorType() ==
                                  atools::fs::FsPaths::MSFS ? atools::io::UTF8 : atools::io::LATIN1;

  while(stream->tellg() < startOffset + size)
  {
    Record r(options, stream);
    rec::IlsVorRecordType t = r.getId<rec::IlsVorRecordType>();
    if(checkSubRecord(r))
      return;

    switch(t)
    {
      case rec::ILS_VOR_NAME:
        name = stream->readString(r.getSize() - Record::SIZE, encoding);
        break;
      case rec::DME:
        r.seekToStart();
        dme = new Dme(options, stream);
        break;
      case rec::LOCALIZER:
      case rec::GLIDESLOPE:
        break;
      default:
        qWarning().nospace().noquote() << "Unexpected record type in TACAN record 0x" << hex << t << dec
                                       << " for ident " << ident;
    }
    r.seekToEnd();
  }
}

Tacan::~Tacan()
{
  delete dme;
}

QDebug operator<<(QDebug out, const Tacan& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const NavBase&>(record)
                          << " Tacan["
                          << "channel" << record.getChannel();
  out << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
