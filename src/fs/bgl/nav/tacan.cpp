/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel albar965@mailbox.org
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

#include "fs/bgl/nav/localizer.h"
#include "fs/bgl/nav/dme.h"
#include "fs/bgl/nav/glideslope.h"
#include "io/binarystream.h"

#include "fs/bgl/converter.h"
#include "fs/bgl/recordtypes.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

Tacan::Tacan(const NavDatabaseOptions *options, BinaryStream *bs)
  : NavBase(options, bs)
{
  position = BglPosition(bs, true, 1000.f);
  channel = bs->readInt();

  qint8 flags = bs->readByte();
  channelId = (flags & 0x1) ? 'Y' : 'X';
  dmeOnly = (flags & 0x2) == 0;
  range = bs->readFloat();
  magVar = converter::adjustMagvar(bs->readFloat());
  ident = converter::intToIcao(bs->readUInt());
  unsigned int regionFlags = bs->readUInt();
  region = converter::intToIcao(regionFlags & 0x7ff, true);
  airportIdent = converter::intToIcao((regionFlags >> 11) & 0x1fffff, true);

  while(bs->tellg() < startOffset + size)
  {
    Record r(options, bs);
    rec::IlsVorRecordType t = r.getId<rec::IlsVorRecordType>();
    if(checkSubRecord(r))
      return;

    switch(t)
    {
      case rec::ILS_VOR_NAME:
        name = bs->readString(r.getSize() - Record::SIZE);
        break;
      case rec::DME:
        r.seekToStart();
        dme = new Dme(options, bs);
        break;
      case rec::LOCALIZER:
      case rec::GLIDESLOPE:
        break;
      default:
        qWarning().nospace().noquote() << "Unexpected record type in TACAN record 0x" << hex << t << dec <<
        " for ident " << ident;
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
