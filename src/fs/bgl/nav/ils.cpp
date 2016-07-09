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

#include "fs/bgl/nav/ils.h"
#include "fs/bgl/nav/localizer.h"
#include "fs/bgl/nav/dme.h"
#include "fs/bgl/nav/glideslope.h"

#include "fs/bgl/converter.h"
#include "fs/bgl/recordtypes.h"
#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

enum IlsFlags
{
  FLAGS_DME_ONLY = 1 << 0,
  FLAGS_BC = 1 << 2,
  FLAGS_GS = 1 << 3,
  FLAGS_DME = 1 << 4,
  FLAGS_NAV = 1 << 5
};

Ils::Ils(const BglReaderOptions *options, BinaryStream *bs)
  : NavBase(options, bs), localizer(nullptr), glideslope(nullptr), dme(nullptr)
{
  bs->readUByte();
  int flags = bs->readUByte();

  backcourse = (flags & FLAGS_BC) == FLAGS_BC;
  // TODO  compare values with record presence
  // dmeOnlyOrIls = (flags & FLAGS_DME_ONLY) == FLAGS_DME_ONLY;
  // hasGlideslope = (flags & FLAGS_GS) == FLAGS_GS;
  // hasDme = (flags & FLAGS_DME) == FLAGS_DME;
  // hasNav = (flags & FLAGS_NAV) == FLAGS_NAV;

  // ILS transmitter position
  position = BglPosition(bs, true, 1000.f);
  frequency = bs->readInt() / 1000;
  range = bs->readFloat();
  magVar = converter::adjustMagvar(bs->readFloat());

  // ILS ident
  ident = converter::intToIcao(bs->readUInt());

  unsigned int regionFlags = bs->readUInt();
  // Two letter region code
  region = converter::intToIcao(regionFlags & 0x7ff, true); // TODO wiki region is never set
  // Read airport ICAO ident
  airportIdent = converter::intToIcao((regionFlags >> 11) & 0x1fffff, true);

  // Read all subrecords of ILS
  while(bs->tellg() < startOffset + size)
  {
    Record r(options, bs);
    rec::IlsVorRecordType t = r.getId<rec::IlsVorRecordType>();

    switch(t)
    {
      case rec::ILS_VOR_NAME:
        name = bs->readString(r.getSize() - Record::SIZE);
        break;
      case rec::LOCALIZER:
        // This is actually not optional for an ILS
        r.seekToStart();
        localizer = new Localizer(options, bs);
        break;
      case rec::GLIDESLOPE:
        r.seekToStart();
        glideslope = new Glideslope(options, bs);
        break;
      case rec::DME:
        r.seekToStart();
        dme = new Dme(options, bs);
        break;
      default:
        qWarning().nospace().noquote() << "Unexpected record type in ILS record 0x"
                                       << hex << t << dec << " for ident "
                                       << ident;
    }
    r.seekToEnd();
  }
}

Ils::~Ils()
{
  delete localizer;
  delete dme;
  delete glideslope;
}

QDebug operator<<(QDebug out, const Ils& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const NavBase&>(record)
  << " Ils[backcourse " << record.backcourse;
  if(record.localizer != nullptr)
    out << ", " << *record.localizer;
  if(record.glideslope != nullptr)
    out << ", " << *record.glideslope;
  if(record.dme != nullptr)
    out << ", " << *record.dme;
  out << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
